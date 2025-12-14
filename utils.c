#include "utils.h"
#include <stdlib.h>

Image *image_create(int width, int height) {
    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;

    img->width = width;
    img->height = height;

    img->data = malloc((size_t)width * height * sizeof(Pixel));
    if (!img->data) {
        free(img);
        return NULL;
    }

    img->pixels = malloc(height * sizeof(Pixel *));
    if (!img->pixels) {
        free(img->data);
        free(img);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        img->pixels[y] = img->data + y * width;
    }

    return img;
}

void image_free(Image *img) {
    if (!img) return;
    free(img->pixels);
    free(img->data);
    free(img);
}
