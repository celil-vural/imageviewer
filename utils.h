//
// Created by celilvural on 12/15/25.
//

#ifndef IMGAVIEWER_UTILS_H
#define IMGAVIEWER_UTILS_H
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


Image *image_create(int width, int height);
void image_free(Image *img);
#endif //IMGAVIEWER_UTILS_H