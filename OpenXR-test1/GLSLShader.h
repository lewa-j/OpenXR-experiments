#pragma once

#include <GL/glew.h>


class GLSLShader
{
public:
	GLSLShader();
	virtual ~GLSLShader();
	virtual int Load(const char* vert, const char* frag);
	virtual int Create(const char* vertSrc, const char* fragSrc);
	virtual void Use();
	virtual void UniformMat4(int loc, const float* val);

	int id;
	int u_mvpMtx;
};
