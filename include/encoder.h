#ifndef ENCODER_H
#define ENCODER_H

void init_encoder(const char *filename, int width, int height);
void encode_frame(unsigned char *bgra_data);
void cleanup_encoder();

#endif