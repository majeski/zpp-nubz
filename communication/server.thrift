include "structs.thrift"

namespace cpp communication
namespace java com.cnk.communication

service Server {
	void shutdown(),
	i32 ping(1: structs.HelloMsg msg),

	structs.MapImagesResponse getMapImages(1: structs.MapImagesRequest request),
	void setMapImage(1: structs.SetMapImageRequest request),
}