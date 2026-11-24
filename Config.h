#pragma once
// Tunable constants (README: Config.h). Units: meters, Y up, hallway along -Z.
namespace cfg {
constexpr float HALL_W = 3.f, HALL_H = 2.8f, HALL_L = 24.f;
constexpr float START_Z = -20.f, END_Z = -1.0f;      // character path
constexpr float MARK_PROGRESS = 0.8f;                // mark position (0..1)
constexpr float HIT_WINDOW = 0.03f;                  // +/- progress that counts as "on mark"
constexpr float STEP_LEN = 0.04f, STEP_JITTER = 0.008f;
constexpr float STEP_MIN = 0.55f, STEP_MAX = 1.2f;   // seconds between steps
constexpr float FOV = 70.f, FOG_DENSITY = 0.10f;
}
