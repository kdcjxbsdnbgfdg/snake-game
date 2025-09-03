#version 460

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec2 texcoord;
layout(binding = 0) uniform Offset{
	vec2 offset;
};
layout(binding = 1) uniform windowSize{
	float aspectRatio;
};

layout(location = 0) out vec3 out_colour;
layout(location = 1) out vec2 out_texcoord;

void main() {
	if (aspectRatio > 1){
		gl_Position = vec4((pos.x + offset.x) / aspectRatio, pos.y + offset.y, 0.0f, 1.0f);
	}
	else{
		gl_Position = vec4(pos.x + offset.x, (pos.y + offset.y) * aspectRatio, 0.0f, 1.0f);
	}
	out_colour = colour;
	out_texcoord = texcoord;
}
