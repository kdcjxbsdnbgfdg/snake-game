#version 460

layout(location = 0) in vec3 colour;
layout(location = 1) in vec2 texcoord;

out vec4 output_colour;

layout (binding = 0) uniform sampler2D snakeHead;
layout (binding = 1) uniform sampler2D snakeBody;
layout (binding = 2) uniform sampler2D apple;
layout (binding = 3) uniform active_tex_index{
	int active_tex;
};

void main() {
	if(active_tex == 0){
	output_colour = 
		texture(snakeHead, texcoord);
	}
	else if (active_tex == 1){
		output_colour = texture(snakeBody, texcoord);
	} 
	else if (active_tex == 2){
		output_colour = texture(apple, texcoord);
	} 
		// * vec4(colour, 0.5f)
}
