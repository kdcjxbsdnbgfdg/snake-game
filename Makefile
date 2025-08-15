make:
	gcc -o 2dGame main.c -g -lm -lSDL2 -lglut -DFREEGLUT_STATIC -lOpenGL -lSOIL
