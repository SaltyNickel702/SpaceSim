#include "Render.h"

using namespace std;
using namespace glm;

namespace GLObjects {
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

        Object () {}
    };

    struct Camera {
        vec2 pos;
        double zoom; //meter per pixel
        vec2 screenDim;
    };
}


struct Space {
    thread* tick; // must be a pointer bc thread is created inside local function
    Render render;
    Render::Object screen;
    float lastTickTime = 0;

    vector<GLObjects::Object> objects;
    unsigned int objSSBO;

    GLObjects::Camera camera;

    Space () : Space(800,600) {}
    Space (int w, int h) : render(w,h), screen(render) {
        camera.pos = vec2(0,0);
        camera.zoom = 1;
        camera.screenDim = vec2(w,h);

        render.compileShader("simulation", new Shader::Compute("simulation.glsl"));
        render.compileShader("drawPoints", new Shader::VF("pointVert.glsl","pointFrag.glsl"));

        render.tickQueue.push_back([this]() {
            cout << "ticked" << endl;
            //Generate object ssbo
            glGenBuffers(1, &objSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, objSSBO);

            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLObjects::Object) * (objects.size() ? objects.size() : 1), nullptr, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        });

        Render::Script* pipeline = new Render::Script (render, "pipeline");
        pipeline->run = [this]() {
            if (objects.size() == 0) return;
            cout << 1 << endl;
            //Compute Simulation
            glUseProgram(render.shaders["simulation"]->ID);

            cout << 2 << endl;
            //Uniforms
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objSSBO);
            float currentTime = glfwGetTime();
            glUniform1f(glGetUniformLocation(render.shaders["simulation"]->ID, "Time"), currentTime);
            glUniform1f(glGetUniformLocation(render.shaders["simulation"]->ID, "lastTime"), lastTickTime);
            lastTickTime = currentTime;

            cout << 3 << endl;
            //Dispatch
            GLuint numGroups = (objects.size() + 127) / 128; // depends on local_size_x
            glDispatchCompute(numGroups, 1, 1);


            cout << 4 << endl;
            //Synchronize ssbo
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


            cout << 5 << endl;
            //Draw Everything
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objSSBO);

            glUniform2fv(glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.pos"), 1, value_ptr(camera.pos));
            glUniform1d(glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.zoom"), camera.zoom);
            glUniform2fv(glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.screenDim"), 1, value_ptr(camera.screenDim));

            glUseProgram(render.shaders["drawPoints"]->ID);
            glDrawArrays(GL_POINTS, 0, objects.size());
            cout << 6 << endl;
        };
    }

    void rebufferObjects () { //can be called asynchronously
        render.tickQueue.push_back([this]() {
            //Update object ssbo
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, objSSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                sizeof(GLObjects::Object) * objects.size(),
                objects.data());
        });
    }
};


int main () {
    Space space(800,600);

    GLObjects::Object obj;
    obj.pos = vec2(0,0);
    obj.vel = vec2(0,0);
    obj.mass = 1;
    obj.radius = 100;
    obj.color = vec4(1,1,1,1);
    vector<GLObjects::Object> objs = {obj};
    space.objects = objs;
    space.rebufferObjects();

    space.render.renderThread->join();
    return 0;
}