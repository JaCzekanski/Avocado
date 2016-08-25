#include "opengl.h"
#include <SDL.h>
#include <glad/glad.h>
#include <memory>
#include "shader/Program.h"
#include "device/gpu.h"


namespace opengl
{
	std::unique_ptr<Program> renderShader;
	std::unique_ptr<Program> blitShader;

	// emulated console GPU calls
	GLuint renderVao; // attributes
	GLuint renderVbo; // buffer

	// VRAM to screen blit
	GLuint blitVao;
	GLuint blitVbo;

	GLuint vramTex;
	GLuint framebuffer;

	void init()
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}

	bool loadExtensions()
	{
		return gladLoadGLLoader(SDL_GL_GetProcAddress);
	}

	bool setup()
	{
		renderShader = std::make_unique<Program>("data/shader/render");
		if (!renderShader->load())
		{
			printf("Cannot load render shader: %s\n", renderShader->getError().c_str());
			return false;
		}

		blitShader = std::make_unique<Program>("data/shader/blit");
		if (!blitShader->load())
		{
			printf("Cannot load blit shader: %s\n", blitShader->getError().c_str());
			return false;
		}


		// Render buffer
		{
			glGenVertexArrays(1, &renderVao);
			glBindVertexArray(renderVao);

			glGenBuffers(1, &renderVbo);
			glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
			glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			glVertexAttribIPointer(renderShader->getAttrib("position"), 2, GL_UNSIGNED_INT, sizeof(Vertex), 0);
			glVertexAttribIPointer(renderShader->getAttrib("color"), 3, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(2 * sizeof(int)));
			glVertexAttribPointer(renderShader->getAttrib("texcoord"), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(int) + 3 * sizeof(int)));

			glEnableVertexAttribArray(renderShader->getAttrib("position"));
			glEnableVertexAttribArray(renderShader->getAttrib("color"));
			glEnableVertexAttribArray(renderShader->getAttrib("texcoord"));
		}

		// Blit buffer
		{
			struct BlitStruct
			{
				float pos[2];
				float tex[2];
			};

			BlitStruct blitVertex[] = {
				{0.f, 0.f, 0.f, 0.f},
				{1.f, 0.f, 1.f, 0.f},
				{1.f, 1.f, 1.f, 1.f},

				{ 0.f, 0.f, 0.f, 0.f },
				{ 1.f, 1.f, 1.f, 1.f },
				{ 0.f, 1.f, 0.f, 1.f },
			};

			glGenVertexArrays(1, &blitVao);
			glBindVertexArray(blitVao);

			glGenBuffers(1, &blitVbo);
			glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(blitVertex), blitVertex, GL_STATIC_DRAW);

			glVertexAttribPointer(blitShader->getAttrib("position"), 2, GL_FLOAT, GL_FALSE, sizeof(BlitStruct), 0);
			glVertexAttribPointer(blitShader->getAttrib("texcoord"), 2, GL_FLOAT, GL_FALSE, sizeof(BlitStruct), (void*)(2 * sizeof(float)));

			glEnableVertexAttribArray(blitShader->getAttrib("position"));
			glEnableVertexAttribArray(blitShader->getAttrib("texcoord"));
		}
		
		{
			glGenTextures(1, &vramTex);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, vramTex);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}


		renderShader->use();
		glUniform1i(renderShader->getUniform("vram"), 0);

		blitShader->use();
		glUniform1i(blitShader->getUniform("vram"), 0);

		return true;
	}

	void render(device::gpu::GPU* gpu)
	{
		glViewport(0, 0, resWidth, resHeight);
//		glClearColor(0.f, 0.f, 0.f, 1.0f);
//		glClearDepth(0.0f);
//		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// Update VRAM texture
		glBindTexture(GL_TEXTURE_2D, vramTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->VRAM);

		auto &renderList = gpu->render();

		// Update renderlist
		if (!renderList.empty()) {
			glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, renderList.size() * sizeof(Vertex), renderList.data());
		}


		// First stage - render calls to VRAM
		if (!renderList.empty()) {
			renderShader->use();
			glBindVertexArray(renderVao);
			glBindBuffer(GL_ARRAY_BUFFER, renderVbo);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, vramTex);

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			//glDrawArrays(GL_TRIANGLES, 0, renderList.size());
			for (int i = 0; i < renderList.size(); i += 3) {
				glDrawArrays(GL_TRIANGLES, i, 3);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}


		// Second stage - blit VRAM to screen
		blitShader->use();
		glBindVertexArray(blitVao);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, vramTex);

		glDrawArrays(GL_TRIANGLES, 0, 6);


		renderList.clear();
	}
}
