#version 450

layout(location = 0) out vec2 ss_uv;
layout(location = 1) out vec2 ws_uv;
layout(binding = 0) uniform uf_data
{
	vec4 coords[4];
	vec2 renderPos; // top-left of render area
	vec2 renderSize; // size of render area
};

void main()
{
	int vId = gl_VertexID;
	vec4 vertexData = coords[vId];
	vec2 screenOffset = vertexData.xy;
	vec2 vertexUv = vertexData.zw;

	ss_uv = vertexUv;

	// calculate uv for world space (0-1 is left-right on map) using pre-normalized (divided by map size) vertex uv
	ws_uv = renderPos + (vertexUv * renderSize);

	gl_Position = vec4(screenOffset, 0.0, 1.0);
}
