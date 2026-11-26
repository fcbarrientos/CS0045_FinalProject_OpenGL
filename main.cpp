// Hallway Ghost Ward: Hold left mouse button to move the interactive line right,
// release to move left. Keep the line inside the moving green mark to fill the
// progress bar to 100% before the ghost reaches you at 60 seconds!
#include <GL/glew.h>
#include <GL/glut.h>
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
float loseTime = 0.f;
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

void drawGhost(float z, float s, float step, bool weakened) {
    glPushMatrix();
    // Eerie floating hover + horizontal sway as ghost drifts toward player
    float hoverY = 0.12f + 0.06f * sinf(now * 2.8f) + 0.02f * fabsf(sinf(bob));
    float swayX  = 0.12f * sinf(now * 1.4f);
    glTranslatef(swayX, hoverY, z);
    glRotatef(sinf(now * 2.0f) * 4.f, 0, 0, 1);
    glScalef(s, s, s);

    // Translucent ghostly body (shifts slightly blue/cyan when being banished in the green zone)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (weakened) glColor4f(0.45f, 0.85f, 0.95f, 0.78f);
    else          glColor4f(0.78f, 0.82f, 0.88f, 0.85f);

    part(0, 1.25f, 0, 0.46f, 1.05f, 0.26f);                         // upper torso / shroud
    part(-0.13f, 0.55f, 0, 0.14f, 0.65f, 0.14f);                    // tattered robe left
    part(0.13f, 0.55f, 0, 0.14f, 0.65f, 0.14f);                     // tattered robe right
    part(0, 0.60f, 0, 0.18f, 0.55f, 0.16f);                         // tattered robe center
    part(0, 1.98f, 0, 0.28f, 0.38f, 0.28f);                         // head

    for (int i = -1; i <= 1; i += 2) {                              // reaching ghostly arms
        glPushMatrix();
        glTranslatef(i * 0.32f, 1.72f, 0);
        glRotatef(-25.f + sinf(now * 3.f) * 8.f * i + step * 10.f, 1, 0, 0);
        part(0, -0.52f, 0, 0.08f, 1.05f, 0.08f);
        glPopMatrix();
    }
    glDisable(GL_BLEND);

    // Glowing eyes (bright red when close, cyan when player is inside green zone)
    glDisable(GL_LIGHTING);
    if (weakened) glColor3f(0.3f, 1.0f, 0.9f);
    else          glColor3f(1.0f, 0.25f, 0.2f);
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * 0.08f, 2.00f, 0.15f);
        glutSolidSphere(0.028, 8, 8);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawDangerLine() {                          // floor boundary line right in front of the player
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glDepthMask(GL_FALSE);
    float b = 0.25f + 0.55f * game.ghostProgress;
    glColor3f(b, 0.1f, 0.1f);
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
    glColor3f(0.35f, 0.3f, 0.2f);
    glBegin(GL_LINES); glVertex3f(0, 0.005f, 0); glVertex3f(0, 0.005f, -cfg::HALL_L); glEnd();
    glDisable(GL_LINE_STIPPLE);
    glPointSize(2); glColor3f(0.6f, 0.6f, 0.5f);
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
    float target = ((rand() % 100) < 3) ? 0.25f : 1.f;                     // random flicker
    flick += (target - flick) * clamp01(dt * 30.f);
    dip -= dt * 4.f; if (dip < 0) dip = 0;                                  // dip after each ghost step
    if (game.state == ST_LOSE) {
        loseTime += dt;
        if (loseTime > 0.6f) { blackout -= dt * 3.f; if (blackout < 0) blackout = 0; }
    } else {
        loseTime = 0.f;
        blackout = 1.f;
    }
    flash -= dt * 1.6f; if (flash < 0) flash = 0;
}

void applyLights(float sx, float sy) {
    float k = flick * (1.f - 0.5f * dip) * blackout;
    // Flashlight shifts slightly cooler/brighter when player holds the line in the green mark
    float boost = game.inGreenZone ? 1.25f : 1.0f;
    float spotCol[4] = {1.0f * k * boost, 0.95f * k * boost, (game.inGreenZone ? 1.0f : 0.8f) * k * boost, 1};
    float pointCol[4] = {0.9f * k, 0.55f * k, 0.3f * k, 1};
    float noSpec[4] = {0, 0, 0, 1}, amb[4] = {0.03f, 0.03f, 0.04f, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    // spotlight (flashlight) at the camera
    float p0[4] = {0, 1.15f, 0.1f, 1}, d0[3] = {sx * 2.f, sy * 2.f - 0.02f, -1};
    glLightfv(GL_LIGHT0, GL_POSITION, p0); glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, d0);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 32.f); glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 10.f);
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

    vignette(0.55f + 0.4f * game.ghostProgress);
    if (flash > 0) { glColor4f(flashR, flashG, flashB, flash); rect(0, 0, (float)winW, (float)winH); }
    if (blackout < 1) { glColor4f(0, 0, 0, 1 - blackout); rect(0, 0, (float)winW, (float)winH); }

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
            : "HOLD Left Click = Move Right  |  RELEASE = Move Left";
        if (game.inGreenZone) glColor3f(0.35f, 1.0f, 0.5f);
        else                  glColor3f(0.9f, 0.9f, 0.9f);
        text((winW - textW(msg)) / 2.f, pby + pbh + 34.f, msg);
    }

    // Top-left HUD: 60s Ghost Countdown Timer & Controls
    float timeLeft = cfg::GAME_DURATION - game.elapsed;
    if (timeLeft < 0.f) timeLeft = 0.f;
    char timeBuf[64];
    snprintf(timeBuf, sizeof timeBuf, "Ghost Arrival: %.1fs", timeLeft);
    if (timeLeft <= 10.f) glColor3f(1.0f, 0.25f, 0.2f);
    else                  glColor3f(0.95f, 0.9f, 0.85f);
    text(14, winH - 26.f, timeBuf);

    glColor3f(0.85f, 0.85f, 0.85f);
    text(14, winH - 50.f, "Hold Left Mouse = Line Right | Release = Line Left");
    text(14, winH - 72.f, "R = Restart | ESC = Close window");

    // End-of-game banner
    if (game.state == ST_WIN || (game.state == ST_LOSE && loseTime > 0.8f)) {
        bool win = (game.state == ST_WIN);
        const char* big = win ? "YOU WIN!" : "YOU LOSE!";
        const char* sub = win ? "You filled the progress bar to 100% and banished the ghost!  (R = restart)"
                              : "The ghost reached you after 1 minute!  (R = restart)";
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
    if (game.state == ST_LOSE && loseTime < 0.8f) {
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

    // Ghost moves steadily toward the player, reaching END_Z at exactly 60 seconds
    float z = Game::progressToPosition(game.ghostProgress), sc = 1.f;
    if (game.state == ST_LOSE) {                          // jumpscare lunge: translate + scale
        float p = clamp01(loseTime / 0.45f); p *= p;
        z = z + (-0.55f - z) * p;
        sc = 1.f + 0.65f * p;
    }
    drawGhost(z, sc, dip, game.inGreenZone);
    drawOverlay();
    glutSwapBuffers();
}

void timer(int) {
    int ms = glutGet(GLUT_ELAPSED_TIME); float dt = (ms - lastMs) / 1000.f; lastMs = ms;
    if (dt > 0.05f) dt = 0.05f;
    now = ms / 1000.f; bob += dt * 3.f;

    game.update(dt);
    if (game.evStep) { dip = 1; bob = 0; game.evStep = false; }
    if (game.evWin)  { flash = 0.45f; flashR = 0.3f; flashG = 1.f; flashB = 0.5f; game.evWin = false; }
    if (game.evLose) { flash = 1.f; flashR = 1.f; flashG = 0.2f; flashB = 0.2f; game.evLose = false; }
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
    if (k == 'r' || k == 'R') {
        game.reset();
        dip = 0; flash = 0; blackout = 1; loseTime = 0.f;
    }
}
void mouse(int btn, int st, int, int) {
    if (btn == GLUT_LEFT_BUTTON) {
        if (st == GLUT_DOWN) game.setMouseHold(true);
        else if (st == GLUT_UP) game.setMouseHold(false);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH); glutCreateWindow("Ghost Ward - 60s Survival");
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return 1; }
    srand(1234 + (unsigned)glutGet(GLUT_ELAPSED_TIME));

    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float fogCol[4] = {0.02f, 0.02f, 0.03f, 1};
    glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_EXP2); glFogf(GL_FOG_DENSITY, cfg::FOG_DENSITY); glFogfv(GL_FOG_COLOR, fogCol);

    buildHallway();
    for (int i = 0; i < NUM_DUST; ++i) {
        dust[i][0] = randRange(-1.4f, 1.4f); dust[i][1] = randRange(0, cfg::HALL_H); dust[i][2] = randRange(-cfg::HALL_L, 0);
    }

    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); glutMouseFunc(mouse);
    lastMs = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
