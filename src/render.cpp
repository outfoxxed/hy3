#include "render.hpp"

#include <GLES2/gl2.h>
#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>
#include <hyprland/src/render/Shader.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprutils/math/Misc.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include "render/Renderer.hpp"
#include "render/Texture.hpp"
#include "shaders.hpp"

using Render::GL::g_pHyprOpenGL;
using Hyprutils::Math::CBox;

void Hy3Render::renderTab(
    const CBox& box,
    float opacity,
    bool blur,
    const CHyprColor& fillColor,
    const CHyprColor& borderColor,
    int borderWidth,
    int radius
) {
	static auto& shader = Hy3Shaders::instance()->tab;
	auto& rdata = g_pHyprRenderer->m_renderData;

	auto rbox = box;
	rdata.renderModif.applyToBox(rbox);

	const auto& monitorSize = rdata.pMonitor->m_transformedSize;
	auto monitorBox = CBox {Vector2D(), monitorSize};

	const auto monitor_inverted = Math::wlTransformToHyprutils(
			Math::invertTransform(g_pHyprRenderer->m_renderData.pMonitor->m_transform)
	);

	Hyprutils::Math::eTransform transform = monitor_inverted;

	auto glMatrix = g_pHyprRenderer->projectBoxToTarget(monitorBox, transform);

	g_pHyprOpenGL->useShader(shader.program);

#ifndef GLES2
	glUniformMatrix3fv(shader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());
#else
	glMatrix.transpose();
	glUniformMatrix3fv(shader.proj, 1, GL_FALSE, glMatrix.getMatrix().data());
#endif

	WP<Render::ITexture> blurTex;

	if (blur) {
		blurTex = rdata.pMonitor->resources()->m_blurFB->getTexture();
		if (!blurTex) blur = false;
	}

	if (blur) {
		GLCALL(glActiveTexture(GL_TEXTURE0));
		//GLCALL(glBindTexture(blurTex->m_target, blurTex->m_texID));
		GLCALL(glBindTexture(GL_TEXTURE_2D, blurTex->m_texID));
		GLCALL(glUniform1i(shader.blurTex, 0));
	}

	GLCALL(glUniform1i(shader.applyBlur, blur));

	// premultiplied
	GLCALL(glUniform4f(
	    shader.fillColor,
	    fillColor.r * fillColor.a,
	    fillColor.g * fillColor.a,
	    fillColor.b * fillColor.a,
	    fillColor.a
	));

	if (borderWidth != 0) {
		GLCALL(glUniform4f(
		    shader.borderColor,
		    borderColor.r * borderColor.a,
		    borderColor.g * borderColor.a,
		    borderColor.b * borderColor.a,
		    borderColor.a
		));
	}

	GLCALL(glUniform2f(shader.monitorSize, monitorSize.x, monitorSize.y));
	GLCALL(glUniform2f(shader.pixelOffset, rbox.x, rbox.y));
	GLCALL(glUniform2f(shader.pixelSize, rbox.w, rbox.h));
	GLCALL(glUniform1f(shader.opacity, opacity));
	GLCALL(glUniform1f(shader.outerRadius, radius));
	GLCALL(glUniform1f(shader.borderWidth, borderWidth));

	GLCALL(glBindVertexArray(shader.program->getUniformLocation(SHADER_SHADER_VAO)));
	GLCALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	GLCALL(glBindVertexArray(0));

	if (blur) {
		// GLCALL(glBindTexture(blurTex->m_target, 0));
		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
	}
}
