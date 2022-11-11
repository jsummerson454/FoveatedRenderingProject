#define STB_IMAGE_IMPLEMENTATION // image loading library

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>
#include "shader.h"
#include "FlyCamera.h"
#include "Mesh.h"
#include "Scene.h"
#include "stb_image.h"

//REMEMBER TO CHANGE IN FRAGMENT SHADER TOO WHEN ALTERING NUMBER OF POINT LIGHT SOURCES
#define NUM_LIGHTS 10

//REMEMBER TO ALTER BLENDING FRAGMENT SHADER TOO WHEN ALTERING NUMBER OF LAYERS
#define NUM_LAYERS 3

//REMEMBER TO KEEP THIS CONSISTENT WITH THE VERTEX SHADER
#define INSTANCES 20

//THIS SHOULD BE AVOIDED - slows renderer down by syncing GPU and CPU with glFinish() calls, but provides ms/draw call timings
//#define DRAW_TIMING

//MSAA samples, remove definition entirely to disable MSAA
#define SAMPLES 4

bool FOVEATION_ENABLED = true;
bool UPDATE_PROJECTION = false;
double DELTA_T = 0.0;
int WIDTH = 0, HEIGHT = 0;

int main();

//function declarations
//void framebuffer_size_callback_function(GLFWwindow*, int, int); - not necessary, using fixed size fullscreen window
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
#ifdef SAMPLES
void generate_multisample_eccentricity_framebuffer(unsigned int* framebuffer, int width, int height, int samples);
void generate_intermediate_framebuffer(unsigned int* framebuffer, unsigned int* texture, int width, int height);
#else
void generate_eccentricity_framebuffer(unsigned int* framebuffer, unsigned int* texture, int width, int height);
#endif

glm::vec3 getColour(int i) {
	switch (i%6) {
	case 0:
		return glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	case 1:
		return glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 2:
		return glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	case 3:
		return glm::vec3(1.0f, 0.0f, 1.0f);
		break;
	case 4:
		return glm::vec3(0.0f, 1.0f, 1.0f);
		break;
	case 5:
		return glm::vec3(1.0f, 1.0f, 0.0f);
		break;
	}
};

glm::vec3 randomColour() {
	return glm::vec3((float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX);
}

//Default cam
//FlyCamera cam(glm::vec3(0.0f, 0.0f, 3.0f));

//Parameters for high poly dragon test
//FlyCamera cam(glm::vec3(-1.537668, 1.067694, -0.516135), -343.499664, -30.099977, 44.687038);

//Parameters for low poly dragon test
//FlyCamera cam(glm::vec3(-1.811132, 1.207838, -0.584567), -344.599731, -29.599981, 44.687038);

//Parameters for cityscape
FlyCamera cam(glm::vec3(-3.000140, 1.453398, -2.767532), -670.001526, -20.000036, 31.015045);


int main() {
	// ---------- INITIALISATION  ----------
	stbi_set_flip_vertically_on_load(true);
	std::srand(1);

	//initalise glfw
	if (!glfwInit()) {
		std::cout << "Error initalising glfw" << std::endl;
	}

	//configuring glfw (hints set for next call of glfwCreateWindow)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef SAMPLES
	glfwWindowHint(GLFW_SAMPLES, SAMPLES);
#endif

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	//by deafult I use the last monitor in the list, even though the primary monitor is the first element of this list
	//since I want it on my external monitor, not my laptop screen

	const GLFWvidmode* mode = glfwGetVideoMode(monitors[count - 1]);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	//Create the window and check success
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Foveated Rendering", monitors[count - 1], NULL);
	if (window == NULL) {
		std::cout << "Error creating glfw window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwGetFramebufferSize(window, &WIDTH, &HEIGHT);

	//make window the main context
	glfwMakeContextCurrent(window);
	//binding callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	//capture cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//use GLAD to load openGL function pointers before trying to use openGL functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Error initalising GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	Shader mainShader("vertexShader.gl", "fragmentShader.gl");
	Shader lightShader("lightVertexShader.gl", "lightFragmentShader.gl");
	
	mainShader.use();
	//binding textures to uniforms, see Mesh::draw()
	mainShader.setInt("diffuseMap", 0); //GL_TEXTURE0
	mainShader.setInt("specularMap", 1); //GL_TEXTURE1

	Scene scene("Resources\\buildings\\buildings.obj");

	// ----------- LIGHTING -----------
	//global illumination
	//globalLightDir defined as light direction away from the source (ie the direction the light is shining)
	//glm::vec3 globalLightDir(0.0f, -1.0f, -0.5f); // DEFAULT lighting
	glm::vec3 globalLightDir(0.0f, -1.0f, 0.5f); // CITYSCAPE lighting
	glm::vec3 globalLightCol(1.0f, 1.0f, 1.0f);
	
	mainShader.setVec3f("globalLight.direction", globalLightDir);
	//DEFAULT LIGHTING:
	//mainShader.setVec3f("globalLight.ambient", globalLightCol * 0.2f);
	//mainShader.setVec3f("globalLight.diffuse", globalLightCol * 0.7f);
	//mainShader.setVec3f("globalLight.specular", globalLightCol * 1.0f);
	//CITYSCAPE LIGHTING:
	mainShader.setVec3f("globalLight.ambient", globalLightCol * 0.0f);
	mainShader.setVec3f("globalLight.diffuse", globalLightCol * 0.2f);
	mainShader.setVec3f("globalLight.specular", globalLightCol * 1.0f);

	//point lights
	glm::vec3 pointLightPosCol[NUM_LIGHTS*2];
	for (int i = 0; i < NUM_LIGHTS; i++) {
		//CITYSCAPE (ellipse) POSITIONS:
		float angle = glm::radians((float)(360 * i) / NUM_LIGHTS);
		pointLightPosCol[2*i] = glm::vec3(sin(angle) * 2.5f, 1.5f, cos(angle) * 3.5f);

		//colours:
		//pointLightPosCol[2 * i + 1] = getColour(i);
		pointLightPosCol[2 * i + 1] = randomColour();
		//pointLightPosCol[2 * i + 1] = glm::vec3(1.0f);
	}
	//attenuation coefficients
	float pointLightConstant = 1.0f;
	float pointLightLinear = 0.22f;
	float pointLightQuadratic = 0.20f;


	for (int i = 0; i < NUM_LIGHTS; i++) {
		mainShader.setVec3f(("lights[" + std::to_string(i) + "].pos").c_str(), pointLightPosCol[2 * i]);
		mainShader.setVec3f(("lights[" + std::to_string(i) + "].diffuse").c_str(), pointLightPosCol[2*i + 1] * 1.0f);
		mainShader.setVec3f(("lights[" + std::to_string(i) + "].specular").c_str(), pointLightPosCol[2*i + 1] * 1.0f);
		mainShader.setFloat(("lights[" + std::to_string(i) + "].constant").c_str(), pointLightConstant);
		mainShader.setFloat(("lights[" + std::to_string(i) + "].linear").c_str(), pointLightLinear);
		mainShader.setFloat(("lights[" + std::to_string(i) + "].quadratic").c_str(), pointLightQuadratic);
	}

	// setting up buffers and shaders for rendering the light sources as points
	unsigned int light_vbo, light_vao;
	glGenBuffers(1, &light_vbo);
	glGenVertexArrays(1, &light_vao);

	glBindVertexArray(light_vao);
	glBindBuffer(GL_ARRAY_BUFFER, light_vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(pointLightPosCol), pointLightPosCol, GL_STATIC_DRAW);

	glVertexAttribPointer(lightShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(lightShader.getAttributeLocation("pos"));

	glVertexAttribPointer(lightShader.getAttributeLocation("inCol"), 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(sizeof(float)*3));
	glEnableVertexAttribArray(lightShader.getAttributeLocation("inCol"));

	// -------- FOVEATION SPECIFIC SETUP --------
	//Foveated rendering specific setup (framebuffers, textures, single quad vao etc)
	Shader blendingShader("blendingVertexShader.gl", "blendingFragmentShader.gl");

	//Sizes define how much of the (full resolution) screen the layer covers. The base layer should always cover the full screen (defined in pixels)
	int sizes[NUM_LAYERS * 2] = {
		WIDTH, HEIGHT, // BASE LAYER SHOULD ALWAYS COVER FULL SCREEN
		900, 900,
		250, 250,
	};
	//Resolutions defined using the resolution of the layer (resolution shouldn't be larger than its size, since that would go over the screen's resolution)
	//
	int resolutions[NUM_LAYERS * 2] = {
		WIDTH/3, HEIGHT/3,
		450, 450,
		sizes[NUM_LAYERS*2 - 2], sizes[2*NUM_LAYERS - 1] // FOVEA LAYER SHOULD ALWAYS BE AT NATIVE SCREEN RESOLUTION
	};
	//THESE SHOULD ALWAYS BE SQUARE (EXCEPT BASE LAYER) DUE TO CIRCULAR BLENDING, OTHERWISE JUST WASTED COMPUTATION

#ifdef SAMPLES
	glEnable(GL_MULTISAMPLE);
	unsigned int multisampleFBs[NUM_LAYERS], intermediateFBs[NUM_LAYERS], intermediateFBtextures[NUM_LAYERS];
	for (int i = 0; i < NUM_LAYERS; i++) {
		generate_multisample_eccentricity_framebuffer(&multisampleFBs[i], resolutions[2 * i], resolutions[2 * i + 1], SAMPLES);
		generate_intermediate_framebuffer(&intermediateFBs[i], &intermediateFBtextures[i], resolutions[2 * i], resolutions[2 * i + 1]);
	}
#else
	unsigned int framebufferIDs[NUM_LAYERS], framebufferTextureIDs[NUM_LAYERS];
	for (int i = 0; i < NUM_LAYERS; i++) {
		generate_eccentricity_framebuffer(&framebufferIDs[i], &framebufferTextureIDs[i], resolutions[2 * i], resolutions[2 * i + 1]);
	}
#endif
	

	//now setup buffers for the single quad that the texture will be rendered onto
	unsigned int quadVAO, quadVBO;
	float quad[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	glGenBuffers(1, &quadVBO);
	glGenVertexArrays(1, &quadVAO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);

	glVertexAttribPointer(blendingShader.getAttributeLocation("inPos"), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(blendingShader.getAttributeLocation("inPos"));

	glVertexAttribPointer(blendingShader.getAttributeLocation("inTexCoords"), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
	glEnableVertexAttribArray(blendingShader.getAttributeLocation("inTexCoords"));

	blendingShader.use();

	blendingShader.setVec2f("screenSize", glm::vec2(WIDTH, HEIGHT));
	blendingShader.setInt("textures[0]", 0);

	//skip the base layer, we don't need boundaries for it as it covers the full screen
	for (int i = 1; i < NUM_LAYERS; i++) {
		glm::vec4 vec;
		vec.x = (float)(WIDTH - sizes[2 * i]) / (2 * WIDTH);
		vec.y = (float)(WIDTH + sizes[2 * i]) / (2 * WIDTH);
		vec.z = (float)(HEIGHT - sizes[2 * i + 1]) / (2 * HEIGHT);
		vec.w = (float)(HEIGHT + sizes[2 * i + 1]) / (2 * HEIGHT);

		blendingShader.setVec4f(("boundaries[" + std::to_string(i - 1) + "]").c_str(), vec);
		blendingShader.setInt(("textures[" + std::to_string(i) + "]").c_str(), i);
	}


	// -------- RENDER LOOP --------

	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glPointSize(10.0f);

	glm::mat4 model[INSTANCES];
	glm::mat3 normalMatrix[INSTANCES];

	//Default setup for rendering a single instance
	//model[0] = glm::mat4(1.0f);

	// For low poly dragon - vertex positions are huge for some reason, also dragon is pointing upwards
	//model[0] = glm::scale(model[0], glm::vec3(0.007f));
	//model[0] = glm::rotate(model[0], glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));

	// For high poly dragon - model is a bit small, so scale up slightly
	//model[0] = glm::scale(model[0], glm::vec3(2.0f));

	for (int i = 0; i < INSTANCES; i++) {
		//FOR CITYSCAPE:
		glm::mat4 m = glm::mat4(1.0f);
		model[i] = glm::scale(glm::translate(m, glm::vec3((i % 4 - 2.0f) * 0.5f, 0.0f, (-i + INSTANCES / 2.0f) * 0.3f)), glm::vec3(0.001f));

		normalMatrix[i] = glm::mat3(glm::transpose(glm::inverse(model[i])));
	}
	
	glm::mat4 projection = glm::perspective(glm::radians(cam.fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

	mainShader.use();
	for (int i = 0; i < INSTANCES; i++) {
		normalMatrix[i] = glm::mat3(glm::transpose(glm::inverse(model[i])));
		mainShader.setMat4f(("model["+std::to_string(i)+"]").c_str(), &model[i][0][0]);
		mainShader.setMat3f(("normalMatrix["+std::to_string(i)+"]").c_str(), &normalMatrix[i][0][0]);
	}
	

	// Per frame timing (for delta_t, needed so camera movement speed is not tied to framerate)
	double previousTime = 0.0;

	#ifdef DRAW_TIMING
	// Timings to calculate ms/draw call
	double drawTimer = 0.0;
	int numFramesDraw = 0;
	#endif

	// Timings to calculate ms/frame
	double lastTime = 0.0;
	int numFrames = 0;

	glfwSetTime(0.0);
	//render loop
	while (!glfwWindowShouldClose(window)) {
		//usually want to clear the screen at start of new frame, clearing to set colour in this caese to check everything works AND CLEAR DEPTH BUFFER
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = cam.getViewMatrix();
		if (UPDATE_PROJECTION) {
			projection = glm::perspective(glm::radians(cam.fov), (float)WIDTH/HEIGHT, 0.1f, 100.0f);
			UPDATE_PROJECTION = false;
		}

		mainShader.use();

		glm::mat4 VP = projection * view;
		glm::mat4 MVP[INSTANCES];
		for (int i = 0; i < INSTANCES; i++) {
			MVP[i] = VP * model[i];
			mainShader.setMat4f(("MVP[" + std::to_string(i) + "]").c_str(), &MVP[i][0][0]);
		}
		
		mainShader.setVec3f("camPos", cam.camPos);
		
		//light positions already defined in world coordinates, so only need view and projection matrices
		lightShader.use();
		lightShader.setMat4f("VP", &VP[0][0]);
		
		#ifdef DRAW_TIMING
		glFinish();
		double startDraw = glfwGetTime();
		#endif
		if (FOVEATION_ENABLED) {
			#ifdef SAMPLES
			scene.drawFoveatedMultisample(mainShader, blendingShader, multisampleFBs, intermediateFBs, intermediateFBtextures, resolutions, sizes, NUM_LAYERS, quadVAO, INSTANCES);
			#else
			scene.drawFoveated(mainShader, blendingShader, framebufferIDs, framebufferTextureIDs, resolutions, sizes, NUM_LAYERS, quadVAO, INSTANCES);
			#endif
		}
		else {
			scene.draw(mainShader, INSTANCES);
			//draw the point lights (mainly used as debugging tool/checking lights are in correct positions relative to objects)
			// can't really get a good sense of the light positions from screenshots as they are rendered as fixed size points, ideally
			// need to be moving around the scene for this to be useful
			lightShader.use();
			glBindVertexArray(light_vao);
			glDrawArrays(GL_POINTS, 0, NUM_LIGHTS);
		}
        #ifdef DRAW_TIMING
		glFinish();
		double endDraw = glfwGetTime();

		drawTimer += (endDraw - startDraw);
		numFramesDraw++;
		if (drawTimer >= 5.0) {
			printf("%f ms/draw\n", 5000.0 / double(numFramesDraw));
			numFramesDraw = 0;
			drawTimer -= 5.0;
		}	
		#endif

		glfwSwapBuffers(window); //double buffer
		glfwPollEvents(); //check for event triggers and calls corresponding callback functions

		double currentTime = glfwGetTime();
		DELTA_T = currentTime - previousTime;
		previousTime = currentTime;
		
		numFrames++;
		if (currentTime - lastTime >= 5.0) {
			printf("%f ms/frame\n", 5000.0 / double(numFrames));
			numFrames = 0;
			lastTime += 5.0;
		}

		cam.processKeyboardInput(window, DELTA_T);
	}

	//window instructed to close, so close successfully
	glfwTerminate();
	return 0;
}

//callback function to handle user input (toggles, events on key holds - ie camera movements - are done on every render loop iteration), not the callback
//(done since GLFW_HOLD has a delay before activating, making it not suitable for camera movements)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		GLint result[2];
		glGetIntegerv(GL_POLYGON_MODE, result);
		if (result[1] == GL_LINE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	}
	else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		FOVEATION_ENABLED = !FOVEATION_ENABLED;
		std::cout << "Swapped rendering method (disregard next timing result)" << std::endl;
	}
	else if (key == GLFW_KEY_P && action == GLFW_REPEAT) {
		cam.fov += 200.0f * DELTA_T;
		if (cam.fov >= 90.f) {
			cam.fov = 90.0f;
		}
		UPDATE_PROJECTION = true;
	}
	else if (key == GLFW_KEY_O && action == GLFW_REPEAT) {
		cam.fov -= 200.0f * DELTA_T;
		if (cam.fov <= 15.f) {
			cam.fov = 15.0f;
		}
		UPDATE_PROJECTION = true;
	}
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		cam.printParameters();
	}
}

float lastMouseX, lastMouseY;
bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
		return;
	}

	cam.processMouseMovement(window, xpos - lastMouseX, lastMouseY - ypos);
	lastMouseX = xpos;
	lastMouseY = ypos;
}



#ifdef SAMPLES
void generate_multisample_eccentricity_framebuffer(unsigned int* framebuffer, int width, int height, int samples) {
	//modification of generate_eccentricity_framebuffer that now attaches multisampled texture and depth attachments
	//set up framebuffer object
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer); //subsequent read and write framebuffer operations will now affect our newly created framebuffer object

	//generate and attach MULTISAMPLE texture as colour attachment to framebuffer
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

	//attach MULTISAMPLE renderbuffer object for depth testing
	unsigned int renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

	//check framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer is not complete!" << std::endl;
	}
}

void generate_intermediate_framebuffer(unsigned int* framebuffer, unsigned int* texture, int width, int height) {
	//only need a FBO with a colour attachment to blit (resolve) to then read from in blending shader
	//very similar to generate_eccentricity_framebuffer but without the renderbuffer depth attachment

	//set up framebuffer object
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer); //subsequent read and write framebuffer operations will now affect our newly created framebuffer object

	//generate and attach texture as colour attachment to framebuffer
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//need to set these as we sample from the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);

	//check framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer is not complete!" << std::endl;
	}

}
#else
void generate_eccentricity_framebuffer(unsigned int* framebuffer, unsigned int* texture, int width, int height) {
	//set up framebuffer object
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer); //subsequent read and write framebuffer operations will now affect our newly created framebuffer object

	//generate and attach texture as colour attachment to framebuffer
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//need to set these as we sample from the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);

	//attach renderbuffer object for depth testing
	unsigned int renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

	//check framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer is not complete!" << std::endl;
	}
}
#endif