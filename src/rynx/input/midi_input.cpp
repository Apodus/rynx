
#define NOMINMAX

#include <windows.h>
#include <mmsystem.h>

#include <rynx/input/midi_input.hpp>
#include <rynx/std/unordered_map.hpp>

#include <iostream>
#include <vector>
#include <string_view>
#include <string>
#include <codecvt>

// implementation details
namespace {
	rynx::unordered_map<void*, rynx::function<void(uint8_t, uint8_t, uint8_t)>> g_midiInputMappings;

	constexpr int numNotesInOctave = 12;
	constexpr std::string_view sMidiKeyNames[] = {
		"A",
		"A#",
		"B",
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#"
	};

	constexpr int sKeyOffsets[] = {
		0, // a
		2, // b
		3, // c
		5, // d
		7, // e
		8, // f
		10 // g
	};

	std::string_view noteNumberToKey(int noteNumber) {
		return sMidiKeyNames[(noteNumber - 9 + ::numNotesInOctave) % ::numNotesInOctave];
	}

	int noteNumberToOctave(int noteNumber) {
		return (noteNumber - 9) / ::numNotesInOctave;
	}

	int musicalNoteToMidiKeyNumber(char c, int octave, bool sharp) {
		// It is known that c4 = 60
		// Checking:   4    *        12          +          3           +   0   = 51, therefore we add an offset of 9 to final result
		int number = octave * ::numNotesInOctave + sKeyOffsets[c - 'a'] + sharp;
		return number + 9;
	}

	rynx::string from_wstring(std::wstring wstr) {
		// Convert wstring to string using codecvt
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string str = converter.to_bytes(wstr);
		return rynx::string(str.c_str(), str.length());
	}

	// MIDI callback function
	void CALLBACK MidiInCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR /* dwInstance */, DWORD_PTR dwParam1, DWORD_PTR /* dwParam2 */) {
		
		switch (wMsg)
		{
		case MIM_OPEN:
			// MIDI input device opened successfully
			break;

		case MIM_CLOSE:
			// MIDI input device closed
			logmsg("device is closed now hehe");
			break;

		case MIM_DATA:
		{
			// MIDI data received, process it
			// dwParam1 contains the MIDI message
			BYTE status = (BYTE)dwParam1;
			BYTE data1 = (BYTE)(dwParam1 >> 8);
			BYTE data2 = (BYTE)(dwParam1 >> 16);
			auto mapping_it = g_midiInputMappings.find(hMidiIn);
			if (mapping_it != g_midiInputMappings.end())
				mapping_it->second(status, data1, data2);
			break;
		}
		case MIM_LONGDATA:
			// Sysex or long MIDI data received, process it
			logmsg("long data");
			break;

		case MIM_ERROR:
		{
			// MIDI input error occurred
			// dwParam1 contains the error code
			MMRESULT error = static_cast<MMRESULT>(dwParam1);

			// Handle the error
			switch (error)
			{
			case MMSYSERR_ALLOCATED:
				logmsg("Device already allocated!");
				break;

			case MMSYSERR_BADDEVICEID:
				logmsg("Invalid device id!");
				break;

			case MMSYSERR_INVALPARAM:
				logmsg("Invalid parameter!");
				break;

			case MMSYSERR_HANDLEBUSY:
				logmsg("Handle simultaneously in use elsewhere or something!");
				break;

				// Add more error cases as needed

			default:
				logmsg("random error");
				// Unknown error
				break;
			}

			break;
		}
		default:
			// Other MIDI messages
			break;
		}
	}

	class MidiDeviceListener {
	public:
		MidiDeviceListener() {}

		~MidiDeviceListener() {
			closeCurrentDevice();
		}

		void closeCurrentDevice() {
			if (m_currentDevice >= 0) {
				midiInStop(m_handleMidiIn);
				midiInClose(m_handleMidiIn);
				m_currentDevice = -1;
				m_handleMidiIn = nullptr;
			}
		}

		void* getHandle() const {
			return m_handleMidiIn;
		}

		bool openDeviceByIndex(int32_t index) {
			closeCurrentDevice();
			MMRESULT result = midiInOpen(
				&m_handleMidiIn,
				index,
				(DWORD_PTR)MidiInCallback,
				0,
				CALLBACK_FUNCTION);

			result = midiInReset(m_handleMidiIn);

			if (result != MMSYSERR_NOERROR) {
				std::cerr << "Failed to open MIDI input device." << std::endl;
				return false;
			}

			result = midiInStart(m_handleMidiIn);
			if (result != MMSYSERR_NOERROR) {
				std::cerr << "Failed to start MIDI input device." << std::endl;
				midiInClose(m_handleMidiIn);
				return false;
			}

			m_currentDevice = index;
			return true;
		}

	private:
		
		int32_t m_currentDevice = -1;
		HMIDIIN m_handleMidiIn = nullptr;
	};
}




rynx::vector<rynx::string> rynx::io::midi::list_devices() {
	UINT numDevices = midiInGetNumDevs();
	rynx::vector<rynx::string> midiDeviceNames;
	rynx::vector<MIDIINCAPS> midiDeviceCaps;
	midiDeviceCaps.resize(numDevices);

	for (UINT i = 0; i < numDevices; ++i) {
		if (midiInGetDevCaps(i, &midiDeviceCaps[i], sizeof(MIDIINCAPS)) == MMSYSERR_NOERROR) {
			midiDeviceNames.emplace_back(from_wstring(midiDeviceCaps[i].szPname));
		}
	}

	return midiDeviceNames;
}


rynx::io::midi::device rynx::io::midi::create_input_stream() {
	return { new ::MidiDeviceListener() };
}

bool rynx::io::midi::device::open(int midiDeviceIndex, rynx::function<void(uint8_t, uint8_t, uint8_t)> callback) {
	auto* ptr = static_cast<::MidiDeviceListener*>(this->pDeviceImplementation);
	if (!ptr->openDeviceByIndex(midiDeviceIndex))
		return false;
	
	// setup callback
	g_midiInputMappings[ptr->getHandle()] = callback;
	return true;
}

void rynx::io::midi::device::close() {
	if (this->pDeviceImplementation != nullptr) {
		auto* ptr = static_cast<::MidiDeviceListener*>(this->pDeviceImplementation);
		ptr->closeCurrentDevice();
		delete ptr;
		this->pDeviceImplementation = nullptr;
		g_midiInputMappings.erase(ptr->getHandle());
	}
}
