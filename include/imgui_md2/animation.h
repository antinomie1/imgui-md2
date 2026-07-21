#pragma once

#include <imgui.h>

#include <cstdint>
#include <unordered_map>

namespace ImGuiMD2 {

enum class Easing : std::uint8_t {
    Linear,
    Standard,      // cubic-bezier(0.4, 0, 0.2, 1)
    Deceleration,  // cubic-bezier(0, 0, 0.2, 1)
    Acceleration,  // cubic-bezier(0.4, 0, 1, 1)
    Sharp           // cubic-bezier(0.4, 0, 0.6, 1)
};

float Ease(Easing easing, float progress);

class Animator {
public:
    void NewFrame(float delta_time, int frame_index);

    // Retargetable animation. The first call starts at `target`; call Snap()
    // first when a different initial value is required.
    float Animate(ImGuiID id, float target, float duration,
                  Easing easing = Easing::Standard);
    ImVec2 Animate(ImGuiID id, ImVec2 target, float duration,
                   Easing easing = Easing::Standard);
    void Snap(ImGuiID id, float value);
    void Snap(ImGuiID id, ImVec2 value);
    void Remove(ImGuiID id);
    void Clear();

private:
    struct Track {
        ImVec2 value{};
        ImVec2 from{};
        ImVec2 target{};
        float elapsed = 0.0f;
        float duration = 0.0f;
        Easing easing = Easing::Standard;
        int dimensions = 1;
        int touched_frame = 0;
        bool initialized = false;
    };

    Track& Retarget(ImGuiID id, ImVec2 target, int dimensions, float duration,
                    Easing easing);
    std::unordered_map<ImGuiID, Track> tracks_;
    float delta_time_ = 1.0f / 60.0f;
    int frame_index_ = 0;
};

} // namespace ImGuiMD2
