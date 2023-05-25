#version 450

layout(location = 0) out vec2 param_uv;

layout(binding = 0) uniform uf_data
{
	vec4 coords[4];
};

void main()
{
	int vId = gl_VertexID;
	int ix = vId&1;
	int iy = vId>>1;

	//vec2 pos = vec2(ix, iy);

	//param_uv = vec2(ix, iy);
	//gl_Position = vec4(pos, 0.0, 1.0);
	
	vec4 ufSample = coords[vId];
	param_uv = ufSample.zw;
	gl_Position = vec4(ufSample.xy, 0.0, 1.0);
}
