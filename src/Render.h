#ifndef RENDER_H
#define RENDER_H

// #define DEBUG 0

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <functional>
#include <memory>
#include <chrono>

using namespace std;


namespace Shader {
	struct Shader {
		unsigned int ID = 0;
		bool readyToRun = false;

		virtual void compile () {};

		virtual ~Shader () {
			if (ID != 0) glDeleteProgram(ID);
		}
	};
	struct VF : Shader {
		VF (string vertexPath,  string fragmentPath) : vP(vertexPath), fP(fragmentPath) {}
		string vP, fP;

		void compile () override {
			cout << "Starting Compiling" << endl;

			string vertexCode;
			string fragmentCode;

			ifstream vShader;
			ifstream fShader;

			vShader.exceptions(ifstream::failbit | ifstream::badbit);
			fShader.exceptions(ifstream::failbit | ifstream::badbit);

			//Read Files
			try {
				vShader.open("./assets/shaders/" + vP);
				fShader.open("./assets/shaders/" + fP);

				stringstream vShaderStream, fShaderStream;
				vShaderStream << vShader.rdbuf();
				fShaderStream << fShader.rdbuf();

				vShader.close();
				fShader.close();

				vertexCode = vShaderStream.str();
				fragmentCode = fShaderStream.str();
			} catch (ifstream::failure e) {
				cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n" <<endl;
			}
			const char* vertexCString = vertexCode.c_str();
			const char* fragmentCString = fragmentCode.c_str();


			//Compile Shaders
			unsigned int vertex, fragment;
			int success;
			char infoLog[512];

			//Vertex
			vertex = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertex,1,&vertexCString,NULL);
			glCompileShader(vertex);

			glGetShaderiv(vertex,GL_COMPILE_STATUS,&success);
			if (!success) {
				glGetShaderInfoLog(vertex,512,NULL,infoLog);
				cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
			}

			//fragment
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment,1,&fragmentCString,NULL);
			glCompileShader(fragment);

			glGetShaderiv(fragment,GL_COMPILE_STATUS,&success);
			if (!success) {
				glGetShaderInfoLog(fragment,512,NULL,infoLog);
				cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
			}

			
			//Program Creation
			ID = glCreateProgram();
			glAttachShader(ID,vertex);
			glAttachShader(ID,fragment);
			glLinkProgram(ID);

			glGetProgramiv(ID,GL_LINK_STATUS,&success);
			if (!success) {
				glGetShaderInfoLog(fragment,512,NULL,infoLog);
				cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
			}

			glDeleteShader(vertex);
			glDeleteShader(fragment);

			readyToRun = true;
		}
	};
	struct Compute : Shader {
		Compute (string computePath) : cP(computePath) {}
		string cP;

		void compile () override {
			string computeCode;

			ifstream cFile;

			cFile.exceptions(ifstream::failbit | ifstream::badbit);

			//Read Files
			try {
				cFile.open("./assets/shaders/" + cP);

				stringstream cFilStream, fShaderStream;
				cFilStream << cFile.rdbuf();

				cFile.close();

				computeCode = cFilStream.str();
			} catch (ifstream::failure e) {
				cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n" <<endl;
			}
			const char* computeSource = computeCode.c_str();


			//Compile
			GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
			glShaderSource(computeShader, 1, &computeSource, nullptr);
			glCompileShader(computeShader);

			// Check for compile errors
			GLint success;
			glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char infoLog[512];
				glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
				cerr << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
			}

			
			//Link
			ID = glCreateProgram();
			glAttachShader(ID, computeShader);
			glLinkProgram(ID);

			// Check for link errors
			glGetProgramiv(ID, GL_LINK_STATUS, &success);
			if (!success) {
				char infoLog[512];
				glGetProgramInfoLog(ID, 512, nullptr, infoLog);
				cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
		}
	};
}
struct Render {
	static vector<Render*> renderStructs;
	map<string,Shader::Shader*> shaders;
	thread* renderThread;
	GLFWwindow* window;
	int width, height;
	atomic<bool> loopBreaker{false};
	vector<function<void()>> tickQueue;

	struct Object {
		Object () = delete;
		Object (Render &r) : render(r), ready(false), flagReady(false), hidden(false), draw([](){}) {
			r.objects.push_back(this);
		};

		Render &render;
		
		static std::vector<Object*> objects;

		bool hidden;

		bool flagReady; //Ready to be pushed to buffer
		std::vector<float> vertices;
		std::vector<unsigned int> indices;
		std::vector<unsigned int> attr;


		bool ready; //Ready to be drawn
		unsigned int VAO, EBO, VBO;

		unsigned int totalIndices;
		unsigned int totalVertices;
		unsigned int attrPerVert;

		//Both must be called from Render Thread
		void updateBuffers () {
			if (!flagReady) return;
			cleanBuffers();

			float* v = vertices.data();
			unsigned int* i = indices.data();
			unsigned int* a = attr.data();

			totalIndices = indices.size();

			// Calculate attribute layout
			attrPerVert = 0; //Stride per Vertex
			unsigned int sums[attr.size()]; //offset per attribute
			for (int j = 0; j < attr.size(); j++) {
				sums[j] = attrPerVert;
				attrPerVert += a[j];
			}

			totalVertices = vertices.size() / attrPerVert;

			// Generate OpenGL buffers
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), v, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), i, GL_STATIC_DRAW);

			for (int j = 0; j < attr.size(); j++) {
				glVertexAttribPointer(j, a[j], GL_FLOAT, GL_FALSE, attrPerVert * sizeof(float), (void*)(sums[j] * sizeof(float)));
				glEnableVertexAttribArray(j);
			}

			glBindVertexArray(0);
			ready = true;
			flagReady = false;
		}
		void cleanBuffers () {
			if (!ready) return;
			ready = false;
			if (glIsVertexArray(VAO)) glDeleteVertexArrays(1, &VAO);
			if (glIsBuffer(VBO)) glDeleteBuffers(1, &VBO);
			if (glIsBuffer(EBO)) glDeleteBuffers(1, &EBO);
		}

		std::function<void()> draw;

		~Object () {
			cleanBuffers();
			
			//clear from objects vector
			auto pointer = find(render.objects.begin(), render.objects.end(), this);
			if (pointer != render.objects.end()) {
				render.objects.erase(pointer);
			}
		}
	};
	vector<Object*> objects;
	
	struct Script {
		Script () = delete;
		Script (Render& r, string name) : render(r), run([](){}) {
			r.scripts[name] = this;
		}
		function<void()> run;

		Render& render;
	};
	map<string, Script*> scripts;

	Render () : Render(800,600) {}
	Render (int w, int h) : width(w), height(h), renderThread(nullptr), window(nullptr) {
		renderStructs.push_back(this);
		renderThread = new thread(&init, this);
	}

	Shader::Shader* compileShader (string name, Shader::Shader* shader) {
		shaders[name] = shader;
		shaderQueue.push_back(shader);
		return shader;
	}

	~Render () {
		auto it = find(renderStructs.begin(), renderStructs.end(), this);
		if (it != renderStructs.end()) renderStructs.erase(it);
	}

	std::atomic<bool> scriptTicked {false}; //Tells render to reset keystroke buffer
	bool isKeyDown (int GLFW_key) { //Is the GLFW_KEY_--- being held down
		return keysDownArr[GLFW_key].load();
	}
	bool isKeyPressed (int GLFW_key) { //Has the GLFW_KEY_--- been pressed in the last tick
		return keysPressArr[GLFW_key].load();
	}
	bool isMouseBtnDown (int GLFW_MOUSE_BTN) { //Is the GLFW_MOUSE_BUTTON_--- being held down
		return mouseDownArr[GLFW_MOUSE_BTN].load();
	}
	bool isMouseBtnPressed (int GLFW_MOUSE_BTN) { //Has the GLFW_MOUSE_BUTTON_--- been pressed in the last tick
		return mousePressArr[GLFW_MOUSE_BTN].load();
	}
	glm::vec2 getMousePos () {
		return glm::vec2(mousePos[0].load(), mousePos[1].load());
	}
	bool cursorLocked = true;

	private:
		static void windowResizeCallback(GLFWwindow* window, int w, int h) { //for when the window gets resized
			Render* r = static_cast<Render*>(glfwGetWindowUserPointer(window));
			glViewport(0, 0, w, h);
			r->width = w;
			r->height = h;
		}

		//Shader Gen
		struct ShaderData {
			string name;
			Shader::Shader shader;
		};
		vector<Shader::Shader*> shaderQueue;
		void generateShaders () {
			for (Shader::Shader* s : shaderQueue) {
				s->compile();
			}
			shaderQueue.clear();
		}

		void init () {
			//Initialize
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //Set Version
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Use core version of OpenGL
			glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
			#if defined(DEBUG)
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
			#endif

			//Create GLFW window
			window = glfwCreateWindow(width, height, "Space Sim", NULL, NULL); //Size, title, monitor, shared recourses
			if (window == NULL) {
				cout << "Failed to create GLFW window" << endl;
				glfwTerminate();
				return;// -1;
			}
			glfwMakeContextCurrent(window);
			glfwSetWindowUserPointer(window,this);


			//Initialize GLAD
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				cout << "Failed to initialize GLAD" << endl;
				return;// -1;
			}
		
			#if defined(DEBUG)
			cout << "Enabling debug" << endl;
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			cout << "Set flags" << endl;
			glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
				cerr << "GL DEBUG: type=" << type << " severity=" << severity << " msg=" << message;
				GLint currentProgram = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
				std::cout << " Active Program ID: " << currentProgram << std::endl;
			}, nullptr);
			cout << "Added callback" << endl;
			#endif

			glEnable(GL_PROGRAM_POINT_SIZE);

			//Sets GL Viewport (camera)
			glViewport(0, 0, width, height);
			glfwSetFramebufferSizeCallback(window,windowResizeCallback); //assigns resize callback function

			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);

			// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// glfwSetCursorPos(window,0,0);

			loop();
		}
		void loop () {
			glClearColor(0.0,0.0,0.0,1.0);
			while (!(glfwWindowShouldClose(window) || loopBreaker.load())) {
				generateShaders();
				updateInput();

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				//Run scripts
				for (function<void()> f : tickQueue) { //single run functions
					f();
				}
				tickQueue.clear();
				for (auto& [name,s] : scripts) { //every tick
					s->run();
				}

				//Update and Draw meshes
				for (Object* o : objects) {
					if (o->flagReady) o->updateBuffers();
					if (!o->hidden) o->draw();
				}

				glfwSwapBuffers(window);
			}

			glfwTerminate();
		}


		//Inputs
		void updateInput () {
			glfwPollEvents();
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

			bool ticked = scriptTicked.load(); //If scripting ticked in the last frame
			if (ticked) {
				//reset things that are only for one tick
				for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++) {
					keysPressArr[i].store(false);
				}
				for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
					mousePressArr[i].store(false);
				}
			}
			for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++) {
				bool keyDownBefore = keysDownArr[i].load();
				if (glfwGetKey(Render::window, i) == GLFW_PRESS) {
					if (!keyDownBefore) { //Key pressed this frame
						keysPressArr[i].store(true);
					}
					keysDownArr[i].store(true);
				} else { 
					if (ticked) //Key down events are guaranteed to be present for atleast one tick | If script is running slowly, and a key has been pressed and released between ticks, keysDown should still be true for that tick
						keysDownArr[i].store(false);
				}
			}
			//Same for Mouse press
			for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
				bool mouseDownBefore = mouseDownArr[i].load();
				if (glfwGetMouseButton(Render::window, i) == GLFW_PRESS) {
					if (!mouseDownBefore) { //Mouse pressed this frame
						mousePressArr[i].store(true);
					}
					mouseDownArr[i].store(true);
				} else {
					if (ticked)
						mouseDownArr[i].store(false);
				}
			}

			//Get Mouse position
			double x;
			double y;
			glfwGetCursorPos(window, &x, &y);
			mousePos[0].store(x);
			mousePos[1].store(y);

			scriptTicked.store(false);
		}
		atomic<bool> keysDownArr[GLFW_KEY_LAST + 1];
		atomic<bool> keysPressArr[GLFW_KEY_LAST + 1];
		atomic<bool> mouseDownArr[GLFW_MOUSE_BUTTON_LAST + 1];
		atomic<bool> mousePressArr[GLFW_MOUSE_BUTTON_LAST + 1];
		atomic<double> mousePos[2];
};
vector<Render*> Render::renderStructs;

#endif