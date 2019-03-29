#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;
layout(location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 MVP;

out vec3 WorldPos;
out vec2 Tex;
out vec3 Normal;

void main()
{
	Tex = aTex;
	WorldPos = vec3(model * vec4(aPos, 1.0));
	Normal = mat3(model) * aNormal;

	gl_Position = MVP * vec4(aPos, 1.0);
}