#pragma once

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

GLuint makeShader(const char* vertexPath, const char* fragmentPath);