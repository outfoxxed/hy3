attribute highp vec2 pos;

uniform mat3 proj;
uniform highp vec2 pixelOffset;
uniform highp vec2 pixelSize;
uniform highp vec2 monitorSize;

varying highp vec2 pixCoord;
varying highp vec2 monitorTexCoord;

void main() {
	pixCoord = pos * pixelSize;
	monitorTexCoord = (pixelOffset + pixCoord) / monitorSize;
	gl_Position = vec4(proj * vec3(monitorTexCoord, 1.0), 1.0);
}
