#pragma once
// Person 2: gameplay logic. Plain C++, no GL/GLUT includes.
#include <cmath>
#include <cstdlib>
#include "../Config.h"

enum GameStateId { ST_WAITING, ST_MOVING, ST_ON_MARK, ST_PAST_MARK, ST_WIN, ST_LOSE };

static float randRange(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

struct Game {
    int state = ST_WAITING;
    float progress = 0, shown = 0, stateTime = 0, stepTimer = 0;
    int steps = 0;
    bool earlyClick = false;
    bool evStep = false, evWin = false, evLose = false;   // events: main.cpp reads, then clears

    static float progressToPosition(float p) { return cfg::START_Z + p * (cfg::END_Z - cfg::START_Z); }
    float markZ() const { return progressToPosition(cfg::MARK_PROGRESS); }

    // 0..1: how "hot" the mark is (1 while the character stands on it)
    float markPulse() const {
        if (state == ST_ON_MARK) return 1.f;
        float p = 1.f - fabsf(shown - cfg::MARK_PROGRESS) / 0.4f;
        if (p < 0) p = 0;
        return p * 0.6f;
    }

    void setState(int s) { state = s; stateTime = 0; }
    void reset() {
        state = ST_WAITING; progress = shown = stateTime = 0; steps = 0; earlyClick = false;
        evStep = evWin = evLose = false;
    }

    void click() {                                        // mouse = action
        if (state == ST_ON_MARK) { setState(ST_WIN); evWin = true; }
        else if (state == ST_WAITING || state == ST_MOVING) { earlyClick = true; setState(ST_LOSE); evLose = true; }
    }

    void update(float dt) {
        stateTime += dt;
        if (state == ST_WAITING) {
            if (stateTime > 2.f) { setState(ST_MOVING); stepTimer = randRange(cfg::STEP_MIN, cfg::STEP_MAX); }
        } else if (state == ST_MOVING || state == ST_ON_MARK) {
            stepTimer -= dt;
            if (stepTimer <= 0) {
                progress += cfg::STEP_LEN + randRange(-cfg::STEP_JITTER, cfg::STEP_JITTER);
                stepTimer = randRange(cfg::STEP_MIN, cfg::STEP_MAX);
                steps++; evStep = true;
                float d = progress - cfg::MARK_PROGRESS;
                if (d > cfg::HIT_WINDOW) setState(ST_PAST_MARK);
                else if (d >= -cfg::HIT_WINDOW) setState(ST_ON_MARK);
            }
        } else if (state == ST_PAST_MARK) {
            if (stateTime > 0.5f) { setState(ST_LOSE); evLose = true; }
        }
        float k = dt * 14.f; if (k > 1) k = 1;
        shown += (progress - shown) * k;                  // smooth step motion
    }
};