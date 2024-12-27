attribute highp vec2 pos;
attribute highp vec2 texcoord;

uniform mat3 proj;

varying highp vec2 v_texcoord;

void main() {
	gl_Position = vec4(proj * vec3(pos, 1.0), 1.0);
	v_texcoord = pos;
}
