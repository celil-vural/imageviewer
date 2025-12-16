#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "ppm.h"
#include "png.h"
#include "iv.h"

ImageFormat detect_image_format(FILE *fp) {
	unsigned char buf[8];
	fread(buf, 1, 8, fp);
	fseek(fp, 0, SEEK_SET);

	// PNG
	if (memcmp(buf, png_sig, 8) == 0)
		return IMG_PNG;

	// JPEG
	if (buf[0] == 0xFF && buf[1] == 0xD8)
		return IMG_JPEG;

	// GIF
	if (memcmp(buf, "GIF", 3) == 0)
		return IMG_GIF;

	// BMP
	if (buf[0] == 'B' && buf[1] == 'M')
		return IMG_BMP;

	if (memcmp(buf, "P6", 2) == 0 || memcmp(buf, "P3", 2) == 0)
		return IMG_PPM;

	return IMG_UNKNOWN;
}

Image *read_image(char *filename) {
	Image *img;
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Error opening file: %s\n", filename);
		return NULL;
	}
	const ImageFormat fmt = detect_image_format(fp);
	fclose(fp);
	switch (fmt) {
		case IMG_PPM:
			return read_ppm_image(filename);
		case IMG_UNKNOWN:
			break;
		case IMG_PNG:
			return read_png_image(filename);
		case IMG_JPEG:
			break;
		case IMG_GIF:
			break;
		case IMG_BMP:
			break;
	}
	return NULL;
}
int main(const int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <image.ppm>\n", argv[0]);
		return 1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
		return 1;
	}

	Image *img = read_image(argv[1]);
	if (!img) {
		SDL_Quit();
		return 1;
	}

	SDL_Window *win = SDL_CreateWindow("Image Viewer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		img->width, img->height, SDL_WINDOW_SHOWN);

	if (!win) {
		fprintf(stderr, "Window error: %s\n", SDL_GetError());
		image_free(img);
		SDL_Quit();
		return 1;
	}

	SDL_Surface *surf = SDL_GetWindowSurface(win);
	if (!surf) {
		fprintf(stderr, "Surface error: %s\n", SDL_GetError());
		SDL_DestroyWindow(win);
		image_free(img);
		SDL_Quit();
		return 1;
	}

	if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);

	Uint32 *pixels = surf->pixels;
	const int pitch = surf->pitch / sizeof(Uint32);


	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			const Pixel p = img->pixels[y][x];
			pixels[y * pitch + x] = SDL_MapRGBA(surf->format, p.r, p.g, p.b, p.a);
		}
	}

	if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);

	SDL_UpdateWindowSurface(win);

	bool running = true;
	SDL_Event e;
	while (running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = false;
		}
		SDL_Delay(16);
	}

	image_free(img);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}