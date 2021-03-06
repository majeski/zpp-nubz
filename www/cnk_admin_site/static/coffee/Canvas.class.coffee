root = exports ? this
root.Canvas = class Canvas extends root.View
  # constructor :: (String, MapData) -> Context
  constructor: (@_containerMap, @_mapData) ->
    super()
    @_mapBounds = []
    @_exhibits = (new L.LayerGroup for i in @_mapData.availableFloors)
    @_floorLayer = (new L.LayerGroup for i in @_mapData.availableFloors)
    @_dataLoaded = (false for i in @_mapData.availableFloors)
    @_map = L.map(@_containerMap[1..], {
      minZoom: @_mapData.minZoom[0]
      zoom: @_mapData.minZoom[0]
      center: [0, 0]
      crs: L.CRS.Simple
      zoomControl: false
    })

    @_areLabelsVisible = true
    @_setHandlers()


  # _setHandlers :: () -> undefined
  _setHandlers: =>
    @_map.on('zoomend', =>  @fireEvents('zoomChange', @_map.getZoom()))
    return


  # _loadData :: Int -> undefined
  _loadData: (floor) =>
    tileInfo = @_mapData.floorTilesInfo[floor]
    if tileInfo.length is 0
      @_dataLoaded[floor] = true
      return
    @_addMapBounds(floor, [0, tileInfo[-1..][0].scaledSize.height],
                          [tileInfo[-1..][0].scaledSize.width, 0])
    @_addFloorLayer(floor)
    @addExhibits((e for id, e of @_mapData.exhibits when e.mapFrame?.floor is floor))
    @_dataLoaded[floor] = true
    return


  # _addMapBounds :: (Int, LatLng, LatLng) -> Context
  _addMapBounds: (floor, northEast, southWest) =>
    @_mapBounds[floor] = new L.LatLngBounds(@_map.unproject(northEast, @_mapData.maxZoom[floor]),
                                            @_map.unproject(southWest, @_mapData.maxZoom[floor]))
    @


  # _addFloorLayer :: Int -> Context
  _addFloorLayer: (floor) =>
    url = "#{@_getFloorUrl(floor)}?t=#{Math.floor(Math.random() * 1021)}"
    tileInfo = @_mapData.floorTilesInfo[floor]
    for j in [0...tileInfo.length]
      zoomLayer = L.tileLayer(url.replace('{z}', "#{j + @_mapData.minZoom[floor]}"), {
        minZoom: j + @_mapData.minZoom[floor]
        maxZoom: j + @_mapData.minZoom[floor]
        tileSize: tileInfo[j].tileSize.width
        continuousWorld: true
        crs: L.CRS.Simple
        bounds: @_mapBounds[floor]
      })
      @_floorLayer[floor].addLayer zoomLayer
    @


  # getFloorUrl :: Int -> String
  _getFloorUrl: (floor) =>
    @_mapData.floorUrl.replace("{f}", "#{floor}")


  # addExhibits :: [Exhibit] -> Context
  addExhibits: (exhibits) =>
    for e in exhibits
      floor = e.mapFrame.floor
      x = e.mapFrame.frame.x
      y = e.mapFrame.frame.y
      polygonBounds = new L.LatLngBounds(
        @_map.unproject([x, y], @_mapData.maxZoom[floor]),
        @_map.unproject([x + e.mapFrame.frame.size.width,
                         y + e.mapFrame.frame.size.height], @_mapData.maxZoom[floor]),
      )
      options =
          fillColor: e.rgbHex
          fillOpacity: 0.7
          weight: 1
          strokeColor: '#B4AFD1'
          strokeOpacity: 1
      r = L.rectangle(polygonBounds, @_exhibitOptions(options, exhibitId: e.exhibitId))
      r.bindLabel(e.name, direction: 'auto')
      @_exhibits[floor].addLayer(@_prepareExhibit(r))
    @_updateState(exhibits[0].mapFrame.floor) if exhibits.length > 0
    @


  # _updateState :: Int -> undefined
  _updateState: (floor) ->
    @_updateLabelsVisibility(floor)
    return


  # removeExhibit :: Int -> Context
  removeExhibit: (exhibitId) =>
    exhibitFrame = @_mapData.exhibits[exhibitId].mapFrame
    if not exhibitFrame?
      return
    @_exhibits[exhibitFrame.floor]?.eachLayer((layer) =>
      if parseInt(layer.options.exhibitId) is exhibitId
        @_exhibits[exhibitFrame.floor].removeLayer(layer)
    )
    @


  # _exhibitOptions :: [JsObject] -> JsObject
  _exhibitOptions: (options...) -> jQuery.extend(options...)


  # _prepareExhibit :: L.Rectangle -> L.Rectangle
  _prepareExhibit: (exhibit) ->
    exhibit.showLabel() if @_areLabelsVisible
    exhibit


  # setFloorLayer :: (Int, Int) -> Context
  setFloorLayer: (newFloor, activeFloor) =>
    if not @_dataLoaded[newFloor]
      @_loadData newFloor
    @_map.removeLayer(@_exhibits[activeFloor])
    @_map.removeLayer(@_floorLayer[activeFloor])
    @_map.addLayer(@_floorLayer[newFloor])
    @_map.addLayer(@_exhibits[newFloor])
    @_map.setView([0, 0], @_mapData.minZoom[newFloor], animate: false)
    @_map.setMaxBounds(@_mapBounds[newFloor])
    @_map.invalidateSize(animate: false)
    @_map.fireEvent('zoomend')
    @_updateState(newFloor)
    @


  # getVisibleFrame :: Int -> (L.Point, Int, Int)
  getVisibleFrame: (activeFloor) =>
    bounds = @_map.getBounds()
    maxZoomTileInfo = @_mapData.floorTilesInfo[activeFloor][-1..][0]
    maxX = maxZoomTileInfo?.scaledSize.width ? 0
    maxY = maxZoomTileInfo?.scaledSize.height ? 0
    maxZoom = @_mapData.maxZoom[activeFloor]
    castedPixelBounds = [
        @_map.project(bounds.getNorthWest(), maxZoom)
        @_map.project(bounds.getSouthEast(), maxZoom)
    ]
    min = castedPixelBounds[0]
    max = castedPixelBounds[1]
    topLeft = new L.Point(Math.min(maxX, Math.max(0, min.x)), Math.min(maxY, Math.max(0, min.y)))
    bottomRight = new L.Point(Math.min(maxX, Math.max(0, max.x)), Math.min(maxY, Math.max(0, max.y)))
    [topLeft, width = bottomRight.x - topLeft.x, height = bottomRight.y - topLeft.y]


  # flyToExhibit :: (Int, Int) -> Context
  flyToExhibit: (exhibitsId, oldFloor) =>
    exhibit = @_mapData.exhibits[exhibitsId]
    frame = exhibit.mapFrame.frame
    bounds = new L.LatLngBounds(
      @_map.unproject([frame.x, frame.y],
                       @_mapData.maxZoom[oldFloor]),
      @_map.unproject([frame.x + frame.size.width,
                       frame.y + frame.size.height],
                       @_mapData.maxZoom[oldFloor])
    )
    if oldFloor isnt exhibit.mapFrame.floor
      @setFloorLayer(exhibit.mapFrame.floor, oldFloor)
    @_map.flyToBounds(bounds, animate: false)
    @_map.fireEvent('zoomend')
    @


  # setLabelsVisibility :: (Boolean, Int) -> Context
  setLabelsVisibility: (@_areLabelsVisible, floor) =>
    @_updateLabelsVisibility(floor)
    @

  # _updateLabelsVisibility :: Int -> Context
  _updateLabelsVisibility: (floor) =>
    if @_areLabelsVisible
      @_exhibits[floor].invoke("showLabel")
    else
      @_exhibits[floor].invoke("hideLabel")
    @


  # zoomIn :: () -> Context
  zoomIn: =>
    @_map.zoomIn()
    @


  # zoomOut :: () -> Context
  zoomOut: =>
    @_map.zoomOut()
    @
