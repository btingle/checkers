#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec2 texc;
out vec3 norml;
out vec3 frag_pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	norml = norm;
	texc = tex;
	frag_pos = vec3(model * vec4(pos, 1.0));
	gl_Position = projection * view * model * vec4(pos, 1.0);
}