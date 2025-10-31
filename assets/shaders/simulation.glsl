#version 430 core
layout(local_size_x = 128) in;

// Space Objects
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

uniform float Time;

#define PI 3.14159265359

// Math Func
mat4 rotateX(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        1, 0, 0, 0,
        0, c,-s, 0,
        0, s, c, 0,
        0, 0, 0, 1
    );
}
mat4 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1
    );
}
mat4 rotateZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        c,-s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}


void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= ObjectCount) return;

    // --- do your simulation work on objects[i] here ---
}