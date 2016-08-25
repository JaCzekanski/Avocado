#pragma once
#include <string>
#include <vector>
#include "Shader.h"
#include <glad/glad.h>

class Program
{
	std::string name;
	std::string error = "";
	std::vector<Shader> shaders;
	GLuint programId = 0;

	void destroy();
	GLuint link(std::vector<Shader> &shaders);
	bool initialized;
public:
	Program(std::string name);
	~Program();

	std::string getName() { return name; }

	bool Program::load();
	std::string getError();

	GLuint get();
	bool use();
	GLint getAttrib(const GLchar *name);
	GLint getUniform(const GLchar *name);
};

