#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "ppm.h"

int main(const int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <image.ppm>\n", argv[0]);
		return 1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
		return 1;
	}

	Image *img = read_ppm_image(argv[1]);
	if (!img) {
		SDL_Quit();
		return 1;
	}

	SDL_Window *win = SDL_CreateWindow("PPM Viewer",
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
	int pitch = surf->pitch / sizeof(Uint32);

	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			Pixel p = img->pixels[y][x];
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