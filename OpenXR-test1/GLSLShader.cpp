
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "GLSLShader.h"
#include <vector>
#include <string>

#define Log printf

std::string FS_ReadAllText(const char* filename)
{
	FILE* input = fopen(filename, "rb");
	if (input == NULL)
	{
		printf("File: %s not found\n", filename );
		return "";
	}

	if (fseek(input, 0, SEEK_END) != 0)
		return "";
	long size = ftell(input);
	if (size == -1)
		return "";
	if (fseek(input, 0, SEEK_SET) != 0)
		return "";

	std::string content;
	content.resize(size);

	fread((char*)content.c_str(), 1, (size_t)size, input);
	if (ferror(input))
	{
		return "";
	}

	fclose(input);
	return content;
}

int CreateShader(int type, const char* src)
{
	int s = glCreateShader(type);
	glShaderSource(s, 1, &src, 0);
	glCompileShader(s);
	int status = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &status);
	if (!status) {
		Log("shader compile status %d\n", status);

		int size = 0;
		glGetShaderiv(s, GL_INFO_LOG_LENGTH, &size);
		std::vector<char> data(size);
		glGetShaderInfoLog(s, size, &size, data.data());

		Log("%s\n", data.data());
	}
	return s;
}

GLSLShader::GLSLShader() : id(0)
{
	u_mvpMtx = -1;
}

GLSLShader::~GLSLShader()
{
	glDeleteProgram(id);
}

int GLSLShader::Create(const char* vt, const char* ft)
{
	int vs = CreateShader(GL_VERTEX_SHADER, vt);
	int fs = CreateShader(GL_FRAGMENT_SHADER, ft);

	id = glCreateProgram();

	glBindAttribLocation(id, 0, "a_position");
	glBindAttribLocation(id, 1, "a_normal");
	glBindAttribLocation(id, 2, "a_uv");

	glAttachShader(id, vs);
	glAttachShader(id, fs);
	glLinkProgram(id);
	Log("GLSL Created id %d\n", id);

	glDeleteShader(vs);
	glDeleteShader(fs);

	int status = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (!status)
		Log("link status %d\n", status);

	u_mvpMtx = glGetUniformLocation(id, "u_mvpMtx");
	//u_color = glGetUniformLocation(id, "u_color");

	return 0;
}

int GLSLShader::Load(const char* vert, const char* frag)
{
	std::string vt = FS_ReadAllText((std::string("shaders/") + vert + ".vs").c_str());
	if (vt.size() == 0) {
		Log("shader: %s.vs not found\n", vert);
		return -1;
	}
	std::string ft = FS_ReadAllText((std::string("shaders/") + frag + ".fs").c_str());
	if (ft.size() == 0) {
		Log("shader: %s.fs not found\n", frag);
		return -1;
	}

	return Create(vt.c_str(), ft.c_str());
}

void GLSLShader::Use()
{
	if (id)
		glUseProgram(id);
}

void GLSLShader::UniformMat4(int loc, const float* val)
{
	glUniformMatrix4fv(loc, 1, false, val);
}
