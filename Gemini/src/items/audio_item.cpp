// audio_item.cpp
#include "items/audio_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<SoundCard> AudioItem::sound_data() {
    std::vector<SoundCard> cards;
    // Check lspci for audio controllers
    auto lines = utils::run_capture("lspci -nnk");
    SoundCard cur_card;
    bool in_audio = false;
    int dev_id = 1;

    for (const auto& line : lines) {
        if (line.find("Audio device") != std::string::npos ||
            line.find("Multimedia audio controller") != std::string::npos) {
            if (in_audio && !cur_card.name.empty()) {
                cards.push_back(cur_card);
            }
            in_audio = true;
            cur_card = SoundCard();
            cur_card.id = "Device-" + std::to_string(dev_id++);
            size_t colon = line.find(": ");
            if (colon != std::string::npos) {
                std::string rest = line.substr(colon + 2);
                size_t sub_colon = rest.find(": ");
                if (sub_colon != std::string::npos) {
                    cur_card.name = utils::clean(utils::clean_pci(rest.substr(sub_colon + 2)));
                } else {
                    cur_card.name = utils::clean(utils::clean_pci(rest));
                }
            }
        } else if (in_audio) {
            if (line.find("Kernel driver in use:") != std::string::npos) {
                cur_card.driver = utils::trim(line.substr(line.find(':') + 1));
            } else if (!line.empty() && line[0] != '\t' && line[0] != ' ') {
                cards.push_back(cur_card);
                in_audio = false;
                cur_card = SoundCard();
            }
        }
    }
    if (in_audio && !cur_card.name.empty()) {
        cards.push_back(cur_card);
    }

    // Fallback: /proc/asound/cards if lspci gave nothing
    if (cards.empty()) {
        auto asound_lines = utils::read_file("/proc/asound/cards");
        for (const auto& l : asound_lines) {
            if (!l.empty() && l[0] == ' ' && std::isdigit(l[1])) {
                SoundCard sc;
                sc.id = "Device-" + std::to_string(dev_id++);
                size_t colon = l.find(" - ");
                if (colon != std::string::npos) {
                    sc.name = utils::trim(l.substr(colon + 3));
                } else {
                    sc.name = utils::trim(l);
                }
                cards.push_back(sc);
            }
        }
    }

    return cards;
}

AudioServer AudioItem::pipewire_status() {
    AudioServer s;
    if (!utils::run_capture("pgrep -x pipewire").empty()) {
        s.name = "PipeWire";
        s.version = utils::program_version_cmd("pipewire");
        s.status = "active";
    }
    return s;
}

AudioServer AudioItem::pulse_status() {
    AudioServer s;
    if (!utils::run_capture("pgrep -x pulseaudio").empty()) {
        s.name = "PulseAudio";
        s.version = utils::program_version_cmd("pulseaudio");
        s.status = "active";
    }
    return s;
}

AudioServer AudioItem::jack_status() {
    AudioServer s;
    if (!utils::run_capture("pgrep -x jackd").empty()) {
        s.name = "JACK";
        s.status = "active";
    }
    return s;
}

std::vector<OutputRow> AudioItem::device_output(const std::vector<SoundCard>& cards) {
    std::vector<OutputRow> rows;
    bool is_first = true;
    for (const auto& c : cards) {
        std::vector<Field> fields;
        fields.push_back(field(c.id, c.name));
        if (!c.driver.empty()) {
            fields.push_back(field("driver", c.driver));
        }
        if (is_first) {
            rows.push_back(make_row("Audio", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> AudioItem::asound_output() { return {}; }
std::vector<OutputRow> AudioItem::usb_output(const std::vector<SoundCard>& cards) { return {}; }
std::string AudioItem::sound_helpers(const std::string& key, const std::string& val) { return val; }
void AudioItem::sound_tools() {}

std::vector<OutputRow> AudioItem::sound_output(const std::vector<SoundCard>& cards, const AudioServer& server) {
    auto rows = device_output(cards);
    if (!server.name.empty()) {
        std::vector<Field> sf;
        sf.push_back(field("Server-1", server.name));
        if (!server.version.empty()) sf.push_back(field("v", server.version));
        if (!server.status.empty()) sf.push_back(field("status", server.status));
        if (rows.empty()) {
            rows.push_back(make_row("Audio", sf));
        } else {
            rows.push_back(make_row("", sf, true));
        }
    }
    return rows;
}

std::vector<OutputRow> AudioItem::get() {
    auto cards = sound_data();
    AudioServer s = pipewire_status();
    if (s.name.empty()) s = pulse_status();
    if (s.name.empty()) s = jack_status();
    return sound_output(cards, s);
}

}  // namespace inxi
