#version 460

layout(location = 0) in vec3 colour;
layout(location = 1) in vec2 texcoord;

out vec4 output_colour;

layout (binding = 0) uniform sampler2D snakeHead;
layout (binding = 1) uniform sampler2D snakeBody;
layout (binding = 2) uniform sampler2D apple;
layout (binding = 3) uniform sampler2D text;

layout (binding = 4) uniform active_tex_index{
	int active_tex;
};
layout (binding = 5) uniform rotation_buffer{
	int rotation;
};

void main() {
	vec2 tex = texcoord;
	mat2 rotMatrix = mat2(
		0, -1,
		1, 0
		);
	for(int i = 0; i < rotation; i++){
		tex *= rotMatrix;
	}
	if(active_tex == 0){
	output_colour = 
		texture(snakeHead, tex);
	}
	else if (active_tex == 1){
		output_colour = texture(snakeBody, tex);
	} 
	else if (active_tex == 2){
		output_colour = texture(apple, tex);
	} 
	else if (active_tex == 3){
		// idk what the .r is for
		output_colour = vec4(1, 1, 1, texture(text, tex).r) * vec4(colour, 1);
	}
		// * vec4(colour, 0.5f)
}
