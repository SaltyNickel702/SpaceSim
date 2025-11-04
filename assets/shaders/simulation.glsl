#version 430 core
layout(local_size_x = 128) in;
#define O(i) objects[i]

// Space Objects
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

uniform float Time;
uniform float lastTime;
uniform float timeScale;
float dTime;


//Constants
#define PI 3.14159265359
#define G 1

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


vec2 getGravity (uint i1, uint i2) {
    Object o1 = objects[i1];
    Object o2 = objects[i2];
    if (o1.flagDelete == 1 || o2.flagDelete == 1) return vec2(0);

    float f = G * (o1.mass * o2.mass) / pow(length(o1.pos-o2.pos),2); //magnitude of force
    vec2 dirNorm = normalize(o2.pos-o1.pos);

    return dirNorm * f;
}
void simulateBody (uint id)
{
    vec2 force = vec2(0);
    for (int i = 0; i < objects.length(); i++) {
        if (i == id) continue;
        float dist = length(O(id).pos - O(i).pos);
        if (dist <= O(id).radius + O(i).radius) { //collides
            // O(id).color.w = O(id).radius;
            if (O(id).mass > O(i).mass || O(i).flagDelete == 1) { //join masses together
                //Proportionaly join velocities
                O(id).vel = (O(id).vel * O(id).mass + O(i).vel * O(i).mass) / (O(id).mass + O(i).mass);
                
                O(id).pos = (O(id).pos * O(id).mass + O(i).pos * O(i).mass) / (O(id).mass + O(i).mass);

                float den = O(id).mass / (4.0/3.0 * PI * pow(O(id).radius,3));
                O(id).mass += O(i).mass;
                O(id).radius = pow( (3 * O(id).mass) / (4 * PI * den) ,1.0/3.0);

                O(id).color = (O(id).color * O(id).mass + O(i).color * O(i).mass) / (O(id).mass + O(i).mass);
            } else O(id).flagDelete = 1;
        } else force += getGravity(id,i);
    }

    vec2 acl = force / O(id).mass;

    O(id).vel = O(id).vel + acl * dTime;
    O(id).pos += O(id).vel * dTime;
}

void main()
{
    uint i = gl_GlobalInvocationID.x; //index
    if (i >= objects.length()) return;

    dTime = timeScale * (Time - lastTime);

    simulateBody(i);

    // objects[i].pos.y += 100 * dTime;
}