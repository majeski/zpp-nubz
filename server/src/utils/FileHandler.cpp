#include "FileHandler.h"

namespace utils {

FileHandler::FileHandler(const std::string &path) : released(false), path(path) {
}

FileHandler::FileHandler(const boost::filesystem::path &path) : released(false), path(path) {
}

FileHandler::FileHandler(FileHandler &&that) : released(that.released), path(that.path) {
    that.released = true;
}

FileHandler::~FileHandler() {
    if (!released) {
        auto ec = boost::system::error_code{};
        boost::filesystem::remove_all(path, ec);
    }
}

const boost::filesystem::path &FileHandler::getPath() const noexcept {
    return path;
}

void FileHandler::release() noexcept {
    released = true;
}
}