#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include "font.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

stbtt_bakedchar cdata[98]; // ASCII 32..116

int font_init(Font* font, const char* path, float size) {
    unsigned char* ttf_buffer = malloc(1<<20);
    unsigned char* temp_bitmap = malloc(512*512);

    FILE* f = fopen(path, "rb");
    if (!f) { 
        log_error("Font not found: %s", path); 
        free(ttf_buffer);
        free(temp_bitmap);
        return 0; 
    }
    fread(ttf_buffer, 1, 1<<20, f);
    fclose(f);

    stbtt_BakeFontBitmap(ttf_buffer, 0, size, temp_bitmap, 512, 512, 32, 96, cdata);
    
    glGenTextures(1, &font->texture);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    // Use GL_RED for single channel font mask
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    font->size = size;
    font->chardata = cdata;
    
    free(ttf_buffer);
    free(temp_bitmap);
    return 1;
}

void font_draw(Font* font, const char* text, float x, float y) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    
    glBegin(GL_QUADS);
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            // x and y are updated by this function (passed by reference)
            stbtt_GetBakedQuad((stbtt_bakedchar*)font->chardata, 512, 512, *text - 32, &x, &y, &q, 1);
            
            glTexCoord2f(q.s0, q.t0); glVertex2f((float)(q.x0/640.0-1.0), (float)(1.0-q.y0/360.0));
            glTexCoord2f(q.s1, q.t0); glVertex2f((float)(q.x1/640.0-1.0), (float)(1.0-q.y0/360.0));
            glTexCoord2f(q.s1, q.t1); glVertex2f((float)(q.x1/640.0-1.0), (float)(1.0-q.y1/360.0));
            glTexCoord2f(q.s0, q.t1); glVertex2f((float)(q.x0/640.0-1.0), (float)(1.0-q.y1/360.0));
        }
        ++text;
    }
    glEnd();
}