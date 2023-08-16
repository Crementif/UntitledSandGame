#version 450

layout(location = 0) out vec2 param_uv;

layout(binding = 0) uniform uf_data
{
	vec4 coords[4];
};

void main()
{
	int vId = gl_VertexID;
	vec4 ufSample = coords[vId];
	param_uv = ufSample.zw;
	gl_Position = vec4(ufSample.xy, 0.0, 1.0);
}
