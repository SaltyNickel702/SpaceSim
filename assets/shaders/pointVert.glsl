#version 430 core

struct Path {
    vec2 pos[512];
    vec4 color;
};

struct Object {
    vec2 pos;
    vec2 vel;
    float mass;
    float radius;
    vec4 color;
    Path path;
};

layout(std430, binding = 0) buffer ObjectBuffer {
    Object objects[];
};

struct Camera {
    vec2 pos;
    double zoom; //if errors, use float instead
    vec2 screenDim;
};
uniform Camera camera;

out vec4 colorIn;

void main() {
    uint id = uint(gl_VertexID);  // index of current object

    vec2 pos = objects[id].pos - camera.pos;  // adjust by camera position
    pos = pos / camera.zoom;                  // meters to pixels
    pos = pos / camera.screenDim * 2.0 - vec2(1.0, 1.0); // normalize to clip space

    gl_Position = vec4(pos, 0.0, 1.0);
    gl_PointSize = objects[id].radius / camera.zoom;

    colorIn = objects[id].color;
}