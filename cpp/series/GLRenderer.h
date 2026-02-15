// ─── WizSeries: Minimal WebGL 2 Rendering Utilities ─────────────────────────
// Shared by all visualizers.  Manages a single shader program and a dynamic
// VBO for streaming coloured 2-D vertices each frame.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include <GLES3/gl3.h>
#include <cstdio>
#include <vector>

// ─── Vertex layout: position (x,y) + colour (r,g,b,a) ──────────────────────

struct Vertex {
    float x, y;
    float r, g, b, a;
};

// Append a screen-aligned quad (two triangles) to a vertex buffer.
inline void addQuad(std::vector<Vertex>& out,
                    float x1, float y1, float x2, float y2,
                    float r, float g, float b, float a = 1.0f) {
    out.push_back({x1, y1, r, g, b, a});
    out.push_back({x2, y1, r, g, b, a});
    out.push_back({x1, y2, r, g, b, a});
    out.push_back({x2, y1, r, g, b, a});
    out.push_back({x2, y2, r, g, b, a});
    out.push_back({x1, y2, r, g, b, a});
}

// ─── GLRenderer ─────────────────────────────────────────────────────────────

class GLRenderer {
public:
    bool init() {
        const char* vs_src =
            "#version 300 es\n"
            "layout(location = 0) in vec2 a_pos;\n"
            "layout(location = 1) in vec4 a_color;\n"
            "uniform float u_point_size;\n"
            "out vec4 v_color;\n"
            "void main() {\n"
            "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
            "    gl_PointSize = u_point_size;\n"
            "    v_color = a_color;\n"
            "}\n";

        const char* fs_src =
            "#version 300 es\n"
            "precision mediump float;\n"
            "in vec4 v_color;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "    fragColor = v_color;\n"
            "}\n";

        GLuint vs = compileShader(GL_VERTEX_SHADER, vs_src);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_src);
        if (!vs || !fs) return false;

        program_ = glCreateProgram();
        glAttachShader(program_, vs);
        glAttachShader(program_, fs);
        glLinkProgram(program_);

        GLint linked = 0;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (!linked) return false;

        glDeleteShader(vs);
        glDeleteShader(fs);

        u_point_size_ = glGetUniformLocation(program_, "u_point_size");

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        // position (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        // colour (vec4)
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex),
                              reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        initialized_ = true;
        return true;
    }

    void beginFrame(float width, float height) {
        glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
        glClearColor(0.04f, 0.04f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program_);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void drawPoints(const std::vector<Vertex>& verts, float size = 2.0f) {
        if (!verts.empty()) draw(verts, GL_POINTS, size);
    }
    void drawLines(const std::vector<Vertex>& verts) {
        if (!verts.empty()) draw(verts, GL_LINES, 1.0f);
    }
    void drawLineStrip(const std::vector<Vertex>& verts) {
        if (!verts.empty()) draw(verts, GL_LINE_STRIP, 1.0f);
    }
    void drawTriangles(const std::vector<Vertex>& verts) {
        if (!verts.empty()) draw(verts, GL_TRIANGLES, 1.0f);
    }

    [[nodiscard]] bool isInitialized() const { return initialized_; }

private:
    GLuint program_     = 0;
    GLuint vao_         = 0;
    GLuint vbo_         = 0;
    GLint  u_point_size_ = -1;
    bool   initialized_  = false;

    void draw(const std::vector<Vertex>& verts, GLenum mode, float ps) {
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                     verts.data(), GL_DYNAMIC_DRAW);
        glUniform1f(u_point_size_, ps);
        glDrawArrays(mode, 0, static_cast<GLsizei>(verts.size()));
        glBindVertexArray(0);
    }

    static GLuint compileShader(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::printf("Shader compile error: %s\n", log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
};
