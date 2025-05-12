#pragma once

#include <GLES2/gl2.h>
#include <hyprland/src/render/OpenGL.hpp>

class Hy3Shaders {
public:
	struct {
		GLuint program;
		GLuint posAttrib;
		GLint proj;
		GLint monitorSize;
		GLint pixelOffset;
		GLint pixelSize;
		GLint applyBlur;
		GLint blurTex;
		GLint opacity;
		GLint fillColor;
		GLint borderColor;
		GLint borderWidth;
		GLint outerRadius;
	} tab;

	static Hy3Shaders* instance();

private:
	Hy3Shaders();
	~Hy3Shaders();
};
