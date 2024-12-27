precision highp float;

uniform highp vec2 pixelSize;
uniform float opacity;
uniform highp vec4 fillColor;
uniform highp vec4 borderColor;
uniform float outerRadius;
uniform float borderWidth;

uniform bool applyBlur;
uniform sampler2D blurTex;

varying highp vec2 pixCoord;
varying highp vec2 monitorTexCoord;

void main() {
	float opacityMul = opacity;
	highp vec2 cornerDist = min(pixCoord, pixelSize - pixCoord);

	gl_FragColor = fillColor;

	if (cornerDist.x <= outerRadius && cornerDist.y <= outerRadius) {
		highp vec2 vcornerDist = vec2(outerRadius) - cornerDist;
		float distSq = vcornerDist.x * vcornerDist.x + vcornerDist.y * vcornerDist.y;

		// See https://github.com/hyprwm/Hyprland/blob/e75e2cdac79417ffdbbbe903f72668953483a4e7/src/render/shaders/SharedValues.hpp#L3
		const float SMOOTHING_CONSTANT = 0.58758063398831095317;
		const float SMOOTHING_CONSTANT_2X = SMOOTHING_CONSTANT * 2.0;

		float innerRadius = outerRadius - borderWidth;
		float outerTest1 = outerRadius + SMOOTHING_CONSTANT_2X;
		float outerTest2 = outerRadius - SMOOTHING_CONSTANT_2X;
		float innerTest1 = innerRadius + SMOOTHING_CONSTANT_2X;
		float innerTest2 = innerRadius - SMOOTHING_CONSTANT_2X;

		float dist;
		bool calculatedDist = false;

		if (distSq > outerTest1 * outerTest1) {
			discard;
		} else if (distSq > outerTest2 * outerTest2) {
			dist = sqrt(distSq);
			calculatedDist = true;
			float normalized = 1.0 - smoothstep(0.0, 1.0, (dist - outerRadius + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT_2X));
			gl_FragColor = borderColor;
			opacityMul *= normalized;
		} else if (distSq > innerTest2 * innerTest2) {
			gl_FragColor = borderColor;
		}

		if (distSq > innerTest2 * innerTest2 && distSq <= innerTest1 * innerTest1) {
			if (!calculatedDist) dist = sqrt(distSq);
			float normalized = 1.0 - smoothstep(0.0, 1.0, (dist - innerRadius + SMOOTHING_CONSTANT) / (SMOOTHING_CONSTANT_2X));
			gl_FragColor = gl_FragColor * (1.0 - normalized) + fillColor * normalized;
		}
	} else if (cornerDist.x <= borderWidth || cornerDist.y <= borderWidth) {
		gl_FragColor = borderColor;
	}

	if (applyBlur && gl_FragColor.a != 1.0) {
		gl_FragColor = gl_FragColor + texture2D(blurTex, monitorTexCoord) * (1.0 - gl_FragColor.a);
	}

	gl_FragColor *= opacityMul;
}
