#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <time.h>


void createShader(GLuint shader, char *name){
	FILE *filePointer = fopen(name , "r");	
	char *shader_source = NULL;
	size_t size;
	int strLength = getdelim(&shader_source, &size, EOF, filePointer);
	//printf("%s", shader_source);
	fclose(filePointer);
	glShaderSource(shader, 1, (const char *const *) &shader_source, &strLength);
	free(shader_source);
	glCompileShader(shader);
	int shader_compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);
	if(!shader_compiled){
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(shader, 1024, &log_length, message);
		printf("%s", message);
	}
}

// might change name to be texname
void createTexture(GLuint texture, int activeTexture, char *name){
	glActiveTexture(activeTexture);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	float colour[] = {0.5f, 0, 0.5f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colour);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	int texWidth, texHeight, channels;
	// must be 2^n * 2^n
	char texName[sizeof("textures/.png") + strlen(name)];
	strcpy(texName, "textures/");
	strcat(texName, name);
	strcat(texName, ".png");
	unsigned char *image = stbi_load(texName, &texWidth, &texHeight, &channels, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(image);

}


int main(int argc, char *argv[]){
	/* SDL */
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	/* GL context */
	SDL_Window *window = SDL_CreateWindow("snake", 0, 0, 2560, 1440, 
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);

	/* GL Library functions */
	glutInit(&argc, argv);
	glClearColor(0.16f, 0.4f, 0.484f, 1.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	/* viewport size */
	int width, height;
	// address of where to store width and height
	SDL_GetWindowSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);

	/* triangle
	 * coords go from (-1, -1) to (1, 1) */
	float triangle[42] = {
		// position, colour,          texture coords
		0.125f, 0.000f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // bottom right
		0.000f, 0.125f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,   // top left
		0.000f, 0.000f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // bottom left
		// second triangle
		0.125f, 0.000f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // bottom right
		0.125f, 0.125f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // top right
		0.000f, 0.125f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // top left
	};
	
	/* Create Buffers */
	GLuint vertex_array, vertex_buffer;
	glGenVertexArrays(1, &vertex_array);
	glGenBuffers(1, &vertex_buffer);
	glBindVertexArray(vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);	

	/* movement buffer */
	GLuint movement_buffer;
	glGenBuffers(1, &movement_buffer);
	float posBuffer[2] = {0, 0};
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 

	/* aspect ratio buffer */
	GLuint aspect_ratio_buffer;
	glGenBuffers(1, &aspect_ratio_buffer);
	/* active texture buffer */ 
	GLuint activeTextureBuffer;
	glGenBuffers(1, &activeTextureBuffer);
	int activeTextureIndex = 1;
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, activeTextureBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(activeTextureIndex), &activeTextureIndex, GL_DYNAMIC_DRAW);

	/* store data in buffers */
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW); 
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(2 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(5 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	
	/* Create shaders and link into shader program */
	GLuint vertex_shader, fragment_shader;
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	createShader(vertex_shader, "vertex.glsl");
	createShader(fragment_shader, "fragment.glsl");

	/* create and link shader program */
	GLuint shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	
	/* create textures */
	GLuint snakeHead, snakeBody, apple;
	createTexture(snakeBody, GL_TEXTURE0, "snakeHead");
	createTexture(snakeHead, GL_TEXTURE1, "snakeBody");
	createTexture(apple, GL_TEXTURE2, "apple");

	// where to put this lol
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_BLEND );
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glUseProgram(shader_program);

	char moveX = 0;
	char moveY = 0;
	unsigned int length = 1;

	struct segment{
		char xPos;
		char yPos;
		unsigned int age;
	};
	struct segment segments[8*8];
	// gives random number from 0 to 15
	srand(time(NULL));
	segments[0].xPos = rand() % 16;
	segments[0].yPos = rand() % 16;
	segments[0].age = 0;
	char headX = segments[0].xPos;
	char headY = segments[0].yPos;

	int appleX;
	int appleY;
	int applelocaitons = 64 - length;


	int run = 1;
	while(run == 1){
		/* input */
		static SDL_Event event;
		while(SDL_PollEvent(&event)){
			if (event.type == SDL_QUIT) run = 0;
			else if (event.type == SDL_KEYDOWN 
					&& event.key.keysym.sym == SDLK_ESCAPE) run = 0;
			else if(event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
					case SDLK_s:
						moveY = -1;
						break;
					case SDLK_w:
						moveY = 1;
						break;
					case SDLK_a:
						moveX = -1;
						break;
					case SDLK_d:
						moveX = 1;
						break;
				}
				// makes a new head at the front and puts it in an available spot in the array
				
				for(int i = 0; i < length; i++){
					segments[i].age++;
					if(segments[i].age == length){
						segments[i].age = 0;
						headX += moveX;
						headY += moveY;
						segments[i].xPos = headX;
						segments[i].yPos = headY;
						moveX = 0;
						moveY = 0;
					}
				}
			}
		}

		
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
		for(int i = 0; i < length; i++){
			if(segments[i].age != length){
				posBuffer[0] = ((float) segments[i].xPos / 8) - 1;
				posBuffer[1] = ((float) segments[i].yPos / 8) - 1;
				glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		/* render */
		SDL_GetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glScissor(0, 0, width, height);
		float aspect_ratio = (float) width / height;

		glBindBufferBase(GL_UNIFORM_BUFFER, 1, aspect_ratio_buffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(aspect_ratio), &aspect_ratio, GL_DYNAMIC_DRAW); 

		int activeTextureIndex = 0;
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, activeTextureBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(activeTextureIndex), &activeTextureIndex, GL_DYNAMIC_DRAW);


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		glDrawArrays(GL_TRIANGLES, 0, 6);
		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();
	return 0;
}
