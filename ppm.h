#ifndef PPM_H
#define PPM_H

#include <stdint.h>

typedef struct {
    uint8_t r, g, b, a;
} Pixel;

typedef struct {
    Pixel **pixels;
    Pixel *data;
    int width;
    int height;
} Image;

Image *read_ppm_image(const char *filename);
void   image_free(Image *img);
#endif