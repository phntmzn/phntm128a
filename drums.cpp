// drums.cpp
// Compile: g++ drums.cpp -o drums
// Run: ./drums
// Output: drums.mid

#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>

using namespace std;

const int TPQ = 480;
const int BPM = 140;
const int BARS = 64;

const int DRUM_CHANNEL = 9;

const int KICK = 36;
const int SNARE = 38;
const int CLAP = 39;
const int CLOSED_HAT = 42;
const int OPEN_HAT = 46;
const int TRIANGLE = 81;

struct Event {
    uint32_t tick;
    int note;
    int velocity;
    int duration;
};

void writeByte(vector<uint8_t>& data, uint8_t b) {
    data.push_back(b);
}

void writeVarLen(vector<uint8_t>& data, uint32_t value) {
    uint8_t buffer[5];
    int count = 0;

    buffer[count++] = value & 0x7F;
    while ((value >>= 7)) {
        buffer[count++] = 0x80 | (value & 0x7F);
    }

    for (int i = count - 1; i >= 0; --i) {
        data.push_back(buffer[i]);
    }
}

void write32(ofstream& file, uint32_t value) {
    file.put((value >> 24) & 0xFF);
    file.put((value >> 16) & 0xFF);
    file.put((value >> 8) & 0xFF);
    file.put(value & 0xFF);
}

void write16(ofstream& file, uint16_t value) {
    file.put((value >> 8) & 0xFF);
    file.put(value & 0xFF);
}

int main() {
    vector<uint8_t> track;
    vector<Event> events;

    int microsecondsPerQuarter = 60000000 / BPM;

    // Tempo meta event
    writeVarLen(track, 0);
    writeByte(track, 0xFF);
    writeByte(track, 0x51);
    writeByte(track, 0x03);
    writeByte(track, (microsecondsPerQuarter >> 16) & 0xFF);
    writeByte(track, (microsecondsPerQuarter >> 8) & 0xFF);
    writeByte(track, microsecondsPerQuarter & 0xFF);

    for (int bar = 0; bar < BARS; bar++) {
        uint32_t barStart = bar * 4 * TPQ;

        // Kick on beats 1 and 3
        events.push_back({barStart + 0 * TPQ, KICK, 120, 120});
        events.push_back({barStart + 2 * TPQ, KICK, 115, 120});

        // Snare and clap on beat 3
        events.push_back({barStart + 2 * TPQ, SNARE, 110, 120});
        events.push_back({barStart + 2 * TPQ, CLAP, 105, 120});

        // Closed hi-hats every 8th note
        for (int eighth = 0; eighth < 8; eighth++) {
            uint32_t tick = barStart + eighth * (TPQ / 2);
            events.push_back({tick, CLOSED_HAT, 80, 60});
        }

        // Open hat on beat 4&
        events.push_back({barStart + 3 * TPQ + TPQ / 2, OPEN_HAT, 95, 180});

        // Triangle on beat 2& and 4&
        events.push_back({barStart + 1 * TPQ + TPQ / 2, TRIANGLE, 70, 240});
        events.push_back({barStart + 3 * TPQ + TPQ / 2, TRIANGLE, 70, 240});
    }

    sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        return a.tick < b.tick;
    });

    uint32_t currentTick = 0;

    for (const auto& e : events) {
        writeVarLen(track, e.tick - currentTick);

        writeByte(track, 0x90 | DRUM_CHANNEL);
        writeByte(track, e.note);
        writeByte(track, e.velocity);

        writeVarLen(track, e.duration);

        writeByte(track, 0x80 | DRUM_CHANNEL);
        writeByte(track, e.note);
        writeByte(track, 0);

        currentTick = e.tick + e.duration;
    }

    // End of track
    writeVarLen(track, 0);
    writeByte(track, 0xFF);
    writeByte(track, 0x2F);
    writeByte(track, 0x00);

    ofstream file("drums.mid", ios::binary);

    file.write("MThd", 4);
    write32(file, 6);
    write16(file, 0);
    write16(file, 1);
    write16(file, TPQ);

    file.write("MTrk", 4);
    write32(file, track.size());
    file.write((char*)track.data(), track.size());

    file.close();

    return 0;
}
