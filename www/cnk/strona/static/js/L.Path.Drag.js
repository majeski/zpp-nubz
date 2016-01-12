"use strict";L.Path.include({transform:function(t){if(this._renderer){if(t){this._renderer.transformPath(this,t)}else{this._renderer._resetTransformPath(this);this._update()}}return this},_onMouseClick:function(t){if(this.dragging&&this.dragging.moved()||this._map.dragging&&this._map.dragging.moved()){return}this._fireMouseEvent(t)}});"use strict";L.Handler.PathDrag=L.Handler.extend({initialize:function(t){this._path=t;this._matrix=null;this._startPoint=null;this._dragStartPoint=null;this._mapDraggingWasEnabled=false},addHooks:function(){this._path.on("mousedown",this._onDragStart,this);if(this._path._path){L.DomUtil.addClass(this._path._path,"leaflet-path-draggable")}},removeHooks:function(){this._path.off("mousedown",this._onDragStart,this);if(this._path._path){L.DomUtil.removeClass(this._path._path,"leaflet-path-draggable")}},moved:function(){return this._path._dragMoved},_onDragStart:function(t){var a=t.originalEvent._simulated?"touchstart":t.originalEvent.type;this._mapDraggingWasEnabled=false;this._startPoint=t.containerPoint.clone();this._dragStartPoint=t.containerPoint.clone();this._matrix=[1,0,0,1,0,0];L.DomEvent.stop(t.originalEvent);L.DomUtil.addClass(this._path._renderer._container,"leaflet-interactive");L.DomEvent.on(document,L.Draggable.MOVE[a],this._onDrag,this).on(document,L.Draggable.END[a],this._onDragEnd,this);if(this._path._map.dragging.enabled()){this._path._map.dragging._draggable._onUp();this._path._map.dragging.disable();this._mapDraggingWasEnabled=true}this._path._dragMoved=false;if(this._path._popup){this._path._popup._close()}},_onDrag:function(t){L.DomEvent.stop(t);var a=t.touches&&t.touches.length>=1?t.touches[0]:t;var i=this._path._map.mouseEventToContainerPoint(a);var n=i.x;var r=i.y;var e=n-this._startPoint.x;var s=r-this._startPoint.y;if(!this._path._dragMoved&&(e||s)){this._path._dragMoved=true;this._path.fire("dragstart",t);this._path.bringToFront()}this._matrix[4]+=e;this._matrix[5]+=s;this._startPoint.x=n;this._startPoint.y=r;this._path.fire("predrag",t);this._path.transform(this._matrix);this._path.fire("drag",t)},_onDragEnd:function(t){var a=t.type;var i=this._path._map.mouseEventToContainerPoint(t);if(this.moved()){this._transformPoints(this._matrix);this._path._project();this._path.transform(null)}L.DomEvent.off(document,"mousemove touchmove",this._onDrag,this).off(document,"mouseup touchend",this._onDragEnd,this);this._path.fire("dragend",{distance:Math.sqrt(L.LineUtil._sqDist(this._dragStartPoint,i))});this._matrix=null;this._startPoint=null;this._dragStartPoint=null;if(this._mapDraggingWasEnabled){this._path._map.dragging.enable()}},_transformPoints:function(t){var a=this._path;var i,n,r;var e=L.point(t[4],t[5]);var s=a._map.options.crs;var o=s.transformation;var h=s.scale(a._map.getZoom());var _=s.projection;var d=o.untransform(e,h).subtract(o.untransform(L.point(0,0),h));a._bounds=new L.LatLngBounds;if(a._point){a._latlng=_.unproject(_.project(a._latlng)._add(d));a._point._add(e)}else if(a._rings||a._parts){var g=a._rings||a._parts;var l=a._latlngs;if(!L.Util.isArray(l[0])){l=[l]}for(i=0,n=g.length;i<n;i++){for(var p=0,f=g[i].length;p<f;p++){r=l[i][p];l[i][p]=_.unproject(_.project(r)._add(d));a._bounds.extend(l[i][p]);g[i][p]._add(e)}}}a._updatePath()}});L.Path.addInitHook(function(){if(this.options.draggable){if(this.dragging){this.dragging.enable()}else{this.dragging=new L.Handler.PathDrag(this);this.dragging.enable()}}else if(this.dragging){this.dragging.disable()}});L.SVG.include({_resetTransformPath:function(t){t._path.setAttributeNS(null,"transform","")},transformPath:function(t,a){t._path.setAttributeNS(null,"transform","matrix("+a.join(" ")+")")}});L.SVG.include(!L.Browser.vml?{}:{_resetTransformPath:function(t){if(t._skew){t._skew.on=false;t._path.removeChild(t._skew);t._skew=null}},transformPath:function(t,a){var i=t._skew;if(!i){i=L.SVG.create("skew");t._path.appendChild(i);i.style.behavior="url(#default#VML)";t._skew=i}var n=a[0].toFixed(8)+" "+a[1].toFixed(8)+" "+a[2].toFixed(8)+" "+a[3].toFixed(8)+" 0 0";var r=Math.floor(a[4]).toFixed()+", "+Math.floor(a[5]).toFixed()+"";var e=this._path.style;var s=parseFloat(e.left);var o=parseFloat(e.top);var h=parseFloat(e.width);var _=parseFloat(e.height);if(isNaN(s))s=0;if(isNaN(o))o=0;if(isNaN(h)||!h)h=1;if(isNaN(_)||!_)_=1;var d=(-s/h-.5).toFixed(8)+" "+(-o/_-.5).toFixed(8);i.on="f";i.matrix=n;i.origin=d;i.offset=r;i.on=true}});L.Util.trueFn=function(){return true};L.Canvas.include({_resetTransformPath:function(t){if(!this._containerCopy){return}delete this._containerCopy;if(t._containsPoint_){t._containsPoint=t._containsPoint_;delete t._containsPoint_;this._requestRedraw(t);this._draw(true)}},transformPath:function(t,a){var i=this._containerCopy;var n=this._ctx;var r=L.Browser.retina?2:1;var e=this._bounds;var s=e.getSize();var o=L.DomUtil.getPosition(this._container);if(!i){i=this._containerCopy=document.createElement("canvas");document.body.appendChild(i);i.width=r*s.x;i.height=r*s.y;t._removed=true;this._redraw();i.getContext("2d").translate(r*e.min.x,r*e.min.y);i.getContext("2d").drawImage(this._container,0,0);this._initPath(t);t._containsPoint_=t._containsPoint;t._containsPoint=L.Util.trueFn}n.save();n.setTransform(1,0,0,1,0,0);n.clearRect(o.x,o.y,s.x*r,s.y*r);n.restore();n.save();n.drawImage(this._containerCopy,0,0,s.x,s.y);n.transform.apply(n,a);var h=this._layers;this._layers={};this._initPath(t);t._updatePath();this._layers=h;n.restore()}});