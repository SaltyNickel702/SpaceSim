#version 430 core
in vec4 colorIn;
out vec4 FragColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) discard;  // circular shape
    FragColor = colorIn; // color per object if desired
}