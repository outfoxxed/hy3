#pragma once

#include <string_view>

#include <GLES2/gl2.h>
#include <hyprland/src/render/OpenGL.hpp>

class Hy3Shaders {
public:
	struct {
		GLuint program;
		GLuint posAttrib;
		GLint proj;
		GLint fillColor;
		GLint borderColor;
		GLint pixelSize;
		GLint outerRadius;
		GLint borderWidth;
	} border_rect;

	static Hy3Shaders* instance();

private:
	Hy3Shaders();
	~Hy3Shaders();

	static GLuint createProgram(const std::string_view vert, const std::string_view frag);
	static GLuint compileShader(GLuint type, const std::string_view src);
	static void logShaderError(GLuint shader, bool program);
};
