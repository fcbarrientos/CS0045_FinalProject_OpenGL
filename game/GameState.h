#pragma once
// Person 2: gameplay logic. Plain C++, no GL/GLUT includes.
#include <cmath>
#include <cstdlib>
#include "../Config.h"

enum GameStateId { ST_PLAYING, ST_WIN, ST_LOSE };

static float randRange(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

struct Game {
    int state = ST_PLAYING;

    // Ghost approach & 60s countdown
    float elapsed = 0.f;           // 0..60 seconds
    float ghostProgress = 0.f;     // 0 (far at START_Z) -> 1 (reaches player at END_Z) at exactly 60s
    float stepTimer = 0.f;

    // Horizontal tracking bar (0..1)
    float greenPos = 0.5f;         // center of the moving green zone
    float greenVel = cfg::GREEN_SPEED;
    float greenTargetTimer = 0.f;

    float linePos = 0.15f;         // player's interactive line position (0..1)
    float lineVel = 0.f;
    bool holdingMouse = false;     // true while left mouse button is held down

    // Charge / Catch progress bar (0..1 -> 100% = WIN)
    float catchProgress = 0.f;
    bool inGreenZone = false;

    // Events for visual/audio feedback in main.cpp
    bool evStep = false, evWin = false, evLose = false;

    static float progressToPosition(float p) { return cfg::START_Z + p * (cfg::END_Z - cfg::START_Z); }

    // 0..1 intensity: rises as ghost gets closer or when player is actively charging
    float tensionPulse() const {
        return ghostProgress;
    }

    void reset() {
        state = ST_PLAYING;
        elapsed = 0.f;
        ghostProgress = 0.f;
        stepTimer = 0.6f;

        greenPos = 0.5f;
        greenVel = cfg::GREEN_SPEED;
        greenTargetTimer = 1.2f;

        linePos = 0.15f;
        lineVel = 0.f;
        holdingMouse = false;

        catchProgress = 0.f;
        inGreenZone = false;
        evStep = evWin = evLose = false;
    }

    void setMouseHold(bool down) {
        holdingMouse = down;
    }

    void update(float dt) {
        if (state != ST_PLAYING) return;

        // 1. Timer & Ghost approach: reaches ghostProgress = 1.0 at exactly cfg::GAME_DURATION (60s)
        elapsed += dt;
        if (elapsed >= cfg::GAME_DURATION) {
            elapsed = cfg::GAME_DURATION;
            ghostProgress = 1.f;
            state = ST_LOSE;
            evLose = true;
            return;
        }
        ghostProgress = elapsed / cfg::GAME_DURATION;

        // Periodic footstep pulse for head-bob and light dip
        stepTimer -= dt;
        if (stepTimer <= 0.f) {
            stepTimer = 0.75f - 0.35f * ghostProgress; // steps get faster as ghost nears
            evStep = true;
        }

        // 2. Moving Green Mark (moves horizontally along 0..1 bar, bouncing & changing pace)
        float halfW = cfg::GREEN_WIDTH * 0.5f;
        greenTargetTimer -= dt;
        if (greenTargetTimer <= 0.f) {
            float dir = (rand() % 2 == 0) ? 1.f : -1.f;
            float speedScale = randRange(0.75f, 1.35f);
            greenVel = dir * cfg::GREEN_SPEED * speedScale;
            greenTargetTimer = randRange(0.8f, 2.0f);
        }
        greenPos += greenVel * dt;
        if (greenPos - halfW < 0.f) {
            greenPos = halfW;
            greenVel = fabsf(greenVel);
        } else if (greenPos + halfW > 1.f) {
            greenPos = 1.f - halfW;
            greenVel = -fabsf(greenVel);
        }

        // 3. Interactive Line: hold left mouse -> moves right; release -> moves left
        if (holdingMouse) {
            lineVel += cfg::LINE_ACCEL_R * dt;
        } else {
            lineVel -= cfg::LINE_ACCEL_L * dt;
        }
        if (lineVel > cfg::LINE_MAX_VEL) lineVel = cfg::LINE_MAX_VEL;
        if (lineVel < -cfg::LINE_MAX_VEL) lineVel = -cfg::LINE_MAX_VEL;

        linePos += lineVel * dt;
        if (linePos < 0.f) {
            linePos = 0.f;
            if (lineVel < 0.f) lineVel = 0.f;
        } else if (linePos > 1.f) {
            linePos = 1.f;
            if (lineVel > 0.f) lineVel = 0.f;
        }

        // 4. Progress Bar: fills when interactive line is inside the green mark
        inGreenZone = (fabsf(linePos - greenPos) <= halfW);
        if (inGreenZone) {
            catchProgress += cfg::FILL_RATE * dt;
        } else {
            catchProgress -= cfg::DRAIN_RATE * dt;
        }
        if (catchProgress < 0.f) catchProgress = 0.f;

        if (catchProgress >= 1.f) {
            catchProgress = 1.f;
            state = ST_WIN;
            evWin = true;
        }
    }
};
