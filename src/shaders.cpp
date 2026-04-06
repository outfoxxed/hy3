#include "shaders.hpp"
#include <string>
#include <stdexcept>

#include <GLES2/gl2.h>
#include <hyprland/src/render/OpenGL.hpp>

#include "shader_content.hpp"

Hy3Shaders::Hy3Shaders() {
	{
		auto& s = this->tab;
		s.program = makeShared<CShader>();
		if (!s.program->createProgram(std::string(SHADER_TAB_VERT), std::string(SHADER_TAB_FRAG))) {
			throw std::runtime_error("hy3 tab shader compilation fails");
		}
		auto program = s.program->program();
		s.proj = glGetUniformLocation(program, "proj");
		s.monitorSize = glGetUniformLocation(program, "monitorSize");
		s.pixelOffset = glGetUniformLocation(program, "pixelOffset");
		s.pixelSize = glGetUniformLocation(program, "pixelSize");
		s.applyBlur = glGetUniformLocation(program, "applyBlur");
		s.blurTex = glGetUniformLocation(program, "blurTex");
		s.opacity = glGetUniformLocation(program, "opacity");
		s.fillColor = glGetUniformLocation(program, "fillColor");
		s.borderColor = glGetUniformLocation(program, "borderColor");
		s.borderWidth = glGetUniformLocation(program, "borderWidth");
		s.outerRadius = glGetUniformLocation(program, "outerRadius");
	}
}

Hy3Shaders* Hy3Shaders::instance() {
	static auto* INSTANCE = new Hy3Shaders();
	return INSTANCE;
}
