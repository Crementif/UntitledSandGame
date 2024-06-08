#version 450

// inputs
layout(location = 0) in vec2 attr_pos;
layout(location = 1) in vec2 attr_uv;

// outputs
layout(location = 0) out vec2 param_uv;

void main()
{
	param_uv = attr_uv;
	gl_Position = vec4(attr_pos, 0.0, 1.0);
}