#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <time.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define TARGET_FPS 6
#define BOARD_LENGTH 16
#define BOARD_SIZE 256

struct segment{
	char xPos;
	char yPos;
};

void spawnApple(struct segment *segments, struct segment *appleCoords, int length){
	int n = 0;
	for(int i = 0; i < BOARD_SIZE; i++){
		if(n == (BOARD_SIZE - 1) - length) break; // idk if i need this line
		int x = i % BOARD_LENGTH;
		int y = i / BOARD_LENGTH;
		for(int j = 0; j < length; j++){
			if(segments[j].xPos == x && segments[j].yPos == y) break;
			if(j == length - 1){
				appleCoords[n].xPos = x;
				appleCoords[n].yPos = y;
				n++;
			}
		}
	}
}

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

void uniformBufferInt(int bufferData, GLuint buffer, int bufferIndex){
	glBindBufferBase(GL_UNIFORM_BUFFER, bufferIndex, buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(bufferData), &bufferData, GL_DYNAMIC_DRAW);
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

// x and y start, x and y scale
void renderText(const char *string, FT_Face face, float x, float y, float scaleX, float scaleY){
	FT_GlyphSlot g = face->glyph;
	for(const char *p = string; *p != 0; p++){ // *p deferences the pointer, giving the char, strings are null terminated
		if(FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, g->bitmap.width, g->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		// the ending coords i think
		float x2 = x + g->bitmap_left * scaleX;
		float y2 = y + g->bitmap_top * scaleY;
		float w = g->bitmap.width * scaleX;
		float h = g->bitmap.rows * scaleY;

		// coords and stuff to draw lol
		float box[4][4] = {
			{x2,     y2,     0, 0},
			{x2,     y2 - h, 0, 1},
			{x2 + w, y2,     1, 0},
			{x2 + w, y2 - h, 1, 1}
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 9);

		// im not fully sure if i need the float cast
		x += ((float)g->advance.x/64) * scaleX;
		y += ((float)g->advance.y/64) * scaleY;
	}

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
		// position,    colour,          texture coords
		0.125f, 0.000f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // bottom right
		0.000f, 0.125f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // top left
		0.000f, 0.000f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // bottom left
		// second triangle
		0.125f, 0.000f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // bottom right
		0.125f, 0.125f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top right
		0.000f, 0.125f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f   // top left
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

	/* active texture buffer */ 
	GLuint activeTextureBuffer;
	glGenBuffers(1, &activeTextureBuffer);
	/* rotation buffer */
	GLuint rotation_buffer;
	glGenBuffers(1, &rotation_buffer);

	/* aspect ratio buffer */
	GLuint aspect_ratio_buffer;
	glGenBuffers(1, &aspect_ratio_buffer);
	//int activeTextureIndex = 1;
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(activeTextureIndex), &activeTextureIndex, GL_DYNAMIC_DRAW);

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
	GLuint snakeHead, snakeBody, apple, text;
	createTexture(snakeBody, GL_TEXTURE0, "snakeHead");
	createTexture(snakeHead, GL_TEXTURE1, "snakeBody");
	createTexture(apple, GL_TEXTURE2, "apple");
	// create text texture
	glActiveTexture(GL_TEXTURE3);
	glGenTextures(1, &text);
	glBindTexture(GL_TEXTURE_2D, text);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// buffer for text rendering
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// where to put this lol
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glUseProgram(shader_program);


	// freetype library
	// for fonts and stuff
	FT_Library ft;
	if(FT_Init_FreeType(&ft)){
		printf("Could not init freetype library\n");
	}
	FT_Face face;
	if(FT_New_Face(ft, "UbuntuMono-R.ttf", 0, &face)){
		printf("could not open font\n");
	}
	FT_Set_Pixel_Sizes(face, 0, 48);
	FT_GlyphSlot g = face->glyph;



	enum moveEnum{
		RIGHT,
		UP,
		LEFT,
		DOWN
	};
	srand(time(NULL));
	enum moveEnum moveDir = rand() % 4;
	char moveX = 0;
	char moveY = 0;
	unsigned int length = 1;

	// board size is 16 x 16
	struct segment segments[BOARD_SIZE];
	// gives random number from 0 to 15
	segments[0].xPos = rand() % BOARD_LENGTH;
	segments[0].yPos = rand() % BOARD_LENGTH;
	//segments[0].age = 0;
	char headX = segments[0].xPos;
	char headY = segments[0].yPos;

	struct segment appleCoords[BOARD_SIZE - 1];

	// go over each position, if it isnt on the segments[] array then put it on the appleCoords array
	spawnApple(segments, appleCoords, length);
	// random number from 0 to 63 - length
	int appleRand = rand() % (BOARD_SIZE - length);
	int appleX = appleCoords[appleRand].xPos;
	int appleY = appleCoords[appleRand].yPos;

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
						moveDir = DOWN; break;
					case SDLK_w:
						moveDir = UP; break;
					case SDLK_a:
						moveDir = LEFT; break;
					case SDLK_d:
						moveDir = RIGHT; break;
				}
				
			}
		}

		// move the head forwards before checking if you ate the apple
		switch (moveDir){
			case RIGHT:
				headX++; break;
			case UP:
				headY++; break;
			case LEFT:
				headX--; break;
			case DOWN:
				headY--; break;
		}
		if(headX == appleX && headY == appleY){
			length++;
			spawnApple(segments, appleCoords, length);
			// random number from 0 to (255 - length)
			appleRand = rand() % (BOARD_SIZE - length);
			appleX = appleCoords[appleRand].xPos;
			appleY = appleCoords[appleRand].yPos;
		}
		// move snake forwards one
		for(int i = length - 1; i > 0; i--){
			segments[i].xPos = segments[i - 1].xPos;
			segments[i].yPos = segments[i - 1].yPos;
		}
		segments[0].xPos = headX;
		segments[0].yPos = headY;

		/* render */
		SDL_GetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glScissor(0, 0, width, height);
		float aspect_ratio = (float) width / height;

		glBindBufferBase(GL_UNIFORM_BUFFER, 1, aspect_ratio_buffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(aspect_ratio), &aspect_ratio, GL_DYNAMIC_DRAW); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		
		// render text
		uniformBufferInt(3, activeTextureBuffer, 4);
		uniformBufferInt(0, rotation_buffer, 5);
		float scaleX = 2.0 / width;
		float scaleY = 2.0 / height;
		posBuffer[0] = 0;
		posBuffer[1] = 0;
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 
		renderText("Press esc to close", face, 0, 0, scaleX, scaleY);


		// draw apple
		uniformBufferInt(2, activeTextureBuffer, 4);
		posBuffer[0] = ((float) appleX / 8) - 1;
		posBuffer[1] = ((float) appleY / 8) - 1;
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// change to head texture
		uniformBufferInt(0, activeTextureBuffer, 4);
		uniformBufferInt(moveDir, rotation_buffer, 5);
		// draw head
		posBuffer[0] = ((float) segments[0].xPos / 8) - 1;
		posBuffer[1] = ((float) segments[0].yPos / 8) - 1;
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// switch to body texture
		uniformBufferInt(1, activeTextureBuffer, 3);

		// draw all body segments
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, movement_buffer);
		for(int i = 1; i < length; i++){
			float x = ((float) segments[i].xPos / 8) - 1;
			float y = ((float) segments[i].yPos / 8) - 1;
			float triangle[42] = {
				// position,    colour,          texture coords
				x + 0.125f, y         , 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // bottom right
				x         , y + 0.125f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // top left
				x         , y         , 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // bottom left
																// second triangle
				x + 0.125f, y         , 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // bottom right
				x + 0.125f, y + 0.125f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top right
				x         , y + 0.125f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f   // top left
			};
			glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW); 
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(2 * sizeof(float)));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(5 * sizeof(float)));
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			//posBuffer[0] = ((float) segments[i].xPos / 8) - 1;
			//posBuffer[1] = ((float) segments[i].yPos / 8) - 1;
			posBuffer[0] = 0; posBuffer[1] = 0;
			glBufferData(GL_UNIFORM_BUFFER, sizeof(posBuffer), posBuffer, GL_DYNAMIC_DRAW); 
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		SDL_GL_SwapWindow(window);

		/* wait for the next frame */
		struct timespec wait;
		wait.tv_sec = 0; // initialise to 0 so it isnt random gibberish
		wait.tv_nsec = 1000000000 / TARGET_FPS;
		clock_nanosleep(CLOCK_MONOTONIC, 0, &wait, NULL);
	} /* while (run == 1) */

	SDL_Quit();
	return 0;
}

