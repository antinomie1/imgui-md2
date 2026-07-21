#include <imgui_md2/animation.h>

#include <algorithm>
#include <cmath>

namespace ImGuiMD2 {
namespace {

float Cubic(float a, float b, float c, float t) {
    const float inv = 1.0f - t;
    return 3.0f * inv * inv * t * a + 3.0f * inv * t * t * b + t * t * t * c;
}

float CubicDerivative(float a, float b, float c, float t) {
    const float inv = 1.0f - t;
    return 3.0f * inv * inv * a + 6.0f * inv * t * (b - a) +
           3.0f * t * t * (c - b);
}

float CubicBezier(float x1, float y1, float x2, float y2, float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    float t = progress;
    for (int i = 0; i < 6; ++i) {
        const float x = Cubic(x1, x2, 1.0f, t) - progress;
        const float slope = CubicDerivative(x1, x2, 1.0f, t);
        if (std::abs(slope) < 1.0e-5f) break;
        t = std::clamp(t - x / slope, 0.0f, 1.0f);
    }
    float low = 0.0f;
    float high = 1.0f;
    for (int i = 0; i < 8; ++i) {
        const float x = Cubic(x1, x2, 1.0f, t);
        if (std::abs(x - progress) < 1.0e-5f) break;
        if (x < progress)
            low = t;
        else
            high = t;
        t = (low + high) * 0.5f;
    }
    return Cubic(y1, y2, 1.0f, t);
}

bool Different(ImVec2 a, ImVec2 b) {
    return std::abs(a.x - b.x) > 1.0e-5f || std::abs(a.y - b.y) > 1.0e-5f;
}

} // namespace

float Ease(Easing easing, float progress) {
    switch (easing) {
    case Easing::Linear:
        return std::clamp(progress, 0.0f, 1.0f);
    case Easing::Deceleration:
        return CubicBezier(0.0f, 0.0f, 0.2f, 1.0f, progress);
    case Easing::Acceleration:
        return CubicBezier(0.4f, 0.0f, 1.0f, 1.0f, progress);
    case Easing::Sharp:
        return CubicBezier(0.4f, 0.0f, 0.6f, 1.0f, progress);
    case Easing::Standard:
    default:
        return CubicBezier(0.4f, 0.0f, 0.2f, 1.0f, progress);
    }
}

void Animator::NewFrame(float delta_time, int frame_index) {
    delta_time_ = std::clamp(delta_time, 0.0f, 0.1f);
    frame_index_ = frame_index;
    if ((frame_index_ % 120) == 0) {
        for (auto it = tracks_.begin(); it != tracks_.end();) {
            if (frame_index_ - it->second.touched_frame > 600)
                it = tracks_.erase(it);
            else
                ++it;
        }
    }
}

Animator::Track& Animator::Retarget(ImGuiID id, ImVec2 target, int dimensions,
                                    float duration, Easing easing) {
    Track& track = tracks_[id];
    if (!track.initialized) {
        track.value = track.from = track.target = target;
        track.duration = duration;
        track.easing = easing;
        track.dimensions = dimensions;
        track.initialized = true;
    } else if (Different(track.target, target) || track.dimensions != dimensions) {
        track.from = track.value;
        track.target = target;
        track.elapsed = 0.0f;
        track.duration = duration;
        track.easing = easing;
        track.dimensions = dimensions;
    }

    if (track.touched_frame != frame_index_) {
        track.elapsed += delta_time_;
        const float progress = track.duration <= 0.0f
                                   ? 1.0f
                                   : std::min(track.elapsed / track.duration, 1.0f);
        const float eased = Ease(track.easing, progress);
        track.value.x = track.from.x + (track.target.x - track.from.x) * eased;
        track.value.y = track.from.y + (track.target.y - track.from.y) * eased;
        if (progress >= 1.0f) track.value = track.target;
        track.touched_frame = frame_index_;
    }
    return track;
}

float Animator::Animate(ImGuiID id, float target, float duration, Easing easing) {
    return Retarget(id, ImVec2(target, 0.0f), 1, duration, easing).value.x;
}

ImVec2 Animator::Animate(ImGuiID id, ImVec2 target, float duration, Easing easing) {
    return Retarget(id, target, 2, duration, easing).value;
}

void Animator::Snap(ImGuiID id, float value) {
    Track& track = tracks_[id];
    track.value = track.from = track.target = ImVec2(value, 0.0f);
    track.elapsed = track.duration = 0.0f;
    track.dimensions = 1;
    track.touched_frame = frame_index_;
    track.initialized = true;
}

void Animator::Snap(ImGuiID id, ImVec2 value) {
    Track& track = tracks_[id];
    track.value = track.from = track.target = value;
    track.elapsed = track.duration = 0.0f;
    track.dimensions = 2;
    track.touched_frame = frame_index_;
    track.initialized = true;
}

void Animator::Remove(ImGuiID id) { tracks_.erase(id); }
void Animator::Clear() { tracks_.clear(); }

} // namespace ImGuiMD2
