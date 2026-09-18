// audio_item.h
// Collects sound-card / audio-server information.
// Perl source: package AudioItem (lines ~7672–8256)
//
// Perl subs mapped:
//   get()            → AudioItem::get()
//   device_output()  → AudioItem::device_output()
//   asound_output()  → AudioItem::asound_output()
//   usb_output()     → AudioItem::usb_output()
//   sound_output()   → AudioItem::sound_output()
//   sound_data()     → AudioItem::sound_data()
//   jack_status()    → AudioItem::jack_status()
//   pipewire_status()→ AudioItem::pipewire_status()
//   pulse_status()   → AudioItem::pulse_status()
//   sound_helpers()  → AudioItem::sound_helpers()
//   sound_tools()    → AudioItem::sound_tools()
#pragma once
#include "items/info_item.h"
#include <vector>
#include <string>

namespace inxi {

struct SoundCard {
    std::string id;
    std::string driver;
    std::string name;
    std::string chip_name;
    bool is_usb = false;
    std::string usb_id;
};

struct AudioServer {
    std::string name;    // "PipeWire"|"PulseAudio"|"JACK"|""
    std::string version;
    std::string status;  // "active"|"inactive"|""
};

class AudioItem final : public InfoItem {
public:
    explicit AudioItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Audio"; }
    std::vector<OutputRow> get() override;

private:
    // Perl: device_output() — formats PCI/ALSA sound card rows
    std::vector<OutputRow> device_output(const std::vector<SoundCard>& cards);

    // Perl: asound_output() — parses /proc/asound/cards
    std::vector<OutputRow> asound_output();

    // Perl: usb_output() — formats USB audio device rows
    std::vector<OutputRow> usb_output(const std::vector<SoundCard>& cards);

    // Perl: sound_output() — top-level row builder
    std::vector<OutputRow> sound_output(const std::vector<SoundCard>& cards,
                                        const AudioServer& server);

    // Perl: sound_data() — collects all raw sound data
    std::vector<SoundCard> sound_data();

    // Perl: jack_status() — detects running JACK instance
    AudioServer jack_status();

    // Perl: pipewire_status() — detects running PipeWire instance
    AudioServer pipewire_status();

    // Perl: pulse_status() — detects running PulseAudio instance
    AudioServer pulse_status();

    // Perl: sound_helpers() — helper formatting utilities
    std::string sound_helpers(const std::string& key, const std::string& val);

    // Perl: sound_tools() — checks which audio tools are present
    void sound_tools();
};

}  // namespace inxi
