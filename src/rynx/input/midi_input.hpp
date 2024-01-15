
#pragma once

#include <rynx/std/vector.hpp>
#include <rynx/std/string.hpp>
#include <rynx/std/function.hpp>

namespace rynx::io::midi {
	class device {
	public:
		device(void* impl) : pDeviceImplementation(impl) {}
		~device() { close(); }

		device(device&& other) {
			close();
			pDeviceImplementation = other.pDeviceImplementation;
			other.pDeviceImplementation = nullptr;
		}

		device& operator = (const device& other) const = delete;

		device& operator = (device&& other) {
			close();
			pDeviceImplementation = other.pDeviceImplementation;
			other.pDeviceImplementation = nullptr;
			return *this;
		}

		// state (on/off), midi key number, velocity
		bool open(int midiDeviceIndex, rynx::function<void(uint8_t, uint8_t, uint8_t)>);
		void close();
		
	private:
		void* pDeviceImplementation = nullptr;
	};

	std::vector<rynx::string> list_devices();
	rynx::io::midi::device create_input_stream();
}