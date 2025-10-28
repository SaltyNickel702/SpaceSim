#version 330 core
out vec4 color;
in vec2 pxlPosIn;

float lastTime;
uniform float Time;
uniform vec2 ScreenDim;

#define PI 3.14159265359


//Math Func
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

//Space Objects
struct Object {
    vec2 pos;
    vec2 vel;
    float mass;
    float radius;
    vec4 color;
}
struct Path {
    vec2 pos[512];
    vec4 color;
}


void main()
{
	vec2 adjPxlPos = pxlPosIn;
	adjPxlPos.y *= ScreenDim.y/ScreenDim.x; //Scale to match screen aspect ratio
	
	color = vec4(0);	

    lastTime = Time;
}