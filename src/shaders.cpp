#include "shaders.hpp"
#include <csignal>
#include <string>

#include <GLES2/gl2.h>
#include <hyprland/src/render/OpenGL.hpp>

#include "shader_content.hpp"

Hy3Shaders::Hy3Shaders() {
	{
		auto& s = this->tab;
		s.program = g_pHyprOpenGL->createProgram(std::string(SHADER_TAB_VERT), std::string(SHADER_TAB_FRAG));
		s.posAttrib = glGetAttribLocation(s.program, "pos");
		s.proj = glGetUniformLocation(s.program, "proj");
		s.monitorSize = glGetUniformLocation(s.program, "monitorSize");
		s.pixelOffset = glGetUniformLocation(s.program, "pixelOffset");
		s.pixelSize = glGetUniformLocation(s.program, "pixelSize");
		s.applyBlur = glGetUniformLocation(s.program, "applyBlur");
		s.blurTex = glGetUniformLocation(s.program, "blurTex");
		s.opacity = glGetUniformLocation(s.program, "opacity");
		s.fillColor = glGetUniformLocation(s.program, "fillColor");
		s.borderColor = glGetUniformLocation(s.program, "borderColor");
		s.borderWidth = glGetUniformLocation(s.program, "borderWidth");
		s.outerRadius = glGetUniformLocation(s.program, "outerRadius");
	}
}

Hy3Shaders::~Hy3Shaders() { glDeleteProgram(this->tab.program); }

Hy3Shaders* Hy3Shaders::instance() {
	static auto* INSTANCE = new Hy3Shaders();
	return INSTANCE;
}
