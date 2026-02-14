#ifndef FONT_H
#define FONT_H

#include <glad/glad.h>

typedef struct {
	GLuint texture;
	float size;
	void* chardata;
} Font;

int font_init(Font* font, const char* path, float size);
void font_draw(Font* font, const char* text, float x, float y);

#endif