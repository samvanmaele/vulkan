#include "midi.hpp"
#include "midifile/MidiFile.h"
#include <array>
#include <glm/ext/vector_float4.hpp>
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
static const std::unordered_map<int, int> targetMap
{
    // Foot Targets
    {44, -2}, // Hi-Hat pedal
    {35, -1}, // Acoustic Bass Drum
    {36, -1}, // Bass Drum 1

    // Hand Targets (0-6)
    {42, 0}, // Closed Hi-Hat
    {46, 0}, // Open Hi-Hat

    {49, 1}, // Crash Cymbal 1
    {57, 1}, // Crash Cymbal 2

    {51, 2}, // Ride Cymbal 1
    {53, 2}, // Ride Bell
    {59, 2}, // Ride Cymbal 2

    {38, 3}, // Acoustic Snare
    {40, 3}, // Electric Snare

    {50, 4}, // High Tom

    {47, 5}, // Low-Mid Tom
    {48, 5}, // Hi-Mid Tom

    {41, 6}, // Low Floor Tom
    {43, 6}  // High Floor Tom
};

MidiManager::MidiManager(const std::string & filename)
{
    smf::MidiFile midifile;
    midifile.read(filename);
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();

    drumTrack = midifile[10];
    for (int event=0; event < drumTrack.size(); event++)
    {
        const auto& mevent = drumTrack[event];
        if (mevent.isNoteOn())
        {
            if (mevent.isLinked())
            {
                DrumHit hit;
                hit.startTime = mevent.seconds;
                hit.endTime = mevent.seconds + mevent.getDurationInSeconds();
                hit.note = mevent[1];
                hit.velocity = mevent[2];
                hits.push_back(hit);
            }
        }
    }
}
glm::vec4 MidiManager::getEvent(float time)
{
    glm::vec4 drumPosition = glm::vec4(-1.0f);
    std::fill(states.begin(), states.end(), -1.0f);

    for (size_t i = nextHitIndex; i < hits.size(); ++i)
    {
        const auto& hit = hits[i];
        const float activation = (float)hit.velocity / 127.0f;
        const int target = getTargetIndex(hit.note);

        if (time < hit.startTime) break;
        if (time >= hit.endTime)
        {
            nextHitIndex = i + 1;
            continue;
        }

        if (target == -2) drumPosition.y = activation;
        if (target == -1) drumPosition.x = activation;
        if (target >= 0)
        {
            int slot = 2 * target;
            if (states[slot] < 0.0f) states[slot] = activation;
            else states[slot + 1] = activation;
        }
    }

    lArm = -1.0f;
    rArm = -1.0f;
    for (int target : {2, 6, 4, 5, 0, 3, 1})
    {
        bool leftPriority = (target == 3 || target == 1);
        assignMainSlots(target, leftPriority);
    }
    for (int target : {2, 6, 4, 5, 0, 3, 1})
    {
        bool leftPriority = (target == 3 || target == 1);
        assignSecondarySlots(target, leftPriority);
    }

    drumPosition.z = lArm;
    drumPosition.w = rArm;

    return drumPosition;
}
int MidiManager::getTargetIndex(int note)
{
    auto it = targetMap.find(note);

    if (it != targetMap.end())
    {
        return it->second;
    }
    else
    {
        return -3;
    }
}
void MidiManager::assignMainSlots(int target, bool leftPriority)
{
    int slot1 = 2 * target;
    if (states[slot1] > 0.0f)
    {
        if (leftPriority)
        {
            if (isLarmFree()) assignLimb(lArm, target, slot1);
            else if (isRarmFree()) assignLimb(rArm, target, slot1);
        }
        else
        {
            if (isRarmFree()) assignLimb(rArm, target, slot1);
            else if (isLarmFree()) assignLimb(lArm, target, slot1);
        }
    }
}
void MidiManager::assignSecondarySlots(int target, bool leftPriority)
{
    int slot2 = 2 * target + 1;
    if (states[slot2] > 0.0f)
    {
        if (isLarmFree()) assignLimb(lArm, target, slot2);
        else if (isRarmFree()) assignLimb(rArm, target, slot2);
    }
}
bool MidiManager::isLarmFree()
{
    return lArm < 0.0f;
}
bool MidiManager::isRarmFree()
{
    return rArm < 0.0f;
}
void MidiManager::assignLimb(float &armValue, int targetIndex, int slot)
{
    armValue = (float)targetIndex + states[slot] * 0.1f;
}