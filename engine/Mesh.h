#pragma once
// Person 1: vertex arrays + buffer objects (Modules 4-6).
// Separate VBOs for positions, normals, texcoords and colors, plus an index buffer.
#include <GL/glew.h>
#include <cstring>
#include <vector>

static GLuint makeBuffer(GLenum target, const void* data, size_t bytes, GLenum usage) {
    GLuint id; glGenBuffers(1, &id); glBindBuffer(target, id);
    glBufferData(target, bytes, data, usage); glBindBuffer(target, 0); return id;
}

struct Mesh {
    GLuint vPos = 0, vNor = 0, vUV = 0, vCol = 0, ibo = 0; GLsizei count = 0;
    std::vector<float> pos, nor, uv, col; std::vector<unsigned> idx;

    // Subdivided quad: origin + s*du + t*dv, nu x nv cells, UV repeated uRep x vRep.
    void addGrid(const float o[3], const float du[3], const float dv[3], const float n[3],
                 int nu, int nv, float uRep, float vRep) {
        unsigned base = pos.size() / 3;
        for (int j = 0; j <= nv; ++j) for (int i = 0; i <= nu; ++i) {
            float s = (float)i / nu, t = (float)j / nv;
            for (int k = 0; k < 3; ++k) { pos.push_back(o[k] + s * du[k] + t * dv[k]); nor.push_back(n[k]); col.push_back(1.f); }
            uv.push_back(s * uRep); uv.push_back(t * vRep);
        }
        for (int j = 0; j < nv; ++j) for (int i = 0; i < nu; ++i) {
            unsigned a = base + j * (nu + 1) + i, b = a + 1, c = a + (nu + 1), d = c + 1;
            unsigned q[6] = {a, b, d, a, d, c};
            idx.insert(idx.end(), q, q + 6);
        }
    }
    void upload() {
        vPos = makeBuffer(GL_ARRAY_BUFFER, &pos[0], pos.size() * sizeof(float), GL_STATIC_DRAW);
        vNor = makeBuffer(GL_ARRAY_BUFFER, &nor[0], nor.size() * sizeof(float), GL_STATIC_DRAW);
        vUV  = makeBuffer(GL_ARRAY_BUFFER, &uv[0],  uv.size()  * sizeof(float), GL_STATIC_DRAW);
        vCol = makeBuffer(GL_ARRAY_BUFFER, &col[0], col.size() * sizeof(float), GL_DYNAMIC_DRAW);
        ibo  = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, &idx[0], idx.size() * sizeof(unsigned), GL_STATIC_DRAW);
        count = (GLsizei)idx.size();
    }
    void setTint(float r, float g, float b) {
        for (size_t i = 0; i + 2 < col.size(); i += 3) { col[i] = r; col[i + 1] = g; col[i + 2] = b; }
    }
    // Two ways to refresh the color buffer (Modules 5 and 6):
    void updateColorsSubData() {                                   // glBufferSubData
        glBindBuffer(GL_ARRAY_BUFFER, vCol);
        glBufferSubData(GL_ARRAY_BUFFER, 0, col.size() * sizeof(float), &col[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void updateColorsMapped() {                                    // glMapBuffer
        glBindBuffer(GL_ARRAY_BUFFER, vCol);
        void* p = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (p) { memcpy(p, &col[0], col.size() * sizeof(float)); glUnmapBuffer(GL_ARRAY_BUFFER); }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void draw() const {
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, vPos); glVertexPointer(3, GL_FLOAT, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vNor); glNormalPointer(GL_FLOAT, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vUV);  glTexCoordPointer(2, GL_FLOAT, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vCol); glColorPointer(3, GL_FLOAT, 0, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_COLOR_ARRAY);
    }
};