// Hallway game: click when the character is standing on the floor mark.
// Click too early, or let him walk past the mark, and he lunges at you.
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

void drawCharacter(float z, float s, float step) {
    glPushMatrix();
    glTranslatef(0, 0.03f * fabsf(sinf(bob)), z);
    glRotatef(sinf(bob) * 3.f, 0, 0, 1);         // wobble
    glScalef(s, s, s);                           // lunge scaling
    glColor3f(0.72f, 0.58f, 0.48f);
    part(0, 1.2f, 0, 0.45f, 1.0f, 0.25f);                          // torso
    part(-0.12f, 0.35f, 0, 0.12f, 0.7f, 0.12f);                    // legs
    part(0.12f, 0.35f, 0, 0.12f, 0.7f, 0.12f);
    part(0, 1.95f, 0, 0.28f, 0.4f, 0.28f);                         // head
    for (int i = -1; i <= 1; i += 2) {                             // long arms, swing from the shoulder
        glPushMatrix(); glTranslatef(i * 0.32f, 1.7f, 0); glRotatef(sinf(bob) * 10.f * i + step * 12.f, 1, 0, 0);
        part(0, -0.55f, 0, 0.09f, 1.1f, 0.09f); glPopMatrix();
    }
    glDisable(GL_LIGHTING); glColor3f(1, 0.9f, 0.7f);   // glowing eyes
    for (int i = -1; i <= 1; i += 2) { glPushMatrix(); glTranslatef(i * 0.08f, 1.98f, 0.15f); glutSolidSphere(0.025, 8, 8); glPopMatrix(); }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawMark(float pulse) {                     // GL_POLYGON quad, scaled by markPulse()
    float s = 0.55f * (1.f + 0.25f * pulse * (0.5f + 0.5f * sinf(now * 9.f)));
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glDepthMask(GL_FALSE);
    float b = 0.35f + 0.65f * pulse; glColor3f(b, b, b);
    glPushMatrix(); glTranslatef(0, 0.01f, game.markZ()); glScalef(s, 1, s);
    glBegin(GL_POLYGON);
    glVertex3f(-1, 0, 1); glVertex3f(1, 0, 1);
    glVertex3f(1, 0, -1); glVertex3f(-1, 0, -1);
    glEnd(); glPopMatrix();
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
    dip -= dt * 4.f; if (dip < 0) dip = 0;                                  // dip after each step
    if (game.state == ST_LOSE && game.stateTime > 0.6f) { blackout -= dt * 3.f; if (blackout < 0) blackout = 0; }
    if (game.state != ST_LOSE && game.state != ST_WIN && game.state != ST_PAST_MARK) blackout = 1;
    flash -= dt * 1.6f; if (flash < 0) flash = 0;
}

void applyLights(float sx, float sy) {
    float k = flick * (1.f - 0.5f * dip) * blackout;
    float spotCol[4] = {1.0f * k, 0.95f * k, 0.8f * k, 1}, pointCol[4] = {0.9f * k, 0.55f * k, 0.3f * k, 1};
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

    vignette(0.55f + 0.4f * game.shown);
    if (flash > 0) { glColor4f(flashR, flashG, flashB, flash); rect(0, 0, (float)winW, (float)winH); }
    if (blackout < 1) { glColor4f(0, 0, 0, 1 - blackout); rect(0, 0, (float)winW, (float)winH); }

    // progress bar with the hit window highlighted
    float bx = winW / 2.f - 200, by = 28, bw = 400;
    glColor4f(0, 0, 0, 0.6f); rect(bx - 4, by - 4, bw + 8, 22);
    glColor4f(0.2f, 0.8f, 0.3f, 0.9f);
    rect(bx + bw * (cfg::MARK_PROGRESS - cfg::HIT_WINDOW), by, bw * 2 * cfg::HIT_WINDOW, 14);
    glColor4f(1, 1, 1, 0.9f); rect(bx + bw * clamp01(game.shown) - 2, by - 3, 4, 20);

    const char* msg = 0;
    if (game.state == ST_WAITING) msg = "Wait...";
    else if (game.state == ST_ON_MARK) msg = "NOW!";
    else if (game.state == ST_MOVING) msg = "Click when he stands on the mark";
    glColor3f(0.9f, 0.9f, 0.9f);
    if (msg) text((winW - textW(msg)) / 2, by + 34, msg);
    char buf[64]; snprintf(buf, sizeof buf, "Steps: %d", game.steps); text(14, winH - 26.f, buf);
    glColor3f(0.9f, 0.9f, 0.9f);
    text(14, winH - 50.f, "Mouse left click = CATCH");
    text(14, winH - 72.f, "ESC = Close window");

    if (game.state == ST_WIN || (game.state == ST_LOSE && game.stateTime > 1.2f)) {
        bool win = game.state == ST_WIN;
        const char* big = win ? "YOU WIN" : "YOU LOSE";
        const char* sub = win ? "You caught him on the mark.  R = restart"
                              : (game.earlyClick ? "Too early.  R = restart" : "He got past the mark.  R = restart");
        if (win) glColor3f(0.3f, 1, 0.4f); else glColor3f(1, 0.15f, 0.1f);
        text((winW - textW(big, GLUT_BITMAP_TIMES_ROMAN_24)) / 2, winH / 2.f + 10, big, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.85f, 0.85f, 0.85f); text((winW - textW(sub)) / 2, winH / 2.f - 30, sub);
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
    if (game.state == ST_LOSE && game.stateTime < 0.8f) {
        jx = ((rand() % 1000) / 1000.f - 0.5f) * 0.12f; jy = ((rand() % 1000) / 1000.f - 0.5f) * 0.12f; roll += jx * 40;
    }
    glRotatef(roll, 0, 0, 1);
    gluLookAt(0, 1.2, 0, sx + jx, 1.2 + sy + jy, -1, 0, 1, 0);
    applyLights(sx, sy);

    floorMesh.draw();
    wallMesh.draw();
    drawChair(-1.0f, -10.f); drawChair(1.0f, -15.f);
    drawMark(game.markPulse());
    drawDetails();

    float z = Game::progressToPosition(game.shown), sc = 1;
    if (game.state == ST_LOSE) {                          // jumpscare lunge: translate + scale
        float p = clamp01(game.stateTime / 0.45f); p *= p;
        z = z + (-0.9f - z) * p; sc = 1.f + 0.7f * p;
    }
    drawCharacter(z, sc, dip);
    drawOverlay();
    glutSwapBuffers();
}

void timer(int) {
    int ms = glutGet(GLUT_ELAPSED_TIME); float dt = (ms - lastMs) / 1000.f; lastMs = ms;
    if (dt > 0.05f) dt = 0.05f;
    now = ms / 1000.f; bob += dt * 3.f;
    game.update(dt);
    if (game.evStep) { dip = 1; bob = 0; game.evStep = false; }
    if (game.evWin)  { flash = 0.35f; flashR = 0.3f; flashG = 1; flashB = 0.4f; game.evWin = false; }
    if (game.evLose) { flash = 1; flashR = flashG = flashB = 1; game.evLose = false; }
    updateLights(dt);
    // tint the hallway red as the character nears the mark: update color VBOs each frame
    float p = game.markPulse();
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
    if (k == 'r' || k == 'R') { game.reset(); dip = 0; flash = 0; blackout = 1; }
}
void mouse(int btn, int st, int, int) { if (btn == GLUT_LEFT_BUTTON && st == GLUT_DOWN) game.click(); }

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH); glutCreateWindow("Monster Catcher");
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