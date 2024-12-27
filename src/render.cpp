#include "render.hpp"

#include <GLES2/gl2.h>
#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/render/OpenGL.hpp>

#include "shaders.hpp"

void Hy3Render::renderBorderRect(
    const CBox& box,
    const CHyprColor& fillColor,
    const CHyprColor& borderColor,
    int borderWidth,
    int radius
) {
	static auto& shader = Hy3Shaders::instance()->border_rect;
	auto& rdata = g_pHyprOpenGL->m_RenderData;

	auto rbox = box;
	rdata.renderModif.applyToBox(rbox);

	auto matrix = rdata.monitorProjection.projectBox(
	    rbox,
	    wlTransformToHyprutils(invertTransform(rdata.pMonitor->transform)),
	    rbox.rot
	);

	auto glMatrix = rdata.projection.copy().multiply(matrix);

	glUseProgram(shader.program);

#ifndef GLES2
	glUniformMatrix3fv(shader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());
#else
	glMatrix.transpose();
	glUniformMatrix3fv(shader.proj, 1, GL_FALSE, glMatrix.getMatrix().data());
#endif

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

	glUniform2f(shader.pixelSize, rbox.w, rbox.h);
	glUniform1f(shader.outerRadius, radius);
	glUniform1f(shader.borderWidth, borderWidth);

	glVertexAttribPointer(shader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
	glEnableVertexAttribArray(shader.posAttrib);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(shader.posAttrib);
}
