#version 330 core
layout (location = 0) in vec2 pos;

out vec2 pxlPosIn;

void main() {
    gl_Position = vec4(pos, 0, 1);
	pxlPosIn = pos;
}