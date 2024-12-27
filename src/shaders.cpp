#include "shaders.hpp"
#include <csignal>

#include <GLES2/gl2.h>
#include <hyprland/src/render/OpenGL.hpp>

#include "shader_content.hpp"

Hy3Shaders::Hy3Shaders() {
	{
		auto& s = this->tab;
		s.program = Hy3Shaders::createProgram(SHADER_TAB_VERT, SHADER_TAB_FRAG);
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

GLuint Hy3Shaders::createProgram(const std::string_view vert, const std::string_view frag) {
	auto vshader = compileShader(GL_VERTEX_SHADER, vert);
	auto fshader = compileShader(GL_FRAGMENT_SHADER, frag);

	auto program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);

	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	GLint ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);

	if (ok != GL_TRUE) {
		Hy3Shaders::logShaderError(program, true);
	}

	return program;
}

GLuint Hy3Shaders::compileShader(GLuint type, const std::string_view src) {
	auto srcdata = src.data();
	GLint srclen = src.length();

	auto shader = glCreateShader(type);
	glShaderSource(shader, 1, &srcdata, &srclen);
	glCompileShader(shader);

	GLint ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

	if (ok != GL_TRUE) {
		Hy3Shaders::logShaderError(shader, false);
	}

	return shader;
}

void Hy3Shaders::logShaderError(GLuint shader, bool program) {
	GLint logBufferLength = 0;

	if (program) glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logBufferLength);
	else glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logBufferLength);

	auto* logBuffer = new char[logBufferLength];

	if (program) glGetProgramInfoLog(shader, logBufferLength, &logBufferLength, logBuffer);
	else glGetShaderInfoLog(shader, logBufferLength, &logBufferLength, logBuffer);

	if (program) {
		Debug::log(ERR, "Failed to link shader program: {}", logBuffer);
	} else {
		Debug::log(ERR, "Failed to compile shader: {}", logBuffer);
	}

	raise(SIGABRT);
}
