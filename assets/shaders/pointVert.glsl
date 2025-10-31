#version 330 core
layout(std430, binding = 0) buffer ObjectBuffer {
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
    Object objects[];
};

struct Camera {
    vec2 pos;
    double zoom;
    vec2 screenDim;
}
uniform Camera camera;


void main() {
    uint id = gl_VertexID; //index of current object
    

    vec2 pos = objects[id].pos; //meters
    pos = pos * (1 / camera.zoom); //pixel
    pos = pos * vec2(1/camera.screenDim.x,1/camera.screenDim.y) * 2 - vec2(1,1); //screen space

    gl_Position = vec4(pos,0,0);
    
    gl_PointSize = 0; //radius
}