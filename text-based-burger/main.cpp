#include <Windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>

#include "shader.h"

int WINDOW_WIDTH = 1482;
int WINDOW_HEIGHT = 777;

// Character dimensions
int CHAR_WIDTH = 8;
int CHAR_HEIGHT = 16;
int CHAR_RATIO = CHAR_HEIGHT / CHAR_WIDTH;

// Character columns on screen
int CHAR_COLS = 80;
int CHAR_ROWS = (CHAR_COLS / 4) * 3; // 4:3 aspect ratio

int SMALL_WINDOW_WIDTH = CHAR_WIDTH * CHAR_COLS;
int SMALL_WINDOW_HEIGHT = (CHAR_ROWS * CHAR_HEIGHT) / CHAR_RATIO;

float target_scale = (float)SMALL_WINDOW_HEIGHT / (float)WINDOW_HEIGHT;

const char* WINDOW_TITLE = "Text based burger";

float aspect_ratio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
float aspect_ratio_small = (float)SMALL_WINDOW_HEIGHT / (float)SMALL_WINDOW_WIDTH;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;

	aspect_ratio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
	target_scale = (float)SMALL_WINDOW_HEIGHT / (float)height;
}

void processInput(GLFWwindow* window) {
	// input handling

	// f11 to toggle fullscreen
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (glfwGetWindowMonitor(window) == NULL) {
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else {
			glfwSetWindowMonitor(window, NULL, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
		}
	}
}

static float scale = 1.0f;

static float translation[2] = { 0.0f, 0.0f };

void draw_ui() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//ImGui::ShowDemoWindow(); // Show demo window! :)

	// Render target scale slider
	ImGui::SliderFloat("Scale", &scale, 0.1f, 10.0f);

	// Render target translation sliders
	ImGui::SliderFloat("Translation X", &translation[0], -1.0f, 1.0f);
	ImGui::SliderFloat("Translation Y", &translation[1], -1.0f, 1.0f);
	
	return;
}

void set_uniforms() {
	// Set uniforms here
}

// Create a window for 2d rendering

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);



	// Compile shaders
	Shader raster_shader = Shader("vertex.glsl", "fragment.glsl", std::vector<std::string>(), 330);
	Shader pass_shader = Shader("vertex.glsl", "fragment_pass.glsl", std::vector<std::string>(), 330);


	// Vertex is simple we only render a screen quad

	// Full-screen quad vertices and indices
	float vertices[] = {
		// Positions   // Texture Coords
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
		-1.0f,  1.0f,  0.0f, 1.0f
	};

	unsigned int indices[] = {
		0, 1, 2,   // First triangle
		0, 2, 3    // Second triangle
	};

	// Create and bind VAO and VBO
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// Bind vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Set vertex attributes
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); // Position

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1); // Texture Coordinates

	// Unbind VAO
	glBindVertexArray(0);


	// Frame buffern stuff

	// Create framebuffer
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// Create texture for framebuffer attachment
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	// Set nearest-neighbor filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Attach texture to the framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer




	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		draw_ui();
		set_uniforms();

		pass_shader.setFloat("aspectRatio", aspect_ratio);
		pass_shader.setFloat("aspectRatioSmall", aspect_ratio_small);
		pass_shader.setFloat("target_scale", target_scale);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT); // Match the framebuffer size
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

		raster_shader.use();
		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct 		// Draw the quad
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

		// Second Pass: Render to the screen (nearest-neighbor upscale)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);			// Default framebuffer (screen)
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);	// Fullscreen viewport
		glClear(GL_COLOR_BUFFER_BIT);					// Clear screen
		// Set frambuffer generatred prev as screenTexture uniform
		pass_shader.setInt("screenTexture", 0);
		glBindTexture(GL_TEXTURE_2D, texture);			// Bind the framebuffer texture
		pass_shader.use();						       // Use passthrough shader
		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct              // Draw the quad



		// Draw Dear ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;

}
