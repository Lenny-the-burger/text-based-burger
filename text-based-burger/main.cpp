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

#include "object_utils.h"
#include "game_object_handler.h"
#include "game_object.h"

#include "map_manager.h"

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

int native_x, native_y = 0;

const char* WINDOW_TITLE = "Text based burger";

float aspect_ratio = (float)window_width / (float)window_height;
float aspect_ratio_small = (float)SMALL_WINDOW_HEIGHT / (float)SMALL_WINDOW_WIDTH;

// Master ui handler
unique_ptr<UIHandler> ui;

// Master object handler
unique_ptr<ObjectsHandler> objects_handler;

// Master map manager
MapManager map_manager("gamedata\\maps\\testmap_temp.json");

double last_time = 0;
double frame_time = 0;

double camera_x = 0.0f;
double camera_y = 0.0f;

float mapz = 0.8f;
float map_z_fov = 90.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	window_width = width;
	window_height = height;

	aspect_ratio = (float)window_width / (float)window_height;
}

void processInput(GLFWwindow* window) {
	// input handling

	// f1 to show help menu

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
		ui = make_unique<UIHandler>("test_scene.json", CHAR_COLS, CHAR_ROWS / 2);
		ui->rerender_all();
	}

	// ctrl + f6 rerender all components
	if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		ui->rerender_all();
	}

	// ctrl + f12 to open ui error log
	if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		ui->toggle_error_log();
		ui->cls();
		ui->rerender_all();
	}

	// Set vars other systems need to update

	// Mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Convert from window space to native space
	float scale = (float)window_height / 480.0f; // more hard coded numbers oh yeah
	int offset_x = (window_width - 640 * scale) / 2;

	native_x = (xpos - offset_x) / scale;
	native_y = (ypos - 0) / scale;

	// Convert to character space
	mouse_char_x = (int)(native_x / CHAR_WIDTH);
	mouse_char_y = (int)(native_y / CHAR_HEIGHT);

	
	// Set time
	frame_time = glfwGetTime() - last_time;
	last_time = glfwGetTime();
}

void draw_imgui() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//ImGui::ShowDemoWindow(); // Show demo window! :)

	// Map z height slider
	ImGui::SliderFloat("map z", &mapz, -3.0f, 3.0f);

	// Map z fov slider
	ImGui::SliderFloat("map z fov", &map_z_fov, 30.0f, 150.0f);


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
	glfwSwapInterval(0); // Disable vsync

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
	Shader line_shader = Shader("vertex_lines.glsl", "fragment_lines.glsl", std::vector<std::string>(), 460);

	Shader pass_shader = Shader("vertex.glsl", "fragment_pass.glsl", std::vector<std::string>(), 460);

#pragma region General loading

	// Load font, this had to be done first and on the main thread
	vector<uint32_t> font_data = load_font("gamedata\\fonts\\font2.txt"); // Enter your font path here

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
	ui = make_unique<UIHandler>("gamedata\\ui\\test_scene.json", CHAR_COLS, CHAR_ROWS / 2);
	ui->rerender_all(); // Initial render

	// Load test map
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\testmap.json");


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


	// Line drawing vertex buffer and color buffer
	// Vertex buffer is just draw elements, unindexed. Monolithic buffer, so lines can be any random
	// object so no indeces, gldrawarrays(gl_lines)
	// Color buffer is array of 32 bit uints.

	// How many lines do you think youll want to ever draw:
	int MAX_LINES = 10000;

	float* line_verts = new float[MAX_LINES * 4 * 2]; // 4 vertices per line, 2 floats per vertex

	uint32_t* line_colors = new uint32_t[MAX_LINES]; // 1 color per line

	// Create line vertex buffer
	unsigned int line_VBO, line_VAO, line_color_SSBO;

	// Generate VAO
	glGenVertexArrays(1, &line_VAO);
	glBindVertexArray(line_VAO);

	// Generate and bind VBO
	glGenBuffers(1, &line_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, line_VBO);
	glBufferData(GL_ARRAY_BUFFER, MAX_LINES * 4 * sizeof(float), nullptr, GL_STREAM_DRAW); // Orphaned initially

	// Vertex layout: vec2 per vertex (x, y), 2 vertices per line = 4 floats
	// Just treat it as a flat array of vec2s
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	// Unbind VAO
	glBindVertexArray(0);

	// Color buffer � SSBO
	glGenBuffers(1, &line_color_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, line_color_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_LINES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, line_color_SSBO); // Binding = 1, match in GLSL
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1); // Bind to 1

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Set texture border color to black
	glm::vec4 border_color(0.0f, 0.0f, 0.0f, 1.0f);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

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

	//// Run through the grid and set a test pattern
	//for (int i = 0; i < NUM_CHARS; i++) {
	//	uint32_t c = 0;
	//	// Set characters mod 255
	//	c |= (i % 255);

	//	// Set fg to random color
	//	c |= (rand() % 255) << 8;

	//	// Set bg to random color
	//	c |= (rand() % 255) << 16;

	//	char_grid[i] = c;
	//}

	unsigned int char_grid_buffer;

	glGenBuffers(1, &char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_CHARS * sizeof(uint32_t), char_grid, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	// Set this every frame how many lines were drawing this time
	int num_lines = 0;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		UIUpdateData frame_data;
		frame_data.mouse_char_x = mouse_char_x;
		frame_data.mouse_char_y = mouse_char_y;
		frame_data.time = (int) glfwGetTime();
		frame_data.is_clicking = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		// Rendering starts here

		draw_imgui();
		set_uniforms();

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT); // Match the framebuffer size
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

		raster_shader.use();

		// For now just update all the ui and plop the whole screen on the gpu, 
		// this will eventually happen on a sperate thread (probably)
		ui->update(frame_data);
		//ui->rerender_all(); // for now just rerender all

		// For now just blindly copy the screen to the char grid
		vector<vector<uint32_t>> screen = ui->get_screen();
		for (int i = 0; i < CHAR_ROWS / 2; i++) {
			for (int j = 0; j < CHAR_COLS; j++) {
				char_grid[i * CHAR_COLS + j] = screen[i][j];
			}
		}

		// --- update objects ---
		ObjectUpdateData update_data;
		update_data.time = glfwGetTime();
		update_data.frame_time = frame_time;
		update_data.mouse_pos = vec2(native_x, native_y);
		update_data.is_clicking = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		update_data.camera_pos = vec2(camera_x, camera_y);

		update_data.mapz = mapz;
		update_data.map_z_fov = map_z_fov;

		// Should probably not reset this every frame, but its a pointer so whatever
		update_data.window = window;

		ObjectUpdateReturnData return_data;

		// Call update
		return_data = objects_handler->update(update_data);

		// Update camera position from ovjects handler
		update_data.camera_pos = return_data.camera_pos;

		// Render map first
		map_manager.update(update_data);

		num_lines = map_manager.render(line_verts, line_colors);

		// Render objects
		num_lines = objects_handler->render(line_verts, line_colors, num_lines);


		// The amount of data we send to the gpu is only 7.2 kb, if you want to optimize
		// this then go right ahead if you want another 1 fps over the 1000 you already get

		// Copy the screen to the gpu
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_CHARS * sizeof(uint32_t), char_grid);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct 		// Draw the quad


		// Finally unbind the small framebuffer
		// ---- ALL IN SOFTWARE RAsTER ELEMENTS MUST BE DRAWN ABOVE THIS LINE ----
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


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

		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct              // Draw the quad

		// Draw electron beam lines, these are not rasterized so draw them at screen resolution

		line_shader.use();

		// Set uniforms
		line_shader.setFloat("aspectRatio", aspect_ratio);
		line_shader.setFloat("aspectRatioSmall", aspect_ratio_small);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glLineWidth(2.0f);
		glEnable(GL_LINE_SMOOTH);

		// Upload line vertex data
		glBindBuffer(GL_ARRAY_BUFFER, line_VBO);
		glBufferData(GL_ARRAY_BUFFER, MAX_LINES * 4 * sizeof(float), nullptr, GL_STREAM_DRAW); // Orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_lines * 4 * sizeof(float), line_verts);        // Upload only used

		// Upload line color data
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, line_color_SSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_LINES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num_lines * sizeof(uint32_t), line_colors);

		// Bind the SSBO to binding point 1
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, line_color_SSBO);

		// Setup VAO
		glBindVertexArray(line_VAO);

		// Draw lines, each line has two verts
		glDrawArrays(GL_LINES, 0, num_lines * 2);

		// Unbind for cleanliness
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDisable(GL_BLEND); // Disable blending for next draw calls

		// Draw Dear ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;

}
