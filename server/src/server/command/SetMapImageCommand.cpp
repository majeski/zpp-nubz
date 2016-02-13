#include <boost/filesystem.hpp>
#include <Magick++.h>

#include <utils/log.h>
#include <utils/ImageProcessor.h>

#include <db/command/GetMapImages.h>
#include <db/command/IncrementCounter.h>
#include <db/command/SaveMapImage.h>
#include <db/command/GetMapTiles.h>
#include <db/command/RemoveMapTiles.h>
#include <db/command/RemoveMapTilesInfo.h>
#include <db/command/SaveMapTile.h>
#include <db/command/SaveMapTilesInfo.h>

#include <server/utils/FileHelper.h>
#include <server/io/InvalidInput.h>

#include "SetMapImageCommand.h"

namespace server {
namespace command {

const std::vector<SetMapImageCommand::ZoomLevelInfo> SetMapImageCommand::zoomLevels = {
    {1, 1024, 64}, {2, 2048, 128}, {3, 4096, 256}, {4, 8192, 512}};

SetMapImageCommand::SetMapImageCommand(db::Database &db) : db(db) {
}

// 1. create full map image and save it in public directory
// 2. create tiles and save them in tmp directory inside map tiles directory
// 3. update data in database
// 4. remove old tiles, old map, tmp map
// 5. rename tmp tiles directory to destination name
void SetMapImageCommand::operator()(const io::input::SetMapImageRequest &input) {
    prepareImageProcessor(input.filename);
    db::MapImage fullMap = createFullMapImage(input.floor);

    std::vector<::utils::FileHandler> handlers;
    handlers.emplace_back(utils::FileHelper::getInstance().pathForPublicFile(fullMap.filename));
    handlers.push_back(createTiles(input.floor));
    boost::filesystem::path tmpDirPath = handlers.back().getPath();

    LOG(INFO) << "Saving information about image and tiles to database";
    db.execute([&](db::DatabaseSession &session) {
        std::vector<boost::filesystem::path> filesToDelete;
        if (auto oldFullMapPath = getOldMap(input.floor, session)) {
            filesToDelete.push_back(oldFullMapPath.value());
        }
        filesToDelete.push_back(finalTilesDir(input.floor));
        filesToDelete.push_back(utils::FileHelper::getInstance().pathForTmpFile(input.filename));

        fullMap.version = db::cmd::IncrementCounter::mapVersion()(session);
        db::cmd::SaveMapImage{fullMap}(session);

        db::cmd::RemoveMapTiles{input.floor}(session);
        db::cmd::RemoveMapTilesInfo{input.floor}(session);

        for (const auto &tile : tiles) {
            db::cmd::SaveMapTile{tile}(session);
        }
        for (const auto &tileInfo : tilesInfo) {
            db::cmd::SaveMapTilesInfo{tileInfo}(session);
        }

        for (const auto &toDel : filesToDelete) {
            if (boost::filesystem::exists(toDel)) {
                boost::filesystem::remove_all(toDel);
            }
        }
        switchToNewTiles(tmpDirPath, input.floor);
    });

    for (auto &handler : handlers) {
        handler.release();
    }
}

void SetMapImageCommand::prepareImageProcessor(const std::string &originalMapFilename) {
    const auto pathToImg = utils::FileHelper::getInstance().pathForTmpFile(originalMapFilename);
    imgProc.reset(new ::utils::ImageProcessor(pathToImg.string()));
}

db::MapImage SetMapImageCommand::createFullMapImage(std::int32_t floor) {
    LOG(INFO) << "Preparing full map image";
    imgProc->reset();
    imgProc->scale(zoomLevels.back().size);
    imgProc->addFrameToBeDivisibleBy(zoomLevels.back().tileSize);

    db::MapImage mapImage;
    mapImage.width = (std::int32_t)imgProc->width();
    mapImage.height = (std::int32_t)imgProc->height();
    mapImage.floor = floor;
    mapImage.filename =
        "map_f" + std::to_string(floor) + "_" + std::to_string(rand() % 1000) + ".jpg";

    imgProc->saveImageTo(
        utils::FileHelper::getInstance().pathForPublicFile(mapImage.filename).string());
    return mapImage;
}

::utils::FileHandler SetMapImageCommand::createTiles(std::int32_t floor) {
    tiles.clear();
    tilesInfo.clear();

    boost::filesystem::path mapTilesDir =
        utils::FileHelper::getInstance().pathForMapTilesDirectory();
    boost::filesystem::path mapTilesDirShort;
    mapTilesDir /= "TMP";
    mapTilesDirShort /= std::to_string(floor);
    boost::filesystem::create_directory(mapTilesDir);

    ::utils::FileHandler handler(mapTilesDir.string());

    db::MapTilesInfo mapTilesInfo;
    mapTilesInfo.floor = floor;
    for (const auto &zoomLevelInfo : zoomLevels) {
        LOG(INFO) << "Preparing image with zoom level " << zoomLevelInfo.levelIdx;
        imgProc->reset();
        imgProc->scale(zoomLevelInfo.size);
        imgProc->setTileSize(zoomLevelInfo.tileSize);
        imgProc->addFrameToBeDivisibleBy(zoomLevelInfo.tileSize);

        boost::filesystem::path levelDir = mapTilesDir / std::to_string(zoomLevelInfo.levelIdx);
        boost::filesystem::path levelDirShort =
            mapTilesDirShort / std::to_string(zoomLevelInfo.levelIdx);
        boost::filesystem::create_directory(levelDir);

        LOG(INFO) << "Generating tiles for zoom level " << zoomLevelInfo.levelIdx;
        auto tilesNames = imgProc->generateTiles(levelDir);

        mapTilesInfo.imgHeight = (std::int32_t)imgProc->height();
        mapTilesInfo.imgWidth = (std::int32_t)imgProc->width();
        mapTilesInfo.rowsCount = (std::int32_t)tilesNames.size();
        mapTilesInfo.colsCount = (std::int32_t)tilesNames.back().size();
        mapTilesInfo.tileSize = (std::int32_t)zoomLevelInfo.tileSize;
        mapTilesInfo.zoomLevel = zoomLevelInfo.levelIdx;
        tilesInfo.push_back(mapTilesInfo);

        addTiles(tilesNames, floor, zoomLevelInfo.levelIdx, levelDirShort.string());
    }

    return handler;
}

void SetMapImageCommand::addTiles(const std::vector<std::vector<std::string>> &tilesNames,
                                  std::int32_t floor, std::int32_t zoomLevel,
                                  const std::string &pathPrefix) {
    db::MapTile mapTile;
    mapTile.zoomLevel = zoomLevel;
    mapTile.floor = floor;
    for (std::size_t rowIdx = 0; rowIdx < tilesNames.size(); rowIdx++) {
        const auto &row = tilesNames[rowIdx];
        for (std::size_t colIdx = 0; colIdx < row.size(); colIdx++) {
            mapTile.row = (std::int32_t)rowIdx;
            mapTile.col = (std::int32_t)colIdx;
            mapTile.filename = (boost::filesystem::path(pathPrefix) / row[colIdx]).string();
            tiles.push_back(mapTile);
        }
    }
}

boost::optional<boost::filesystem::path> SetMapImageCommand::getOldMap(
    std::int32_t floor, db::DatabaseSession &session) const {
    db::cmd::GetMapImages getMapImages(floor);
    getMapImages(session);

    if (!getMapImages.getResult().empty()) {
        auto mapImage = getMapImages.getResult().front();
        return utils::FileHelper::getInstance().pathForPublicFile(mapImage.filename);
    } else {
        return {};
    }
}

void SetMapImageCommand::switchToNewTiles(const boost::filesystem::path &tmpDir,
                                          std::int32_t floor) const {
    boost::filesystem::rename(tmpDir, finalTilesDir(floor));
}

boost::filesystem::path SetMapImageCommand::finalTilesDir(std::int32_t floor) const {
    return boost::filesystem::path(utils::FileHelper::getInstance().pathForMapTilesDirectory() /
                                   std::to_string(floor));
}
}
}