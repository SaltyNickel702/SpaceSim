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
uniform float lastTime;
float dTime;


//Constants
#define PI 3.14159265359
#define G 6.67430e-11


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


vec2 getGravity (uint i1, uinti2) {
    Object o1 = objects[i1];
    Object o2 = objects[i2];

    double f = G * (o1.mass * o2.mass) / pow(length(o1.pos-o2.pos),2); //magnitude of force
    vec2 dirNorm = normalize(o2.pos-o1.pos);

    return dirNorm * f;
}
void simulateBody (uint id)
{
    vec2 force = vec2(0);
    for (int i = 0; i < objects.size(); i++) {
        if (i == id) continue;
        force += getGravity(id,i);
    }

    vec2 acl = force * objects[id].mass;

    objects[id].vel += acl * dTime;
    objects[id].pos += objects[id].vel * dTime;
}

void main()
{
    uint i = gl_GlobalInvocationID.x; //index
    if (i >= objects.length()) return;

    dTime = Time - lastTime;

    //simulateBody(i);

    objects[id].pos.x += 100 * dTime;
}