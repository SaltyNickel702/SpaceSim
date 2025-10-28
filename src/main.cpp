#include "Render.h"

using namespace std;
using namespace glm;

stuct Space {
    thread* tick; // must be a pointer bc thread is created inside local function
    Render render;
    Render::Object screen;
    float lastTickTime = 0;
    Space () : Space(800,600) {}
    Space (int w, int h) : render(w,h), screen(render) {
        render.compileShader("space","fullscrnFrag.glsl","system.glsl")

        screen.vertices = {
			-1,1,
			-1,-1,
			1,-1,
			1,1
		};
		screen.indices = {0,1,2,0,2,3};
		screen.attr = {2};
		screen.flagReady = true;

        screen.draw = [this]() {
            Shader& s = *render.shaders["space"];
            glUseProgram(s.ID);


			glUniform2fv(glGetUniformLocation(s.ID,"ScreenDim"), 1, value_ptr(vec2(render.width, render.height)));
			
            float currentTime = glfwGetTime();
            glUniform1f(glGetUniformLocation(s.ID, "Time"), currentTime);
			glUniform1f(glGetUniformLocation(s.ID, "Time"), lastTickTime);
            lastTickTime = currentTime;



            glBindVertexArray(screen.VAO);
			glDrawElements(GL_TRIANGLES,screen.totalIndices,GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
        };
    }
};