#version 430 core

struct Object {
    vec2 pos;
    vec2 vel;
    float mass;
    float radius;
    vec4 color;
    int flagDelete;
    float pad0;
};

layout(std430, binding = 0) buffer ObjectBuffer {
    Object objects[];
};

struct Camera {
    vec2 pos;
    float zoom;
    vec2 screenDim;
};
uniform Camera camera;

out vec4 colorIn;

void main() {
    uint id = uint(gl_VertexID);  // index of current object

    vec2 pos = objects[id].pos - camera.pos;  // adjust by camera position
    pos = pos / camera.zoom;                  // meters to pixels
    pos = pos / camera.screenDim * 2.0; // normalize to clip space

    gl_Position = vec4(pos,0,1);
    gl_PointSize = 2 * objects[id].radius / camera.zoom;

    // colorIn = vec4(0,abs(objects[id].vel / camera.screenDim.x * 10),1);
    colorIn = objects[id].color;
}