#pragma once

#include "Shader.h"
#include "Camera.h"
#include "ImGuiRenderer.h"

class Renderer
{
	unsigned int quadVAO;

	// render shaders
	Shader* albedo_shader;
	Shader* normal_shader;
	Shader* bvh_shader;
	Shader* path_shader;

	Shader* curr_shader;

	// rendering UBO
	unsigned int render_data;

	// accumulate buffers
	unsigned int sample_buffer;
	unsigned int accumulate_buffer;
	unsigned int result_buffer;

	unsigned int sample_texture;
	unsigned int accumulate_texture;
	unsigned int result_texture;

	// image processing shaders
	Shader* accumulate_shader; // combines sample and accumulate textures
	Shader* post_process_shader; // renders accumulate texture to default buffer with post processing

	int current_frame;

	Camera* camera;

	ImGuiRenderer imgui_renderer;

	void sampleRender()
	{
		// render scene
		/*glBindFramebuffer(GL_FRAMEBUFFER, sample_buffer);
		glClear(GL_COLOR_BUFFER_BIT);*/

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void accumulateRender()
	{
		// combine accumulate and sample buffer into result buffer
	}

	void copyResultIntoAccumulate()
	{
		// update accumulate buffer
	}

public:
	Renderer(Camera* camera) : current_frame(1), camera(camera)
	{
		albedo_shader = new Shader("Shaders/Vertex.shader", "Shaders/AlbedoFragment.shader");
		normal_shader = new Shader("Shaders/Vertex.shader", "Shaders/NormalFragment.shader");
		bvh_shader = new Shader("Shaders/Vertex.shader", "Shaders/BVHFragment.shader");
		path_shader = new Shader("Shaders/Vertex.shader", "Shaders/PathTraceFragment.shader");

		curr_shader = albedo_shader;

		// quad
		float quad_vertices[] = {
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		glGenVertexArrays(1, &quadVAO);
		glBindVertexArray(quadVAO);

		unsigned int quadVBO;
		glGenBuffers(1, &quadVBO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// ubo
		glGenBuffers(1, &render_data);
		glBindBuffer(GL_UNIFORM_BUFFER, render_data);
		glBufferData(GL_UNIFORM_BUFFER, 64 + 16 + 4 + 4, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 4, render_data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	~Renderer()
	{
		delete(albedo_shader);
		delete(normal_shader);
		delete(bvh_shader);
		delete(path_shader);

		delete(accumulate_shader);
		delete(post_process_shader);
	}

	void createBuffers(int screen_width, int screen_height)
	{
		glActiveTexture(GL_TEXTURE0);

		// creating sample buffer
		glGenFramebuffers(1, &sample_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, sample_buffer);

		glGenTextures(1, &sample_texture);
		glBindTexture(GL_TEXTURE_2D, sample_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sample_texture, 0);

		// creating accumulate buffer
		glGenFramebuffers(1, &accumulate_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, accumulate_buffer);

		glGenTextures(1, &accumulate_texture);
		glBindTexture(GL_TEXTURE_2D, accumulate_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulate_texture, 0);

		// creating result buffer
		glGenFramebuffers(1, &result_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, result_buffer);

		glGenTextures(1, &result_texture);
		glBindTexture(GL_TEXTURE_2D, result_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result_texture, 0);
	}

	void initImGui(GLFWwindow* window)
	{
		imgui_renderer.init(window);
	}

	void render(Scene* scene)
	{
		curr_shader->use();

		glBindTexture(GL_TEXTURE_2D, scene->environment_map);
		curr_shader->setInt("skybox_texture", 1);

		// update camera matrices
		camera->updateProjection(9.0f / 6.0f);
		camera->updateView();

		glm::mat4 inverse = glm::inverse(camera->view);

		// update ubo
		glBindBuffer(GL_UNIFORM_BUFFER, render_data);
		// camera transform
		glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, glm::value_ptr(inverse));
		// camera position
		glBufferSubData(GL_UNIFORM_BUFFER, 64, 12, &camera->m_pos);
		// scene->num_nodes
		glBufferSubData(GL_UNIFORM_BUFFER, 76, 4, &scene->num_nodes);
		// current frame
		glBufferSubData(GL_UNIFORM_BUFFER, 80, 4, &current_frame);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// clear window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		sampleRender();

		accumulateRender();

		copyResultIntoAccumulate();

		current_frame++;
	}

	void updateImGui()
	{
		imgui_renderer.update();
	}

	void renderImGui()
	{
		imgui_renderer.render();
	}

	void ImGuiDisplayDebugViewRadio()
	{
		ImGui::Text("Debug Views");
		static int e = 0;
		ImGui::RadioButton("Albedo", &e, 0);
		ImGui::RadioButton("Normals", &e, 1);
		ImGui::RadioButton("BVH", &e, 2);
		ImGui::RadioButton("Path Trace", &e, 3);

		if (e == 0)
			curr_shader = albedo_shader;
		else if (e == 1)
			curr_shader = normal_shader;
		else if (e == 2)
			curr_shader = bvh_shader;
		else if (e == 3)
			curr_shader = path_shader;
	}

	void resetAccumulate()
	{
		current_frame = 1;
	}
};