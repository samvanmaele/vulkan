#pragma once
#include <glm/ext/vector_float4.hpp>
#include <midifile/MidiEventList.h>
#include <string>

struct DrumHit {
    float startTime;
    float endTime;
    int note;
    int velocity;
};

class MidiManager
{
    public:
        smf::MidiEventList drumTrack;

        MidiManager(const std::string & filename);
        glm::vec4 getEvent(float time);

    private:
        std::vector<DrumHit> hits;
        size_t nextHitIndex = 0;
        std::array<float, 14> states;
        float lArm = -1.0f;
        float rArm = -1.0f;

        int getTargetIndex(int note);
        void assignMainSlots(int target, bool leftPriority);
        void assignSecondarySlots(int target, bool leftPriority);
        bool isLarmFree();
        bool isRarmFree();
        void assignLimb(float &armValue, int targetIndex, int slot);
};