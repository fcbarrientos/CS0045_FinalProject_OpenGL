// Hallway Foxy Ward: Hold left mouse button to move the interactive line right,
// release to move left. Keep the line inside the moving green mark to fill the
// progress bar to 100% before Foxy reaches you at 60 seconds!
#include <GL/glew.h>
#include <GL/glut.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "Config.h"
#include "engine/Mesh.h"
#include "game/GameState.h"

// ---------------- globals ----------------
Game game;
Mesh floorMesh, wallMesh;                       // VBO-backed hallway
int winW = 1100, winH = 700, lastMs = 0;
float now = 0, dip = 0, flick = 1, blackout = 1, flash = 0, flashR = 1, flashG = 1, flashB = 1, bob = 0;
float darkT = 0.f, darkNext = 4.f;                   // failing-light stretches
Mix_Chunk* scream = nullptr;                         // jumpscare audio
Mix_Chunk* suspense = nullptr, * flicker = nullptr;  // ambience loops
bool flickerMuted = false;
bool spaceDown = false, mouseDown = false;           // the bar can be driven by either
enum { CH_SUSPENSE = 0, CH_FLICKER = 1 };            // reserved mixer channels
void startAmbience();                                // defined with the audio helpers below
float loseTime = 0.f;
bool freezeScare = false;                       // DEBUG: J holds the jumpscare pose for inspection
bool impactHit = false;                         // one-shot: the leap has landed
bool audioCut = false;                          // one-shot: the scream is being faded out
const int NUM_DUST = 150;
float dust[NUM_DUST][3];

float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

// ---------------- scene building ----------------
void buildHallway() {
    const float W = cfg::HALL_W, H = cfg::HALL_H, L = cfg::HALL_L;
    const float fo[3] = {-W / 2, 0, 0}, fu[3] = {W, 0, 0}, fv[3] = {0, 0, -L}, fn[3] = {0, 1, 0};
    floorMesh.addGrid(fo, fu, fv, fn, 12, 96, 3, 24);                     // floor
    const float co[3] = {-W / 2, H, 0}, cn[3] = {0, -1, 0};
    wallMesh.addGrid(co, fu, fv, cn, 12, 96, 3, 24);                      // ceiling
    const float lo[3] = {-W / 2, 0, 0}, lu[3] = {0, 0, -L}, lv[3] = {0, H, 0}, ln[3] = {1, 0, 0};
    wallMesh.addGrid(lo, lu, lv, ln, 96, 12, 12, 1.5f);                   // left wall
    const float ro[3] = {W / 2, 0, 0}, rn[3] = {-1, 0, 0};
    wallMesh.addGrid(ro, lu, lv, rn, 96, 12, 12, 1.5f);                   // right wall
    floorMesh.setTint(0.42f, 0.38f, 0.32f);
    wallMesh.setTint(0.55f, 0.53f, 0.49f);
    floorMesh.upload(); wallMesh.upload();
}

// ---------------- primitives with glBegin/glEnd ----------------
void drawBox() {                                 // unit cube, centered, textured, with normals
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glTexCoord2f(0, 0); glVertex3f(-.5f, -.5f, .5f);  glTexCoord2f(1, 0); glVertex3f(.5f, -.5f, .5f);
                          glTexCoord2f(1, 1); glVertex3f(.5f, .5f, .5f);    glTexCoord2f(0, 1); glVertex3f(-.5f, .5f, .5f);
    glNormal3f(0, 0, -1); glTexCoord2f(0, 0); glVertex3f(.5f, -.5f, -.5f);  glTexCoord2f(1, 0); glVertex3f(-.5f, -.5f, -.5f);
                          glTexCoord2f(1, 1); glVertex3f(-.5f, .5f, -.5f);  glTexCoord2f(0, 1); glVertex3f(.5f, .5f, -.5f);
    glNormal3f(1, 0, 0);  glTexCoord2f(0, 0); glVertex3f(.5f, -.5f, .5f);   glTexCoord2f(1, 0); glVertex3f(.5f, -.5f, -.5f);
                          glTexCoord2f(1, 1); glVertex3f(.5f, .5f, -.5f);   glTexCoord2f(0, 1); glVertex3f(.5f, .5f, .5f);
    glNormal3f(-1, 0, 0); glTexCoord2f(0, 0); glVertex3f(-.5f, -.5f, -.5f); glTexCoord2f(1, 0); glVertex3f(-.5f, -.5f, .5f);
                          glTexCoord2f(1, 1); glVertex3f(-.5f, .5f, .5f);   glTexCoord2f(0, 1); glVertex3f(-.5f, .5f, -.5f);
    glNormal3f(0, 1, 0);  glTexCoord2f(0, 0); glVertex3f(-.5f, .5f, .5f);   glTexCoord2f(1, 0); glVertex3f(.5f, .5f, .5f);
                          glTexCoord2f(1, 1); glVertex3f(.5f, .5f, -.5f);   glTexCoord2f(0, 1); glVertex3f(-.5f, .5f, -.5f);
    glNormal3f(0, -1, 0); glTexCoord2f(0, 0); glVertex3f(-.5f, -.5f, -.5f); glTexCoord2f(1, 0); glVertex3f(.5f, -.5f, -.5f);
                          glTexCoord2f(1, 1); glVertex3f(.5f, -.5f, .5f);   glTexCoord2f(0, 1); glVertex3f(-.5f, -.5f, .5f);
    glEnd();
}

void part(float x, float y, float z, float sx, float sy, float sz) {   // translate + scale a box
    glPushMatrix(); glTranslatef(x, y, z); glScalef(sx, sy, sz); drawBox(); glPopMatrix();
}

void drawChair(float x, float z) {               // prop: GLUT cubes, translated and scaled
    glColor3f(0.3f, 0.18f, 0.09f);
    glPushMatrix(); glTranslatef(x, 0, z);
    glPushMatrix(); glTranslatef(0, 0.45f, 0); glScalef(0.45f, 0.05f, 0.45f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.8f, -0.2f); glScalef(0.45f, 0.7f, 0.05f); glutSolidCube(1); glPopMatrix();
    for (int i = 0; i < 4; ++i) {
        glPushMatrix(); glTranslatef((i & 1 ? 1 : -1) * 0.2f, 0.22f, (i & 2 ? 1 : -1) * 0.2f);
        glScalef(0.05f, 0.45f, 0.05f); glutSolidCube(1); glPopMatrix();
    }
    glPopMatrix();
}

// ---------------- Foxy ----------------
// Local origin = head centre, snout points +Z, feet reach y = -1.23.
// lunge 0->1 drives the jumpscare leap; walk is the gait phase in radians;
// weakened = the player is locked on in the green zone.
void drawFoxy(float z, float s, float lunge, float walk, bool weakened) {
    static bool traced = false;                 // TODO: temporary, remove once confirmed
    if (!traced) { fprintf(stderr, "Foxy scare fired\n"); traced = true; }

    const float open = 6.f + 46.f * lunge;      // jaw angle, degrees
    const float hx   = 1.f + 0.20f * lunge;     // head squash X/Y (kept mild: wide slabs read as rectangles)
    const float hz   = 0.85f + 0.15f * lunge;   // ... Z stays shallow so it can get close
    const float sw   = sinf(walk);              // leg swing; one full stride = two steps

    float hop = sinf(3.14159265f * lunge);      // 0 -> 1 -> 0: he leaves the floor and lands on impact
    float standY = 1.85f - 0.55f * lunge;       // 2.25 m tall in the hall; he ducks into the scare
    glPushMatrix();
    glTranslatef(0.f, standY + 0.18f * hop + 0.03f * fabsf(sw), z);
    glScalef(s, s, s);
    glRotatef(-12.f * lunge, 1, 0, 0);                        // dive: the whole torso pitches at the camera
    glRotatef(sw * 2.0f, 0, 0, 1);                            // weight shifts onto the planted foot
    glRotatef(2.5f * sinf(now * 9.f) * (0.25f + lunge), 0, 1, 0);

    // ---- legs: articulated hip + knee, so the walk reads ----
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 0.14f, -0.95f, 0.f);                 // hip
        glRotatef(i * sw * 24.f, 1, 0, 0);                    // legs in opposition
        glColor3f(0.10f, 0.10f, 0.14f);                       // tattered trousers
        part(0.f, -0.21f, 0.f, 0.17f, 0.42f, 0.17f);          // thigh
        glTranslatef(0.f, -0.42f, 0.f);
        glRotatef(-fabsf(sw) * 14.f, 1, 0, 0);                // knee folds backwards only
        part(0.f, -0.21f, 0.f, 0.15f, 0.42f, 0.15f);          // shin
        glColor3f(0.08f, 0.08f, 0.10f);
        part(0.f, -0.45f, 0.03f, 0.16f, 0.06f, 0.22f);        // foot, sole lands on y = 0
        glPopMatrix();
    }

    glColor3f(0.60f, 0.19f, 0.11f);                          // neck + torso
    part(0.f, -0.24f, 0.f, 0.18f, 0.16f, 0.18f);
    part(0.f, -0.62f, 0.f, 0.58f, 0.70f, 0.32f);
    glColor3f(0.74f, 0.58f, 0.44f);                          // torn belly panel
    part(0.f, -0.60f, 0.18f, 0.26f, 0.34f, 0.05f);

    // ---- arms: pitched forward so they frame the face, left one ends in the hook ----
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 0.34f, -0.40f, 0.f);                // shoulder, on a wider torso
        glRotatef(i * (8.f + 14.f * lunge), 0, 0, 1);        // spread
        glRotatef(-25.f - 45.f * lunge - i * sw * 16.f, 1, 0, 0);  // counter-swing to the legs; thrown forward on impact
        glColor3f(0.60f, 0.19f, 0.11f);
        part(0.f, -0.29f, 0.f, 0.14f, 0.56f, 0.14f);         // upper arm
        part(0.f, -0.62f, 0.02f, 0.12f, 0.16f, 0.12f);       // forearm
        if (i < 0) {                                         // hook: three boxes = a J
            glColor3f(0.55f, 0.56f, 0.60f);
            part(0.f, -0.73f, 0.02f, 0.035f, 0.16f, 0.035f);
            part(0.f, -0.81f, 0.09f, 0.035f, 0.035f, 0.16f);
            part(0.f, -0.75f, 0.17f, 0.035f, 0.12f, 0.035f);
        } else {                                             // hand
            glColor3f(0.10f, 0.10f, 0.14f);
            part(0.f, -0.71f, 0.02f, 0.12f, 0.12f, 0.12f);
        }
        glPopMatrix();
    }

    // ---- head ----
    glPushMatrix();
    glTranslatef(0.f, 0.f, 0.08f * lunge);                   // neck snaps the head ahead of the body
    glRotatef(8.f * lunge, 0, 1, 0);                         // near-frontal: he looks straight down the lens
    glRotatef(2.f * lunge, 1, 0, 0);
    glRotatef(sw * 2.5f, 0, 0, 1);                           // head rocks with each stride
    glScalef(hx, hx, hz);

    glColor3f(0.66f, 0.21f, 0.13f);                          // skull: narrow and tall, not a lizard block
    part(0.f, 0.f, 0.f, 0.38f, 0.40f, 0.34f);
    part(0.f, -0.055f, 0.26f, 0.17f, 0.15f, 0.24f);          // muzzle
    part(0.f, -0.06f, 0.38f, 0.12f, 0.10f, 0.10f);           // tapered front: a snout, not a beam

    glColor3f(0.60f, 0.18f, 0.11f);                          // cheek tufts
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 0.115f, -0.02f, 0.19f);
        glRotatef(i * 18.f, 0, 1, 0);
        part(0.f, 0.f, 0.f, 0.09f, 0.14f, 0.16f);
        glPopMatrix();
    }

    glColor3f(0.03f, 0.02f, 0.02f);                          // sunken eye socket
    part(-0.085f, 0.065f, 0.175f, 0.13f, 0.11f, 0.02f);

    glColor3f(0.48f, 0.14f, 0.09f);                          // brow ridge arches over the eye
    glPushMatrix();
    glTranslatef(-0.085f, 0.155f, 0.185f);
    glRotatef(-10.f, 0, 0, 1);
    part(0.f, 0.f, 0.f, 0.17f, 0.045f, 0.06f);
    glPopMatrix();

    glColor3f(0.20f, 0.03f, 0.03f);                          // throat: dark cavity behind the teeth
    part(0.f, -0.19f, 0.14f, 0.14f, 0.09f, 0.20f);

    glColor3f(0.90f, 0.88f, 0.80f);                          // upper teeth: many small ones, not fangs
    for (int i = -3; i <= 3; ++i) {
        glPushMatrix();
        glTranslatef(i * 0.026f, -0.115f, 0.32f);            // base on the muzzle underside
        glRotatef(90.f, 1, 0, 0);                            // GLUT cones run +Z; point it down
        glutSolidCone(0.013, 0.055, 6, 1);
        glPopMatrix();
    }
    glColor3f(0.10f, 0.05f, 0.05f);                          // nose: small cone at the tip
    glPushMatrix();
    glTranslatef(0.f, -0.055f, 0.44f);
    glutSolidCone(0.042, 0.05, 8, 1);
    glPopMatrix();

    for (int i = -1; i <= 1; i += 2) {                       // broad fox ears: wide base, short point
        glPushMatrix();
        glTranslatef(i * 0.13f, 0.19f, 0.01f);               // base on the skull
        glRotatef(i * 18.f, 0, 0, 1);
        glRotatef(-90.f, 1, 0, 0);                           // stand the +Z cone up
        glColor3f(0.55f, 0.17f, 0.10f);
        glutSolidCone(0.105, 0.21, 4, 1);
        glPopMatrix();
    }

    glColor3f(0.05f, 0.05f, 0.06f);                          // eye patch + strap
    part(0.09f, 0.065f, 0.18f, 0.14f, 0.115f, 0.03f);
    part(0.00f, 0.09f, 0.02f, 0.40f, 0.03f, 0.03f);

    glDisable(GL_LIGHTING);                                  // the one good eye
    if (weakened) glColor3f(0.30f, 1.0f, 0.90f);             // cyan while the player is locked on
    else          glColor3f(1.0f, 0.86f, 0.20f);
    glPushMatrix();
    glTranslatef(-0.085f, 0.065f, 0.185f);
    glutSolidSphere(0.028, 10, 10);
    glTranslatef(0.f, 0.f, 0.022f);                          // pupil
    glColor3f(0.02f, 0.01f, 0.01f);
    glutSolidSphere(0.012, 8, 8);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // ---- lower jaw, pivoted at the back of the snout ----
    glPushMatrix();
    glTranslatef(0.f, -0.125f, 0.08f);
    glRotatef(open, 1, 0, 0);
    glColor3f(0.58f, 0.18f, 0.11f);
    part(0.f, -0.03f, 0.17f, 0.18f, 0.08f, 0.24f);           // jaw slab
    glColor3f(0.70f, 0.10f, 0.10f);
    part(0.f, 0.015f, 0.17f, 0.13f, 0.03f, 0.20f);           // tongue
    glColor3f(0.90f, 0.88f, 0.80f);                          // lower teeth, pointing up
    for (int i = -3; i <= 3; ++i) {
        glPushMatrix();
        glTranslatef(i * 0.026f, 0.015f, 0.28f);
        glRotatef(-90.f, 1, 0, 0);                           // point the cone up
        glutSolidCone(0.012, 0.055, 6, 1);
        glPopMatrix();
    }
    glPopMatrix();                                           // jaw
    glPopMatrix();                                           // head squash
    glPopMatrix();                                           // whole fox
}

void drawDangerLine() {                          // floor boundary line right in front of the player
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glDepthMask(GL_FALSE);
    float b = 0.25f + 0.55f * game.ghostProgress;
    glColor3f(b * flick, 0.1f * flick, 0.1f * flick);
    glPushMatrix(); glTranslatef(0, 0.01f, cfg::END_Z); glScalef(cfg::HALL_W * 0.48f, 1, 0.08f);
    glBegin(GL_POLYGON);
    glVertex3f(-1, 0, 1); glVertex3f(1, 0, 1);
    glVertex3f(1, 0, -1); glVertex3f(-1, 0, -1);
    glEnd();
    glPopMatrix();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

void drawDetails() {                             // stippled floor line + floating dust points
    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_STIPPLE); glLineStipple(2, 0x00FF); glLineWidth(2);
    glColor3f(0.35f * flick, 0.30f * flick, 0.20f * flick);
    glBegin(GL_LINES); glVertex3f(0, 0.005f, 0); glVertex3f(0, 0.005f, -cfg::HALL_L); glEnd();
    glDisable(GL_LINE_STIPPLE);
    glPointSize(2); glColor3f(0.6f * flick, 0.6f * flick, 0.5f * flick);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_DUST; ++i) {
        float y = fmodf(dust[i][1] + now * 0.03f, cfg::HALL_H);
        glVertex3f(dust[i][0] + 0.05f * sinf(now + i), y, dust[i][2]);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// ---------------- lighting ----------------
void updateLights(float dt) {
    if (game.state == ST_PLAYING) {
        float target = 1.f;
        if (darkT > 0.f) {                                     // blackout: dim, hold, come back
            darkT -= dt;
            target = 0.f;                                      // fully dark, no spill
        } else {
            darkNext -= dt;
            if (darkNext <= 0.f) {
                float p = game.ghostProgress;
                darkT    = randRange(0.6f, 2.5f) * (0.5f + 0.5f * p);    // randomized, never over 2.5 s
                darkNext = randRange(2.0f, 6.5f) * (1.f - 0.45f * p);    // randomized gap, shorter as he nears
            }
        }
        flick += (target - flick) * clamp01(dt * ((target < flick) ? 12.f : 5.f));   // drops fast, warms back slowly
        if (darkT > 0.f) { if (!flickerMuted) { Mix_Pause(CH_FLICKER); flickerMuted = true; } }
        else             { if (flickerMuted)  { Mix_Resume(CH_FLICKER); flickerMuted = false; } }

        // The suspense track swells until 0:27, dips until 0:36, then swells again.
        // Ride the fader against it so the mix stays level instead of following the crescendo.
        if (suspense) {
            float t = game.ghostProgress * cfg::GAME_DURATION;              // seconds into the run
            float v;
            if      (t < 27.f) v = 0.42f - 0.14f * (t / 27.f);              // swell 1: 42% -> 28%
            else if (t < 36.f) v = 0.28f + 0.10f * ((t - 27.f) / 9.f);      // dip: let it back up to 38%
            else               v = 0.38f - 0.20f * ((t - 36.f) / 24.f);     // swell 2: 38% -> 18%
            Mix_VolumeChunk(suspense, (int)(MIX_MAX_VOLUME * v));
        }
    } else {
        flick += (1.f - flick) * clamp01(dt * 10.f);           // steady once the scare starts
        darkT = 0.f; darkNext = 4.f;
    }
    dip -= dt * 4.f; if (dip < 0) dip = 0;                                  // dip after each ghost step
    if (game.state == ST_LOSE) {
        if (freezeScare) {                       // DEBUG: hold the pose, J toggles
            loseTime = 0.55f; blackout = 1.f; flash = 0.f;
        } else {
            loseTime += dt;
            if (!impactHit && loseTime >= 0.30f) {           // the leap lands: hard flash on the face
                impactHit = true; flash = 0.80f;
                flashR = 1.f; flashG = 0.75f; flashB = 0.65f;
                if (scream) Mix_PlayChannel(-1, scream, 0);  // the hit
            }
            if (loseTime > 1.75f) {                                  // lights die: cut the scream with them
                if (!audioCut) { audioCut = true; Mix_FadeOutChannel(-1, 350); }
                blackout -= dt * 2.0f; if (blackout < 0) blackout = 0;
            }
        }
    } else {
        loseTime = 0.f;
        blackout = 1.f;
        impactHit = false;
        audioCut = false;
    }
    flash -= dt * 2.4f; if (flash < 0) flash = 0;
}

void applyLights(float sx, float sy) {
    float k = flick * (1.f - 0.5f * dip) * blackout;
    bool scare = (game.state == ST_LOSE && loseTime < 3.4f);
    // Flashlight shifts slightly cooler/brighter when player holds the line in the green mark
    float boost = game.inGreenZone ? 1.25f : 1.0f;
    float spotCol[4] = {1.0f * k * boost, 0.95f * k * boost, (game.inGreenZone ? 1.0f : 0.8f) * k * boost, 1};
    float pointCol[4] = {0.9f * k, 0.55f * k, 0.3f * k, 1};
    float noSpec[4] = {0, 0, 0, 1}, amb[4] = {0.03f, 0.03f, 0.04f, 1};
    if (scare) { amb[0] = 0.13f; amb[1] = 0.10f; amb[2] = 0.09f; }   // lift the shadows so the boxes read as form
    for (int i = 0; i < 3; ++i) amb[i] *= flick;                     // blackout takes the ambient with it
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    // spotlight (flashlight) at the camera; during the scare it moves up-left, so faces get a gradient
    float p0[4] = {scare ? -0.45f : 0.f, scare ? 0.55f : 1.15f, scare ? 0.15f : 0.1f, 1};
    float d0[3] = {sx * 2.f + (scare ? 0.45f : 0.f), sy * 2.f - 0.02f - (scare ? 0.55f : 0.f), -1};
    glLightfv(GL_LIGHT0, GL_POSITION, p0); glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, d0);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, scare ? 85.f : 32.f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, scare ? 2.f : 10.f);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.f); glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.06f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, spotCol); glLightfv(GL_LIGHT0, GL_SPECULAR, noSpec);
    // point light on the ceiling with attenuation
    float p1[4] = {0, 2.6f, -9, 1};
    glLightfv(GL_LIGHT1, GL_POSITION, p1); glLightfv(GL_LIGHT1, GL_DIFFUSE, pointCol);
    glLightfv(GL_LIGHT1, GL_SPECULAR, noSpec);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.f); glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.1f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.15f);
    glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
}

// ---------------- 2D layer (glOrtho, GLUT bitmap text) ----------------
void text(float x, float y, const char* s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (; *s; ++s) glutBitmapCharacter(font, *s);
}
float textW(const char* s, void* font = GLUT_BITMAP_HELVETICA_18) {
    float w = 0; for (; *s; ++s) w += glutBitmapWidth(font, *s); return w;
}
void rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();
}
void vignette(float a) {                         // one GL_TRIANGLE_STRIP ring around the screen edge
    float W = (float)winW, H = (float)winH, t = (W < H ? W : H) * 0.35f;
    float ox[5] = {0, W, W, 0, 0}, oy[5] = {0, 0, H, H, 0};
    float ix[5] = {t, W - t, W - t, t, t}, iy[5] = {t, t, H - t, H - t, t};
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < 5; ++i) {
        glColor4f(0, 0, 0, a); glVertex2f(ox[i], oy[i]);
        glColor4f(0, 0, 0, 0); glVertex2f(ix[i], iy[i]);
    }
    glEnd();
}

void drawOverlay() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vignette((game.state == ST_LOSE) ? 0.30f : 0.55f + 0.4f * game.ghostProgress);
    if (flash > 0) { glColor4f(flashR, flashG, flashB, flash); rect(0, 0, (float)winW, (float)winH); }
    if (blackout < 1) { glColor4f(0, 0, 0, 1 - blackout); rect(0, 0, (float)winW, (float)winH); }

    float timeLeft = cfg::GAME_DURATION - game.elapsed;
    if (timeLeft < 0.f) timeLeft = 0.f;

    // The tracking HUD has no meaning once he has you.
    if (game.state != ST_LOSE) {
        float bw = 460.f;
        float bx = winW / 2.f - bw * 0.5f;

        // --- 1. PROGRESS BAR (fills to 100% while interactive line stays inside green mark) ---
        float pby = 68.f, pbh = 18.f;
        glColor4f(0.f, 0.f, 0.f, 0.7f);
        rect(bx - 4, pby - 4, bw + 8, pbh + 8);
        glColor4f(0.18f, 0.20f, 0.24f, 0.85f);
        rect(bx, pby, bw, pbh);

        float fillW = bw * clamp01(game.catchProgress);
        if (game.inGreenZone) glColor4f(0.25f, 0.95f, 0.55f, 0.95f);
        else                  glColor4f(0.20f, 0.70f, 0.85f, 0.90f);
        rect(bx, pby, fillW, pbh);

        char progBuf[64];
        snprintf(progBuf, sizeof progBuf, "Banish Progress: %d%%", (int)(clamp01(game.catchProgress) * 100.f + 0.5f));
        glColor3f(0.95f, 0.95f, 0.95f);
        text((winW - textW(progBuf)) / 2.f, pby + pbh + 10.f, progBuf);

        // --- 2. HORIZONTAL TRACKING BAR (moving green mark + player-controlled interactive line) ---
        float tby = 28.f, tbh = 22.f;
        glColor4f(0.f, 0.f, 0.f, 0.75f);
        rect(bx - 4, tby - 4, bw + 8, tbh + 8);
        glColor4f(0.14f, 0.14f, 0.16f, 0.9f);
        rect(bx, tby, bw, tbh);

        // Moving Green Mark
        float gw = bw * cfg::GREEN_WIDTH;
        float gx = bx + bw * (game.greenPos - cfg::GREEN_WIDTH * 0.5f);
        if (game.inGreenZone) glColor4f(0.25f, 1.0f, 0.40f, 0.95f);
        else                  glColor4f(0.15f, 0.72f, 0.28f, 0.85f);
        rect(gx, tby, gw, tbh);

        // Interactive Line (moves right on Left Mouse Hold, moves left on Release)
        float lx = bx + bw * clamp01(game.linePos);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        rect(lx - 3.f, tby - 4.f, 6.f, tbh + 8.f);

        // Status prompt above bars
        if (game.state == ST_PLAYING) {
            const char* msg = game.inGreenZone
                ? "LOCKED ON! Keep the line inside the green mark!"
                : "HOLD Left Click or Space = Move Right  |  RELEASE = Move Left";
            if (game.inGreenZone) glColor3f(0.35f, 1.0f, 0.5f);
            else                  glColor3f(0.9f, 0.9f, 0.9f);
            text((winW - textW(msg)) / 2.f, pby + pbh + 34.f, msg);
        }

        // Controls, top-left
        glColor3f(0.85f, 0.85f, 0.85f);
        text(14, winH - 26.f, "Hold Left Mouse or Space = Line Right | Release = Line Left");
        text(14, winH - 48.f, "R = Restart | ESC = Close window");
    }

    // Last ten seconds: a flashing warning, because what comes next is not subtle.
    if (game.state == ST_PLAYING && timeLeft <= 10.f) {
        const char* warn = "JUMPSCARE WARNING!  ESC NOW!";
        float a = 0.35f + 0.65f * fabsf(sinf(now * 3.5f));
        glColor4f(1.f, 0.12f, 0.10f, a);
        text((winW - textW(warn, GLUT_BITMAP_TIMES_ROMAN_24)) / 2.f, winH / 2.f,
             warn, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    // End-of-game banner
    if (game.state == ST_WIN || (game.state == ST_LOSE && loseTime > 3.20f)) {
        bool win = (game.state == ST_WIN);
        const char* big = win ? "YOU WIN!" : "YOU LOSE!";
        const char* sub = win ? "You filled the progress bar to 100% and drove Foxy off!  (R = restart)"
                              : "Foxy reached you after 1 minute!  (R = restart)";
        if (win) glColor3f(0.3f, 1.f, 0.4f); else glColor3f(1.f, 0.15f, 0.1f);
        text((winW - textW(big, GLUT_BITMAP_TIMES_ROMAN_24)) / 2.f, winH / 2.f + 10.f, big, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.9f, 0.9f, 0.9f);
        text((winW - textW(sub)) / 2.f, winH / 2.f - 30.f, sub);
    }

    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_FOG); glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

// ---------------- callbacks ----------------
void display() {
    glClearColor(0.02f, 0.02f, 0.03f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // camera: head sway applied as rotation, jitter during the jumpscare
    float sx = sinf(now * 0.9f) * 0.02f, sy = sinf(now * 1.7f) * 0.012f, jx = 0, jy = 0, roll = sx * 25.f;
    if (game.state == ST_LOSE && loseTime < 1.75f) {
        jx = ((rand() % 1000) / 1000.f - 0.5f) * 0.12f;
        jy = ((rand() % 1000) / 1000.f - 0.5f) * 0.12f;
        roll += jx * 40.f;
    }
    glRotatef(roll, 0, 0, 1);
    gluLookAt(0, 1.2, 0, sx + jx, 1.2 + sy + jy, -1, 0, 1, 0);
    applyLights(sx, sy);

    floorMesh.draw();
    wallMesh.draw();
    drawChair(-1.0f, -10.f); drawChair(1.0f, -15.f);
    drawDangerLine();
    drawDetails();

    // Foxy approaches for 60 s; on loss he leaps out of the dark into the player's face.
    if (game.state == ST_LOSE) {
        float p = clamp01(loseTime / 0.30f);
        p = p * p * (3.f - 2.f * p);                      // smoothstep: the leap accelerates then lands
        float hold = clamp01((loseTime - 0.30f) / 1.30f); // slow creep once he is on you
        drawFoxy(-2.30f + 1.00f * p - 0.08f * hold, 1.0f + 0.25f * p + 0.05f * hold, p, 0.f, game.inGreenZone);
    } else {
        // Animatronic gait: he lurches forward in bursts and stands still between them.
        // The walk phase advances only while he moves, so the feet never skate.
        float cyc = game.ghostProgress * 24.f, c = floorf(cyc), f = cyc - c;
        float move = clamp01(f / 0.22f);                  // the shuffle takes the first fifth of each cycle
        float pos  = (c + move) / 24.f;
        float z    = -20.f + 17.7f * pos;                 // stops at -2.3 m, exactly where the leap starts
        drawFoxy(z, 1.f, 0.f, (c + move) * 3.14159265f, game.inGreenZone);
    }
    drawOverlay();
    glutSwapBuffers();
}

void timer(int) {
    int ms = glutGet(GLUT_ELAPSED_TIME); float dt = (ms - lastMs) / 1000.f; lastMs = ms;
    if (dt > 0.05f) dt = 0.05f;
    now = ms / 1000.f; bob += dt * 3.f;

    game.update(dt);
    if (game.evStep) { dip = 1; bob = 0; game.evStep = false; }
    if (game.evWin)  { flash = 0.45f; flashR = 0.3f; flashG = 1.f; flashB = 0.5f; game.evWin = false;
                       Mix_FadeOutChannel(CH_SUSPENSE, 700); Mix_HaltChannel(CH_FLICKER); }
    if (game.evLose) { flash = 0.55f; flashR = 1.f; flashG = 0.2f; flashB = 0.2f; game.evLose = false;
                       Mix_HaltChannel(CH_SUSPENSE); Mix_HaltChannel(CH_FLICKER); }
    updateLights(dt);

    // tint the hallway red as the ghost gets closer to the player over the 60 seconds
    float p = game.tensionPulse();
    wallMesh.setTint(1, 1 - 0.45f * p, 1 - 0.45f * p); wallMesh.updateColorsMapped();      // glMapBuffer
    floorMesh.setTint(1, 1 - 0.45f * p, 1 - 0.45f * p); floorMesh.updateColorsSubData();   // glBufferSubData

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    winW = w; winH = h ? h : 1; glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(cfg::FOV, (double)winW / winH, 0.05, 60);
    glMatrixMode(GL_MODELVIEW);
}
void keyboard(unsigned char k, int, int) {
    if (k == 27) exit(0);
    if (k == ' ') { spaceDown = true; game.setMouseHold(true); }   // spacebar mirrors holding LMB
    if (k == 'j' || k == 'J') {                      // DEBUG: freeze / restart the jumpscare pose
        freezeScare = !freezeScare;
        if (freezeScare) {
            game.state = ST_LOSE; loseTime = 0.55f; flash = 0; blackout = 1;
            Mix_HaltChannel(CH_FLICKER); Mix_HaltChannel(CH_SUSPENSE);
            if (scream) Mix_PlayChannel(-1, scream, 0);   // hear the scare without waiting 60 s
        } else {
            game.reset(); loseTime = 0.f; flash = 0; blackout = 1;
            Mix_HaltChannel(-1); startAmbience();
        }
    }
    if (k == 'r' || k == 'R') {
        game.reset();
        Mix_HaltChannel(-1);
        startAmbience();
        dip = 0; flash = 0; blackout = 1; loseTime = 0.f;
        freezeScare = false; impactHit = false; audioCut = false;
        flick = 1.f; darkT = 0.f; darkNext = 4.f;
    }
}
void mouse(int btn, int st, int, int) {
    if (btn == GLUT_LEFT_BUTTON) {
        mouseDown = (st == GLUT_DOWN);
        game.setMouseHold(spaceDown || mouseDown);   // either source holds the line
    }
}
void keyboardUp(unsigned char k, int, int) {
    if (k == ' ') { spaceDown = false; game.setMouseHold(mouseDown); }
}

// Assets arrive with dead air at the edges (screen recordings, previews). Cut it, so a
// loop is tight and the scream starts on the hit instead of after the scare.
Mix_Chunk* trimSilence(Mix_Chunk* in, bool trimEnd) {
    if (!in || in->alen < 8) return in;
    const Sint16* s = (const Sint16*)in->abuf;              // mixer format is AUDIO_S16SYS
    Uint32 n = in->alen / sizeof(Sint16), start = 0, end = n;
    Sint16 peak = 0;
    for (Uint32 i = 0; i < n; ++i) { Sint16 v = (Sint16)(s[i] < 0 ? -s[i] : s[i]); if (v > peak) peak = v; }
    Sint16 thr = (Sint16)(peak / 16);                       // ~6% of the loudest peak
    while (start < n && (Sint16)(s[start] < 0 ? -s[start] : s[start]) < thr) ++start;
    if (trimEnd) while (end > start && (Sint16)(s[end - 1] < 0 ? -s[end - 1] : s[end - 1]) < thr) --end;
    if (start < 4 || start >= end) return in;               // nothing worth trimming
    Mix_Chunk* out = new Mix_Chunk(*in);
    out->abuf += start * sizeof(Sint16);                    // borrowed buffer: SDL must not free it
    out->alen = (end - start) * sizeof(Sint16);
    out->allocated = 0;
    fprintf(stderr, "Audio: trimmed %.2f s head, %.2f s tail\n",
            start / 44100.0f / 4.0f, (n - end) / 44100.0f / 4.0f);
    return out;
}

// Hard cap in seconds. The mixer runs 44.1 kHz, 16-bit stereo = 4 bytes per frame.
void capLength(Mix_Chunk* c, float sec) {
    if (!c) return;
    Uint32 maxBytes = (Uint32)(sec * 44100.f) * 4u;
    if (c->alen > maxBytes) c->alen = maxBytes;
}

// Recordings are usually mastered far below full scale. Scale the samples up so the
// scream actually hits, instead of relying on the mixer's ceiling.
Mix_Chunk* amplify(Mix_Chunk* in) {
    if (!in || in->alen < 8) return in;
    const Sint16* s = (const Sint16*)in->abuf;
    Uint32 n = in->alen / sizeof(Sint16);
    Sint32 peak = 0;
    for (Uint32 i = 0; i < n; ++i) { Sint32 v = s[i]; if (v < 0) v = -v; if (v > peak) peak = v; }
    if (peak <= 0) return in;
    float gain = 32000.0f / (float)peak;                     // up to ~98% of full scale
    if (gain < 1.0f) gain = 1.0f;                            // never make it quieter
    if (gain > 8.0f) gain = 8.0f;                            // don't amplify hiss into a wall
    Sint16* d = (Sint16*)malloc(in->alen);
    if (!d) return in;
    for (Uint32 i = 0; i < n; ++i) {
        float v = s[i] * gain;
        if (v >  32767.f) v =  32767.f;
        if (v < -32768.f) v = -32768.f;
        d[i] = (Sint16)v;
    }
    Mix_Chunk* out = new Mix_Chunk(*in);
    out->abuf = (Uint8*)d;
    out->alen = in->alen;
    out->allocated = 1;                                      // this buffer is ours
    fprintf(stderr, "Audio: gain x%.2f\n", gain);
    return out;
}

// Load, de-silence, cap and normalise one clip; prints its final length.
Mix_Chunk* loadClip(const char* rel, bool trimEnd, float maxSec) {
    Mix_Chunk* c = Mix_LoadWAV(rel);
    if (!c) { char alt[512]; snprintf(alt, sizeof alt, "../%s", rel); c = Mix_LoadWAV(alt); }
    if (!c) { fprintf(stderr, "Could not load %s: %s\n", rel, Mix_GetError()); return NULL; }
    c = trimSilence(c, trimEnd);
    if (maxSec > 0.f) capLength(c, maxSec);
    c = amplify(c);
    fprintf(stderr, "Audio: %s (%.1f s)\n", rel, c->alen / 44100.0f / 4.0f);
    return c;
}

// Channels 0 and 1 are reserved for the two loops; the scream takes whatever is free.
void startAmbience() {
    if (suspense) Mix_PlayChannel(CH_SUSPENSE, suspense, -1);
    if (flicker)  Mix_PlayChannel(CH_FLICKER, flicker, -1);
    flickerMuted = false;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH); glutCreateWindow("Ghost Ward - 60s Survival");
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return 1; }
    srand(1234 + (unsigned)glutGet(GLUT_ELAPSED_TIME));

    // ---- audio: scream on the hit, suspense + flicker loops for the run ----
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Audio init failed: %s\n", Mix_GetError());
    } else {
        fprintf(stderr, "Audio driver: %s\n", SDL_GetCurrentAudioDriver());   // "dummy" = no device reachable
        Mix_ReserveChannels(2);                                   // 0 and 1 belong to the loops
        Mix_Volume(-1, MIX_MAX_VOLUME);
        scream   = loadClip("assets/audio/foxy_scream.mp3", false, 0.f);
        suspense = loadClip("assets/audio/absolutesound-suspense-tension-514626.mp3", true, 60.f);
        flicker  = loadClip("assets/audio/FLICKER SOUND EFFECT.mp3", true, 0.f);
        if (scream)   Mix_VolumeChunk(scream,   MIX_MAX_VOLUME);
        if (suspense) Mix_VolumeChunk(suspense, (int)(MIX_MAX_VOLUME * 0.40f));
        if (flicker)  Mix_VolumeChunk(flicker,  (int)(MIX_MAX_VOLUME * 0.40f));
        startAmbience();
    }

    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float fogCol[4] = {0.02f, 0.02f, 0.03f, 1};
    glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_EXP2); glFogf(GL_FOG_DENSITY, cfg::FOG_DENSITY); glFogfv(GL_FOG_COLOR, fogCol);

    buildHallway();
    for (int i = 0; i < NUM_DUST; ++i) {
        dust[i][0] = randRange(-1.4f, 1.4f); dust[i][1] = randRange(0, cfg::HALL_H); dust[i][2] = randRange(-cfg::HALL_L, 0);
    }

    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); glutKeyboardUpFunc(keyboardUp); glutMouseFunc(mouse);
    lastMs = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
