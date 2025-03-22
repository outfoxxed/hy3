#include "render.hpp"

#include <GLES2/gl2.h>
#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprutils/math/Misc.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include "shaders.hpp"

using Hyprutils::Math::CBox;
using Hyprutils::Math::HYPRUTILS_TRANSFORM_NORMAL;

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
	auto& rdata = g_pHyprOpenGL->m_RenderData;

	auto rbox = box;
	rdata.renderModif.applyToBox(rbox);

	const auto& monitorSize = rdata.pMonitor->vecTransformedSize;
	auto monitorBox = CBox {Vector2D(), monitorSize};

	auto matrix =
	    rdata.monitorProjection.projectBox(monitorBox, HYPRUTILS_TRANSFORM_NORMAL, monitorBox.rot);

	auto glMatrix = rdata.projection.copy().multiply(matrix);

	glUseProgram(shader.program);

#ifndef GLES2
	glUniformMatrix3fv(shader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());
#else
	glMatrix.transpose();
	glUniformMatrix3fv(shader.proj, 1, GL_FALSE, glMatrix.getMatrix().data());
#endif

	WP<CTexture> blurTex;

	if (blur) {
		blurTex = rdata.pCurrentMonData->blurFB.getTexture();
		if (!blurTex) blur = false;
	}

	if (blur) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(blurTex->m_iTarget, blurTex->m_iTexID);
		glUniform1i(shader.blurTex, 0);
	}

	glUniform1i(shader.applyBlur, blur);

	// premultiplied
	glUniform4f(
	    shader.fillColor,
	    fillColor.r * fillColor.a,
	    fillColor.g * fillColor.a,
	    fillColor.b * fillColor.a,
	    fillColor.a
	);

	if (borderWidth != 0) {
		glUniform4f(
		    shader.borderColor,
		    borderColor.r * borderColor.a,
		    borderColor.g * borderColor.a,
		    borderColor.b * borderColor.a,
		    borderColor.a
		);
	}

	glUniform2f(shader.monitorSize, monitorSize.x, monitorSize.y);
	glUniform2f(shader.pixelOffset, rbox.x, rbox.y);
	glUniform2f(shader.pixelSize, rbox.w, rbox.h);
	glUniform1f(shader.opacity, opacity);
	glUniform1f(shader.outerRadius, radius);
	glUniform1f(shader.borderWidth, borderWidth);

	glVertexAttribPointer(shader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
	glEnableVertexAttribArray(shader.posAttrib);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(shader.posAttrib);

	if (blur) {
		glBindTexture(blurTex->m_iTarget, 0);
	}
}
