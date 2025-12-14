#include "ppm.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void skip_comments(FILE *fp) {
    int ch = fgetc(fp);
    while (ch == '#') {
        while (fgetc(fp) != '\n') { /* skip line */ }
        ch = fgetc(fp);
    }
    if (ch != EOF) ungetc(ch, fp);
}

static void read_ppm_header(FILE *fp, int *width, int *height, int *is_p6) {
    char buf[256];
    int maxval;

    if (!fgets(buf, sizeof(buf), fp) ||
        (strncmp(buf, "P3", 2) != 0 && strncmp(buf, "P6", 2) != 0)) {
        fprintf(stderr, "Invalid PPM header: %s", buf);
        exit(EXIT_FAILURE);
    }
    *is_p6 = (buf[1] == '6');

    skip_comments(fp);
    if (fscanf(fp, "%d", width) != 1) exit(EXIT_FAILURE);
    skip_comments(fp);
    if (fscanf(fp, "%d", height) != 1) exit(EXIT_FAILURE);
    skip_comments(fp);
    if (fscanf(fp, "%d", &maxval) != 1) exit(EXIT_FAILURE);

    if (maxval != 255) {
        fprintf(stderr, "Only maxval=255 supported (got %d)\n", maxval);
        exit(EXIT_FAILURE);
    }

    // consume the newline after maxval
    while (fgetc(fp) != '\n' && !feof(fp)) { /* skip whitespace */ }
}

static void read_p6(FILE *fp, Image *img) {
    size_t total = (size_t)img->width * img->height;
    Pixel *p = img->data;

    for (size_t i = 0; i < total; i++) {
        uint8_t r, g, b;
        if (fread(&r, 1, 1, fp) != 1 ||
            fread(&g, 1, 1, fp) != 1 ||
            fread(&b, 1, 1, fp) != 1) {
            fprintf(stderr, "P6: premature end of file\n");
            exit(EXIT_FAILURE);
        }
        p->r = r; p->g = g; p->b = b; p->a = 255;
        p++;
    }
}

static void read_p3(FILE *fp, Image *img) {
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int r, g, b;
            if (fscanf(fp, "%d %d %d", &r, &g, &b) != 3) {
                fprintf(stderr, "P3: parse error at row %d, col %d\n", y+1, x+1);
                exit(EXIT_FAILURE);
            }
            img->pixels[y][x].r = (uint8_t)r;
            img->pixels[y][x].g = (uint8_t)g;
            img->pixels[y][x].b = (uint8_t)b;
            img->pixels[y][x].a = 255;
        }
    }
}

Image *read_ppm_image(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Can't open PPM file");
        return NULL;
    }

    int width, height, is_p6;
    read_ppm_header(fp, &width, &height, &is_p6);

    Image *img = image_create(width, height);
    if (!img) {
        fclose(fp);
        return NULL;
    }

    if (is_p6) {
        read_p6(fp, img);
    } else {
        read_p3(fp, img);
    }

    fclose(fp);
    return img;
}