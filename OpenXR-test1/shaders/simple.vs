#version 100
precision highp float;

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;

uniform mat4 u_mvpMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	gl_PointSize = 4.0;
}
