#include "midi.hpp"
#include "midifile/MidiFile.h"
#include <iostream>
#include <iomanip>
#include <unordered_map>

std::unordered_map<int, std::string> gmDrumMap
{
    {35, "Acoustic Bass Drum"},
    {36, "Bass Drum 1"},
    {37, "Side Stick"},
    {38, "Acoustic Snare"},
    {39, "Hand Clap"},
    {40, "Electric Snare"},
    {41, "Low Floor Tom"},
    {42, "Closed Hi-Hat"},
    {43, "High Floor Tom"},
    {44, "Pedal Hi-Hat"},
    {45, "Low Tom"},
    {46, "Open Hi-Hat"},
    {47, "Low-Mid Tom"},
    {48, "Hi-Mid Tom"},
    {49, "Crash Cymbal 1"},
    {50, "High Tom"},
    {51, "Ride Cymbal 1"},
    {52, "Chinese Cymbal"},
    {53, "Ride Bell"},
    {54, "Tambourine"},
    {55, "Splash Cymbal"},
    {56, "Cowbell"},
    {57, "Crash Cymbal 2"},
    {58, "Vibraslap"},
    {59, "Ride Cymbal 2"},
    {60, "Hi Bongo"},
    {61, "Low Bongo"},
    {62, "Mute Hi Conga"},
    {63, "Open Hi Conga"},
    {64, "Low Conga"},
    {65, "High Timbale"},
    {66, "Low Timbale"},
    {67, "High Agogo"},
    {68, "Low Agogo"},
    {69, "Cabasa"},
    {70, "Maracas"},
    {71, "Short Whistle"},
    {72, "Long Whistle"},
    {73, "Short Guiro"},
    {74, "Long Guiro"},
    {75, "Claves"},
    {76, "Hi Wood Block"},
    {77, "Low Wood Block"},
    {78, "Mute Cuica"},
    {79, "Open Cuica"},
    {80, "Mute Triangle"},
    {81, "Open Triangle"}
};

void midiManager::midi()
{
    smf::MidiFile midifile;
    midifile.read("sfx/blur-song_2.mid");
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();

    std::cout << "TPQ: " << midifile.getTicksPerQuarterNote() << std::endl;
    std::cout << "TRACKS: " << midifile.getTrackCount() << std::endl;

    std::cout << std::left << std::setw(7) << "TICK" << std::setw(8) << "DELTA" << std::setw(8) << "DUR" << "| " << std::setw(10) << "VELOCITY" << std::setw(8) << "NOTE" << std::endl;
    std::cout << "___________________________________________________________\n";

    int track = 10;
    for (int event=0; event<midifile[track].size() / 10; event++)
    {
        const auto& mevent = midifile[track][event];
        if (mevent.isNoteOn())
        {
            int note = mevent[1];
            int velocity = mevent[2];

            std::cout << std::left << std::setw(7)  << mevent.tick << std::setw(8) << std::fixed << std::setprecision(3) << mevent.seconds << std::setw(8) << std::fixed << std::setprecision(3) << midifile[track][event].getDurationInSeconds() << "| ";
            std::cout << std::setw(10) << velocity;

            if (auto it = gmDrumMap.find(note); it != gmDrumMap.end())
            {
                std::cout << std::setw(8) << gmDrumMap[note];
            }

            std::cout << std::dec << std::setfill(' ') << std::endl;
        }
    }
}