// drums.cpp
// Compile: g++ drums.cpp -o drums
// Run: ./drums
// Output: drums.mid

#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

const int TPQ = 480;        // ticks per quarter note
const int BPM = 140;
const int BARS = 64;

const int DRUM_CHANNEL = 9; // MIDI channel 10 = index 9
const int HIHAT = 42;       // closed hi-hat
const int SNARE = 38;

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

void addNote(vector<uint8_t>& track, uint32_t delta, int note, int velocity, int duration) {
    writeVarLen(track, delta);
    writeByte(track, 0x90 | DRUM_CHANNEL);
    writeByte(track, note);
    writeByte(track, velocity);

    writeVarLen(track, duration);
    writeByte(track, 0x80 | DRUM_CHANNEL);
    writeByte(track, note);
    writeByte(track, 0);
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

    // Tempo meta event
    int microsecondsPerQuarter = 60000000 / BPM;
    writeVarLen(track, 0);
    writeByte(track, 0xFF);
    writeByte(track, 0x51);
    writeByte(track, 0x03);
    writeByte(track, (microsecondsPerQuarter >> 16) & 0xFF);
    writeByte(track, (microsecondsPerQuarter >> 8) & 0xFF);
    writeByte(track, microsecondsPerQuarter & 0xFF);

    uint32_t currentTick = 0;

    for (int bar = 0; bar < BARS; bar++) {
        uint32_t barStart = bar * 4 * TPQ;

        for (int eighth = 0; eighth < 8; eighth++) {
            uint32_t tick = barStart + eighth * (TPQ / 2);
            addNote(track, tick - currentTick, HIHAT, 80, 60);
            currentTick = tick + 60;
        }

        uint32_t snareTick = barStart + 2 * TPQ; // beat 3
        addNote(track, snareTick - currentTick, SNARE, 110, 120);
        currentTick = snareTick + 120;
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
