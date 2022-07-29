
#pragma once

namespace rynx {
	template<typename T, typename IOStream> void serialize(const T& t, IOStream& io);
	template<typename T, typename IOStream> void deserialize(T& t, IOStream& io);
	template<typename T, typename IOStream> T deserialize(IOStream& io);

	namespace serialization {
		template<typename T> struct Serialize;
	}
}
