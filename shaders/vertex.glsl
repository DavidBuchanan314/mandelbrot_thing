#version 130

uniform vec2 bounds;

in vec2 position;

void main() {
	gl_Position = vec4(position.x, (position.y*bounds.y-bounds.x)*2.0+1.0, 0.0, 1.0);
}
