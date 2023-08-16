#version 450
layout(location = 0) out vec2 param_uv;
void main()
{
	int vId = gl_VertexID;
	int vX = vId&1;
	int vY = (vId>>1)&1;

	vec2 pos = vec2(float(vX)*2.0-1.0, float(vY)*2.0-1.0);
	param_uv = vec2(vX, 1.0 - float(vY));
	gl_Position = vec4(pos, 0.0, 1.0);
}
