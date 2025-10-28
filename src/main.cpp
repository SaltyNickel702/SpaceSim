#include "Render.h"

using namespace std;
using namespace glm;

stuct Space {
    thread* tick; // must be a pointer bc thread is created inside local function
    Render render;
    Render::Object screen;
    Space () : Space(800,600) {}
    Space (int w, int h) : render(w,h), screen(render) {
        render.compileShader("space","fullscrnFrag.glsl","system.glsl")
    }
};