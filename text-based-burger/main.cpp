#include <Windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>

#include "shader.h"
#include "font_loader.h"
#include "component.h"
#include "ui_handler.h"

using namespace std;

// These are not constants 
int window_width = 1200;
int window_height = 600;

// Character dimensions
int CHAR_WIDTH = 8;
int CHAR_HEIGHT = 16;
int CHAR_RATIO = CHAR_HEIGHT / CHAR_WIDTH;

// Character columns on screen
int CHAR_COLS = 80;
int CHAR_ROWS = (CHAR_COLS / 4) * 3; // 4:3 aspect ratio

int SMALL_WINDOW_WIDTH = CHAR_WIDTH * CHAR_COLS;
int SMALL_WINDOW_HEIGHT = (CHAR_ROWS * CHAR_HEIGHT) / CHAR_RATIO;

// Mouse position in character space
int mouse_char_x = 0;
int mouse_char_y = 0;

float target_scale = (float)SMALL_WINDOW_HEIGHT / (float)window_height;

const char* WINDOW_TITLE = "Text based burger";

float aspect_ratio = (float)window_width / (float)window_height;
float aspect_ratio_small = (float)SMALL_WINDOW_HEIGHT / (float)SMALL_WINDOW_WIDTH;

// How many z layers exist
int Z_LAYERS = 4;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	window_width = width;
	window_height = height;

	aspect_ratio = (float)window_width / (float)window_height;
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
			glfwSetWindowMonitor(window, NULL, 100, 100, window_width, window_height, 0);
		}
	}

	// ctrl + f5 reload the ui
	if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		// Reload the ui
	}

	// Mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	//xpos = ImGui::GetMousePos().x;
	//ypos = ImGui::GetMousePos().y;

	// Convert to character space

	// Y position is simple because we can just divide by the character rows

	mouse_char_y = (int)floor((ypos * CHAR_ROWS) / window_height);

	// X position we need to figure out how many character could fit in the window
	// We could do complex scaling math but we know the character ratio so go by that
	int char_pixels_side = (window_height / CHAR_ROWS) / CHAR_RATIO;

	mouse_char_x = (int)floor(xpos / char_pixels_side);

	mouse_char_x /= 2;
	mouse_char_y /= 2;

	// Offset for the window border
	// How many characters could fit in the window
	int char_x_max = window_width / char_pixels_side;
	mouse_char_x -= (char_x_max - CHAR_COLS) / 8;
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

	// Because imgui hijacks things thanks imgui
	ImGui::SetMouseCursor(ImGuiMouseCursor_None);
	
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

	GLFWwindow* window = glfwCreateWindow(window_width, window_height, WINDOW_TITLE, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

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

	glViewport(0, 0, window_width, window_height);

	// Compile shaders
	Shader raster_shader = Shader("vertex.glsl", "fragment.glsl", std::vector<std::string>(), 460);
	Shader pass_shader = Shader("vertex.glsl", "fragment_pass.glsl", std::vector<std::string>(), 460);

#pragma region General loading

	// Load font, this had to be done first and on the main thread
	vector<uint32_t> font_data = load_font("assets\\font2.txt"); // Enter your font path here

	// Bind the main shader and set uniform
	raster_shader.use();

	// Uniform array of uint32_t
	uint32_t* font_data_array = new uint32_t[1024];

	// Copy font data to the array
	for (int i = 0; i < font_data.size(); i++) {
		font_data_array[i] = font_data[i];
	}

	// Set the uniform
	glUniform4uiv(glGetUniformLocation(raster_shader.ID, "glyphs"), 256, font_data_array);


	// Load ui
	UIHandler ui = UIHandler("test_scene.json", CHAR_COLS, CHAR_ROWS);


#pragma endregion

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

	// Character grid buffer
	// Holds CHAR_COLS by CHAR_ROWS characters
	// Stores uints for character and color
	// First 8 bits are character, next 8 are color, next 8 are background color, and 8 unused
	// ordering is done on the cpu to avoid branching.

	int NUM_CHARS = CHAR_COLS * CHAR_ROWS;

	uint32_t* char_grid = new uint32_t[NUM_CHARS];

	// Run through the grid and set a test pattern
	for (int i = 0; i < NUM_CHARS; i++) {
		uint32_t c = 0;
		// Set characters mod 255
		c |= (i % 255);

		// Set fg to random color
		c |= (rand() % 255) << 8;

		// Set bg to random color
		c |= (rand() % 255) << 16;

		char_grid[i] = c;
	}

	unsigned int char_grid_buffer;

	glGenBuffers(1, &char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_CHARS * sizeof(uint32_t), char_grid, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);



	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		update_data frame_data;
		frame_data.mouse_char_x = mouse_char_x;
		frame_data.mouse_char_y = mouse_char_y;
		frame_data.time = (int) glfwGetTime();
		frame_data.is_clicking = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		ui.update(update_data());

		// Rendering starts here

		draw_ui();
		set_uniforms();

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT); // Match the framebuffer size
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

		raster_shader.use();

		// Set mouse position uniform so dont have to waste a z layer for it
		raster_shader.setInt("mouse_char_x", mouse_char_x);
		raster_shader.setInt("mouse_char_y", mouse_char_y);

		// For now just update all the ui and plop the whole screen on the gpu, 
		// this will eventually happen on a sperate thread (probably)
		ui.update(frame_data);
		ui.rerender_all(); // for now just rerender all

		// For now just blindly copy the screen to the char grid
		vector<vector<uint32_t>> screen = ui.get_screen();
		for (int i = 0; i < CHAR_ROWS; i++) {
			for (int j = 0; j < CHAR_COLS; j++) {
				char_grid[i * CHAR_COLS + j] = screen[i][j];
			}
		}

		// Copy the screen to the gpu
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_CHARS * sizeof(uint32_t), char_grid);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct 		// Draw the quad
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

		// Second Pass: Render to the screen (nearest-neighbor upscale)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);			// Default framebuffer (screen)
		glViewport(0, 0, window_width, window_height);	// Fullscreen viewport
		glClear(GL_COLOR_BUFFER_BIT);					// Clear screen
		// Set frambuffer generatred prev as screenTexture uniform
		pass_shader.setInt("screenTexture", 0);
		glBindTexture(GL_TEXTURE_2D, texture);			// Bind the framebuffer texture

		pass_shader.use();						       // Use passthrough shader

		// Set uniforms
		pass_shader.setFloat("aspectRatio", aspect_ratio);
		pass_shader.setFloat("aspectRatioSmall", aspect_ratio_small);
		pass_shader.setFloat("target_scale", target_scale);

		pass_shader.setFloat("scale", scale);
		pass_shader.setFloat("translationX", translation[0]);
		pass_shader.setFloat("translationY", translation[1]);

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
