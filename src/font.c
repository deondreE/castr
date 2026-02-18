#include <glad/glad.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include "font.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static stbtt_bakedchar cdata[96];

int font_init(Font* font, const char* path, float size) {
    // Get actual file size
    FILE* f = fopen(path, "rb");
    if (!f) {
        log_error("Font file not found: %s", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    unsigned char* ttf_buffer = malloc(file_size);
    if (!ttf_buffer) {
        log_error("Failed to allocate font buffer");
        fclose(f);
        return 0;
    }
    fread(ttf_buffer, 1, file_size, f);
    fclose(f);

    unsigned char* temp_bitmap = malloc(512 * 512);
    if (!temp_bitmap) {
        log_error("Failed to allocate bitmap buffer");
        free(ttf_buffer);
        return 0;
    }

    stbtt_BakeFontBitmap(ttf_buffer, 0, size, temp_bitmap, 512, 512, 32, 96, cdata);
    free(ttf_buffer);

    glGenTextures(1, &font->texture);
    glBindTexture(GL_TEXTURE_2D, font->texture);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);

    const GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(temp_bitmap);

    font->size = size;
    font->chardata = malloc(sizeof(cdata));
    if (!font->chardata) {
        log_error("Failed to allocate chardata");
        glDeleteTextures(1, &font->texture);
        return 0;
    }
    memcpy(font->chardata, cdata, sizeof(cdata));

    log_info("Font initialized successfully: %s", path);
    return 1;
}

void font_draw(Font* font, const char* text, float x, float y) {
    if (!font || !font->texture) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    const char* p = text;
    while (*p) {
        if (*p >= 32 && *p < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(
                (stbtt_bakedchar*)font->chardata,
                512, 512, *p - 32, &x, &y, &q, 0
            );
            glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
            glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
            glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
            glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
        }
        p++;
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}