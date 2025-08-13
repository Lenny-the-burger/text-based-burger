#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>

#include "shader.h"
#include "font_loader.h"

#include "systems_controller.h"

using namespace std;

// These are not constants 
int window_width = 1300;
int window_height = 720;

// Character dimensions
int CHAR_WIDTH = 8;
int CHAR_HEIGHT = 16;
int CHAR_RATIO = CHAR_HEIGHT / CHAR_WIDTH;

// Character columns on screen
int CHAR_COLS = 120;
int CHAR_ROWS = 68;

// 960 * 544
int SMALL_WINDOW_WIDTH = CHAR_WIDTH * CHAR_COLS;
int SMALL_WINDOW_HEIGHT = (CHAR_ROWS * CHAR_HEIGHT) / CHAR_RATIO;

// Mouse position in character space
int mouse_char_x = 0;
int mouse_char_y = 0;

float scroll_x = 0.0f, scroll_y = 0.0f;

int native_x, native_y = 0;

const char* WINDOW_TITLE = "Text based burger";

float aspect_ratio = (float)window_width / (float)window_height;
float aspect_ratio_small = (float)SMALL_WINDOW_HEIGHT / (float)SMALL_WINDOW_WIDTH;

unique_ptr<SystemsController> systems_controller;

double last_time = 0;
double frame_time = 0;

float dval1 = 1.0f;

//GLuint fullTex;
GLuint compositeTex;
GLuint depthStencilRBO;
//GLuint blurTex;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	window_width = width;
	window_height = height;

	aspect_ratio = (float)window_width / (float)window_height;

	// Resize fbos that are not the native one
	/*glBindTexture(GL_TEXTURE_2D, fullTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);*/

	glBindTexture(GL_TEXTURE_2D, compositeTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Resize depth-stencil renderbuffer
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	/*glBindTexture(GL_TEXTURE_2D, blurTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);*/
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	scroll_x += (float)xoffset;
	scroll_y += (float)yoffset;
}

void processInput(GLFWwindow* window) {
	// Mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Convert from window space to native space
	float scale = (float)window_height / 536.0f; // more hard coded numbers oh yeah
	int offset_x = (window_width - 960 * scale) / 2;

	native_x = (xpos - offset_x) / scale;
	native_y = (ypos - 0)        / scale;

	// Convert to character space
	mouse_char_x = (int)(native_x / CHAR_WIDTH);
	mouse_char_y = (int)((native_y + 8) / CHAR_HEIGHT); // shader offset
}

void draw_imgui() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	
	ImGui::NewFrame();

	// Because imgui hijacks things thanks imgui
	ImGui::SetMouseCursor(ImGuiMouseCursor_None);
	
	// return here to not draw imgui
	return;

	//ImGui::ShowDemoWindow(); // Show demo window! :)

	ImGui::SliderFloat("dval1", &dval1, 0.0f, 1.0f);
	
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
	// No depth buffer only stencil if you crash you are probably on iphone 4
	glfwWindowHint(GLFW_STENCIL_BITS, 8);

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

	glfwSetScrollCallback(window, scroll_callback);

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

	// Name            || Samples from   || Writes to    || Purpose
	// 
	// raster_shader   || ---			 || nativeFBO    || Render text to native size
	// pass_shader     || nativeFBO      || fullFBO      || Upscale text to full screen size
	// scanline_shader || fullFBO        || compositeFBO || Draw raster glow to composite FBO 
	// stencil_shader  || ---            || compositeFBO || Draw stencil for lines
	// line_shader     || ---		     || compositeFBO || Draw vector lines

	// the scanline shader could be done in the pass shader, but both 

	// Compile shaders
	Shader raster_shader   = Shader("vertex.glsl", "fragment.glsl",				std::vector<std::string>(), 460);
	Shader pass_shader     = Shader("vertex.glsl", "fragment_pass.glsl",		std::vector<std::string>(), 460);
	Shader scanline_shader = Shader("vertex.glsl", "fragment_scanline.glsl",	std::vector<std::string>(), 460);
	Shader stencil_shader  = Shader("vertex.glsl", "fragment_stencil.glsl",		std::vector<std::string>(), 460);
	Shader line_shader     = Shader("vertex_lines.glsl", "fragment_lines.glsl",	std::vector<std::string>(), 460);
	//Shader blur_shader     = Shader("vertex.glsl", "fragment_blur.glsl",        std::vector<std::string>(), 460);
	//Shader crt_shader      = Shader("vertex.glsl", "fragment_crt.glsl",         std::vector<std::string>(), 460);

#pragma region General loading

	// Load font, this had to be done first and on the main thread
	vector<uint32_t> font_data = load_font("gamedata\\fonts\\font.txt"); // Enter your font path here

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

	// Internal render targets
	int NUM_CHARS = CHAR_COLS * CHAR_ROWS;
	uint32_t* char_grid = new uint32_t[NUM_CHARS];

	// How many lines do you think youll want to ever draw:
	int MAX_LINES = 10000;
	float* line_verts = new float[MAX_LINES * 4 * 2]; // 4 vertices per line, 2 floats per vertex
	uint32_t* line_colors = new uint32_t[MAX_LINES]; // 1 color per line

	systems_controller = make_unique<SystemsController>(
		RenderTargets{
			char_grid,
			line_verts,
			line_colors
		},
		"gamedata\\ui\\gameplay_ui.json"
	);

#pragma endregion
#pragma region Vertex Buffers

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

	// Color buffer — SSBO
	glGenBuffers(1, &line_color_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, line_color_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_LINES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, line_color_SSBO); // Binding = 1, match in GLSL
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1); // Bind to 1

#pragma endregion
#pragma region Framebuffers

	//  ==== Native framebuffer for raster text rendering 

	// Create framebuffer
	GLuint nativeFBO;
	glGenFramebuffers(1, &nativeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, nativeFBO);

	// Create texture for framebuffer attachment
	GLuint nativeTex;
	glGenTextures(1, &nativeTex);
	glBindTexture(GL_TEXTURE_2D, nativeTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Set texture border color to black
	glm::vec4 border_color(0.0f, 0.0f, 0.0f, 1.0f);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

	// Attach texture to the framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nativeTex, 0);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

	//// ==== Fullscreen framebuffer for upscaling text

	//GLuint fullFBO;
	//glGenFramebuffers(1, &fullFBO);
	//glBindFramebuffer(GL_FRAMEBUFFER, fullFBO);

	//glGenTextures(1, &fullTex);
	//glBindTexture(GL_TEXTURE_2D, fullTex);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fullTex, 0);

	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	//	std::cerr << "ERROR: Fullscreen framebuffer is not complete!" << std::endl;
	//}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

	// ==== Composite for vector lines and raster text

	GLuint compositeFBO;
	glGenFramebuffers(1, &compositeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);

	glGenTextures(1, &compositeTex);
	glBindTexture(GL_TEXTURE_2D, compositeTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTex, 0);

	// ----- Create Depth + Stencil Renderbuffer -----
	glGenRenderbuffers(1, &depthStencilRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height);

	// Attach renderbuffer to framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "ERROR: Composite framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//// ==== Blur framebuffer for intermediate step
	//GLuint blurFBO;
	//glGenFramebuffers(1, &blurFBO);
	//glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

	//glGenTextures(1, &blurTex);
	//glBindTexture(GL_TEXTURE_2D, blurTex);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex, 0);

	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	//	std::cerr << "ERROR: Blur framebuffer is not complete!" << std::endl;
	//}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

#pragma endregion
#pragma region SSBOs

	// Character grid buffer
	// Holds CHAR_COLS by CHAR_ROWS characters
	// Stores uints for character and color
	// First 8 bits are character, next 8 are color, next 8 are background color, and 8 unused

	unsigned int char_grid_buffer;

	glGenBuffers(1, &char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_CHARS * sizeof(uint32_t), char_grid, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, char_grid_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

#define MAX_STENCIL_REGIONS 16
	// Stencil regions buffer
	// Holds 4 * sizof(float) * MAX_STENCIL_REGIONS bytes
	// Each region is two vec2s [x1, y1], [x2, y2] in native space
	float* stencil_regions = new float[MAX_STENCIL_REGIONS * 4]; // 4 floats per region (2 vec2s)
	int region_count = 0;

	unsigned int stencil_regions_buffer;

	glGenBuffers(1, &stencil_regions_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, stencil_regions_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_STENCIL_REGIONS * 4 * sizeof(float), stencil_regions, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, stencil_regions_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


#pragma endregion

	// Set this every frame how many lines were drawing this time
	int num_lines = 0;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);


		GlobalUpdateData global_update_data;
		global_update_data.mouse_pos_native = vec2(native_x, native_y);
		global_update_data.mouse_pos_char = vec2(mouse_char_x, mouse_char_y);
		global_update_data.scroll_delta = vec2(scroll_x, scroll_y);

		systems_controller->update(window, global_update_data);
		RenderData render_data = systems_controller->render();
		num_lines = render_data.lines_counter;

		// vec2s are just float x, y; so you can cast them directly
		stencil_regions = reinterpret_cast<float*>(render_data.stencil_regions.data());
		region_count = render_data.stencil_regions.size() / 2; // each region is two vec2s

		// Rendering starts here
		draw_imgui();
		set_uniforms();

#pragma region raster_shader
		glBindFramebuffer(GL_FRAMEBUFFER, nativeFBO);
		glViewport(0, 0, SMALL_WINDOW_WIDTH, SMALL_WINDOW_HEIGHT); // Match the framebuffer size
		glClear(GL_COLOR_BUFFER_BIT); // Clear the framebuffer

		raster_shader.use();

		// The amount of data we send to the gpu is only 16.3 kb, if you want to optimize
		// this then go right ahead if you want another 1 fps over the 1000 you already get

		// Copy the screen to the gpu
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, char_grid_buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_CHARS * sizeof(uint32_t), char_grid);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct 		// Draw the quad

		// bind texture and generate mipmaps
		glBindTexture(GL_TEXTURE_2D, nativeTex);		// Bind texture to unit 0
		glGenerateMipmap(GL_TEXTURE_2D);				// Generate mipmaps for the texture
		glBindTexture(GL_TEXTURE_2D, 0);				// Unbind texture


#pragma endregion
#pragma region pass_shader
		// Finally unbind the small framebuffer
		// ---- ALL IN SOFTWARE RASTER ELEMENTS MUST BE DRAWN ABOVE THIS LINE ----
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, window_width, window_height);	// Fullscreen viewport

		pass_shader.use();							// Use shader first
		glActiveTexture(GL_TEXTURE0);					// Activate texture unit 0
		glBindTexture(GL_TEXTURE_2D, nativeTex);		// Bind texture to unit 0
		pass_shader.setInt("screenTexture", 0);		// Set uniform to use unit 0

		// Set uniforms
		pass_shader.setFloat("aspectRatio", aspect_ratio);
		pass_shader.setFloat("aspectRatioSmall", aspect_ratio_small);

		glBindVertexArray(VAO);							// Fullscreen quad VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // Correct              // Draw the quad

#pragma endregion
#pragma region stencil_shader
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
		glEnable(GL_STENCIL_TEST); // Enable stencil test

		// Draw to stencil buffer for lines
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Don't draw colors
		glStencilFunc(GL_ALWAYS, 1, 0xFF);                  // Always pass stencil test
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);          // Replace stencil with ref=1 on pass
		glStencilMask(0xFF);

		stencil_shader.use();
		stencil_shader.setFloat("aspectRatio", aspect_ratio);
		stencil_shader.setFloat("aspectRatioSmall", aspect_ratio_small);
		stencil_shader.setInt("zone_count", region_count); // whatever

		// Upload stencil regions to the shader
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, stencil_regions_buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, MAX_STENCIL_REGIONS * 4 * sizeof(float), stencil_regions);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// Draw fullscreen quad only the fragement is different so we can use the same VAO
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Step 2: Only allow drawing where stencil == 1
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes
		glStencilMask(0x00);                             // Disable writing to stencil

#pragma endregion
#pragma region line_shader
		// Draw electron beam lines, these are not rasterized so draw them at screen resolution

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_BLEND);
		glLineWidth(2.0f); 
		glEnable(GL_LINE_SMOOTH);

		line_shader.use();

		// Set uniforms
		line_shader.setFloat("aspectRatio", aspect_ratio);
		line_shader.setFloat("aspectRatioSmall", aspect_ratio_small);
		

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

		// Draw the first 25 lines that are reserved for the cursor without stencil
		line_shader.setInt("prim_offset", 0);
		glDrawArrays(GL_LINES, 0, 50);

		// Turn on the stencil finally
		glStencilFunc(GL_EQUAL, render_data.stencil_state, 0xFF);

		// Only bother drawing any other lines if we have more than 25 lines
		// so we dont waste time dispatching a shader and random bugginess
		if (num_lines > 25) {
			line_shader.setInt("prim_offset", 25);
			glDrawArrays(GL_LINES, 50, (num_lines * 2) - 50);
		}

		// Clear the stencil right away (why doesnt it clear when you try to
		// clear it at the same time as the color buffer?)
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);

		// Unbind for cleanliness
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDisable(GL_BLEND); // Disable blending for next draw calls
		glDisable(GL_STENCIL_TEST);

#pragma endregion
#pragma region scanline_shader

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		scanline_shader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, compositeTex); // Bind the composite texture
		scanline_shader.setInt("buffer_texture", 0);

		// bind native texture to unit 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, nativeTex);

		// linear filtering for "blur"
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		scanline_shader.setInt("native_texture", 1);

		scanline_shader.setFloat("resy", window_height);
		scanline_shader.setFloat("resx", window_width);

		//scanline_shader.setFloat("dval1", dval1);

		pass_shader.setFloat("aspectRatio", aspect_ratio);
		pass_shader.setFloat("aspectRatioSmall", aspect_ratio_small);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// reset filtering mode
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#pragma endregion

		// Draw Dear ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	systems_controller->clean_up_threads();
	glfwTerminate();
	return 0;

}
