#pragma once
// Tunable constants (README: Config.h). Units: meters, Y up, hallway along -Z.
namespace cfg {
constexpr float HALL_W = 3.f, HALL_H = 2.8f, HALL_L = 24.f;
constexpr float START_Z = -20.f, END_Z = -1.0f;      // ghost approach path

// 60-second survival & fishing-style tracking bar mechanics
constexpr float GAME_DURATION = 60.f;                // ghost reaches player at exactly 60.0s
constexpr float GREEN_WIDTH   = 0.22f;               // width of the moving green zone (0..1)
constexpr float GREEN_SPEED   = 0.42f;               // base horizontal speed of green zone
constexpr float LINE_ACCEL_R  = 2.6f;                // acceleration right when holding left mouse
constexpr float LINE_ACCEL_L  = 2.2f;                // drift/gravity left when releasing left mouse
constexpr float LINE_MAX_VEL  = 1.15f;               // max velocity of interactive line
constexpr float FILL_RATE     = 0.14f;               // progress fill rate per second inside green zone
constexpr float DRAIN_RATE    = 0.04f;               // slight progress drain per second outside green zone

constexpr float FOV = 70.f, FOG_DENSITY = 0.10f;
}
