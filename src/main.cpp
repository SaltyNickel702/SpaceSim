#include "Render.h"

using namespace std;
using namespace glm;

namespace GLObjects {
  struct alignas(16) Object {
		glm::vec2 pos;       // 0
		glm::vec2 vel;       // 8
		float mass;          // 16
		float radius;        // 20
		float _pad1[2];     // 24, 28 -> pad to 32
		glm::vec4 color;     // 32
		int flagDelete;      // 48
		float _pad2[3];      // 56, 60 -> pad to 64

		Object () : pos(vec2(0)), vel(vec2(0)), mass(1), radius(1), color(vec4(1)), flagDelete(0) {}
	};
	static_assert(sizeof(Object) % 16 == 0, "Object size must be multiple of 16 for std430");
	
	struct Camera {
		vec2 pos;
		float zoom; //meter per pixel
		vec2 screenDim;
	};
}


struct Space {
	thread* tick = nullptr; // must be a pointer bc thread is created inside local function
	Render render;
	Render::Object screen;
	float lastDrawTime = 0;

	vector<GLObjects::Object> objects;
	unsigned int objSSBO;
	unsigned int objVAO;

	GLObjects::Camera camera;

	float timeScale = 1;

	Space () : Space(800,600) {}
	Space (int w, int h) : render(w,h), screen(render) {
		camera.pos = vec2(0,0);
		camera.zoom = 1;
		camera.screenDim = vec2(w,h);

		render.compileShader("simulation", new Shader::Compute("simulation.glsl"));
		render.compileShader("drawPoints", new Shader::VF("pointVert.glsl","pointFrag.glsl"));

		render.tickQueue.push_back([this]() {
			//Generate object ssbo
			glGenBuffers(1, &objSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, objSSBO);

			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLObjects::Object) * (objects.size() ? objects.size() : 1), nullptr, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			//Generate a dud VAO
			glGenVertexArrays(1,&objVAO);
			glBindVertexArray(objVAO);
		});

		Render::Script* pipeline = new Render::Script (render, "pipeline");
		pipeline->run = [this]() {
			if (objects.size() == 0) return;


			#if defined(DEBUG)
			cout << endl << "Compute Shader" << endl;
			#endif

			//Compute Simulation
			glUseProgram(render.shaders["simulation"]->ID);

			//Uniforms
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objSSBO);
			
			GLuint timeLoc = glGetUniformLocation(render.shaders["simulation"]->ID, "Time");
			GLuint lastTimeLoc = glGetUniformLocation(render.shaders["simulation"]->ID, "lastTime");
			GLuint timeScaleLoc = glGetUniformLocation(render.shaders["simulation"]->ID, "timeScale");
			float currentTime = glfwGetTime();
			glUniform1f(timeLoc, currentTime);
			glUniform1f(lastTimeLoc, lastDrawTime);
			glUniform1f(timeScaleLoc, timeScale);
			lastDrawTime = currentTime;


			//Dispatch
			GLuint numGroups = (objects.size() + 127) / 128; // depends on local_size_x
			glDispatchCompute(numGroups, 1, 1);


			//Synchronize ssbo
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			std::vector<GLObjects::Object> cpuCopy(objects.size());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, objSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
							sizeof(GLObjects::Object) * objects.size(),
							cpuCopy.data());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			// cout << cpuCopy.at(0).pos.x << " " << cpuCopy.at(0).pos.y << endl;
			// cout << cpuCopy.at(1).color.w << endl;

			vector<GLObjects::Object> objs;
			for (GLObjects::Object o : cpuCopy) {
				if (o.flagDelete == 1) continue;
				objs.push_back(o);
				// cout << o.flagDelete << endl;
			}
			// cout << objs.size() << endl;
			rebufferObjects(objs);
			


			#if defined(DEBUG)
			cout << endl << "Point Shader" << endl;
			#endif

			//Draw Everything
			glBindVertexArray(objVAO);
			glUseProgram(render.shaders["drawPoints"]->ID);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objSSBO);

			GLuint camPosLoc = glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.pos");
			GLuint camZoomLoc = glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.zoom");
			GLuint camScrLoc = glGetUniformLocation(render.shaders["drawPoints"]->ID, "camera.screenDim");
			glUniform2fv(camPosLoc, 1, value_ptr(camera.pos));
			glUniform1f(camZoomLoc, camera.zoom);
			glUniform2fv(camScrLoc, 1, value_ptr(camera.screenDim));

			glDrawArrays(GL_POINTS, 0, objects.size());

			#if defined(DEBUG)
			cout << endl << "Compute ID: " << render.shaders["simulation"]->ID << " Point ID: " << render.shaders["drawPoints"]->ID << endl;
			#endif

			if (glfwWindowShouldClose(render.window)) shouldClose.store(true);
		};
	
		tick = new thread(&Space::tickInit, this);
	}

	void rebufferObjects (vector<GLObjects::Object> objs) { //can be called asynchronously
		//Update object ssbo
		bool rebuffer = objects.size() != objs.size();
		objects = objs;
		render.tickQueue.push_back([this, rebuffer]() {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, objSSBO);

			if (rebuffer) {
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLObjects::Object) * (objects.size() ? objects.size() : 1), objects.data(), GL_DYNAMIC_DRAW);
			} else {
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLObjects::Object) * objects.size(), objects.data());
			}
		});
	}
	private:
		atomic_bool shouldClose{false};
		float lastTickTime = 0;
		float dTime = 0;
		const int tickRate = 120;
		const float tickInterval = 1 / (float)tickRate;
		void tickInit () {
			while (!shouldClose.load()) {
				float time = glfwGetTime();
				dTime = time - lastTickTime;
				double dif = tickInterval - dTime;
				int us = dif * 1e+6; //microseconds
				if (dTime < tickInterval) {
					this_thread::sleep_for(chrono::microseconds(us));
					time = glfwGetTime();
					dTime = time-lastTickTime;
				};
				tickFunc();
				lastTickTime = time;

				render.scriptTicked.store(true);
			}
		}
		void tickFunc () {
			if (render.isKeyDown(GLFW_KEY_Q)) camera.zoom *= 1 + dTime * 1;
			if (render.isKeyDown(GLFW_KEY_E)) camera.zoom /= 1 + dTime * 1;

			if (render.isKeyDown(GLFW_KEY_W)) camera.pos += vec2(0,300) * camera.zoom * dTime;
			if (render.isKeyDown(GLFW_KEY_S)) camera.pos += vec2(0,-300) * camera.zoom * dTime;
			if (render.isKeyDown(GLFW_KEY_D)) camera.pos += vec2(300,0) * camera.zoom * dTime;
			if (render.isKeyDown(GLFW_KEY_A)) camera.pos += vec2(-300,0) * camera.zoom * dTime;

			if (render.isKeyDown(GLFW_KEY_R)) timeScale += 3*dTime;
			if (render.isKeyDown(GLFW_KEY_F)) timeScale /= 1 + 1*dTime;
			if (render.isKeyDown(GLFW_KEY_T)) timeScale = 1;
		}
};


int main () {
	Space space(1200,900);

	float multi = 4;

	GLObjects::Object sun;
	sun.pos = vec2(0,0);
	sun.mass = 10000 * multi;
	sun.radius = 4;
	sun.color = vec4(1,.8,0,1);
	vector<GLObjects::Object> objs = {sun};

	srand(time(0));
	for (int i = 0; i < 1000 * multi; i++) {
		GLObjects::Object o;
		o.mass = pow((rand() % 10)/9,5)*9 + 1;
		o.radius = pow( (3 * o.mass) / (4 * M_PI * 75) ,1.0/3.0) + 0.25;

		float randDeg = rand() % 360;
		float t = rand() / (float)RAND_MAX; // [0,1]
		float randDist = (cbrtf(t) * 170.0f + 30.0f) * multi;

		o.pos = vec2(cos(radians(randDeg)),sin(radians(randDeg))) * randDist;

		// float randSpd = rand() % ((int) sqrt((sun.mass)/randDist));
		float randSpd = sqrt(sun.mass / randDist);// * ((rand() % 2 == 0 )? -1 : 1); //uncomment for 50-50 chance at oribiting opposite way
		float tanDeg = ((int)randDeg + 90) % 360;
		o.vel = vec2(cos(radians(tanDeg)),sin(radians(tanDeg))) * randSpd;

		o.color = vec4(1) * (rand() / (float)RAND_MAX * 0.5f) + 0.5f;
		// o.color = vec4(rand() / (float) RAND_MAX,rand() / (float) RAND_MAX,rand() / (float) RAND_MAX,1);

		// cout << o.pos.x << " " << o.pos.y << endl;

		objs.push_back(o);
	}

	cout << objs.size() << endl;

	space.rebufferObjects(objs);

	space.camera.zoom = 200/space.camera.screenDim.y * 2 * multi;

	space.render.renderThread->join();
	return 0;
}