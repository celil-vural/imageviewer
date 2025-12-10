#include <stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<SDL2/SDL.h>
#include <stdint.h>

typedef struct { uint8_t r,g,b,a; } Pixel;
typedef struct {
	Pixel **pixels;
	Pixel *data;
	int width;
	int height;
} Image;

static void skip_comments(FILE *fp) {
	int ch = fgetc(fp);
	while (ch == '#') {
		while (fgetc(fp) != '\n') {}
		ch = fgetc(fp);
	}
	ungetc(ch, fp);
}
static void read_ppm_header(FILE *fp, int *width, int *height, int *is_p6) {
	char buf[256];
	int maxval;

	// Magic number: P3 or P6
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
		fprintf(stderr, "Only 255 maximum color values are supported. (now: %d)\n", maxval);
		exit(EXIT_FAILURE);
	}
	while (fgetc(fp) != '\n');
}
Image *image_create(int width, int height) {
	Image *img = malloc(sizeof(Image));
	if (!img) return NULL;

	img->width  = width;
	img->height = height;

	img->data = malloc(width * height * sizeof(Pixel));
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
	if (img) {
		free(img->pixels);
		free(img->data);
		free(img);
	}
}
static void read_p6(FILE *fp, Image *img) {
	const size_t pixel_count = img->width * img->height;
	Pixel *ptr = img->data;

	for (size_t i = 0; i < pixel_count; i++) {
		uint8_t r, g, b;
		if (fread(&r, 1, 1, fp) != 1 ||
			fread(&g, 1, 1, fp) != 1 ||
			fread(&b, 1, 1, fp) != 1) {
			fprintf(stderr, "Data reading error\n");
			exit(EXIT_FAILURE);
			}
		ptr->r = r;
		ptr->g = g;
		ptr->b = b;
		ptr->a = 255;
		ptr++;
	}
}
static void read_p3(FILE *fp, Image *img) {
	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			int r, g, b;
			if (fscanf(fp, "%d %d %d", &r, &g, &b) != 3) {
				fprintf(stderr, "Data reading error (row %d, col %d)\n", y + 1, x + 1);
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
		perror("Can't open file");
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ppm_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 1. Start SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    Image *img = read_ppm_image(argv[1]);
    if (!img) {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // 2. Create window and surface
    SDL_Window *window = SDL_CreateWindow("Image Viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        img->width, img->height, SDL_WINDOW_SHOWN);

    if (!window) {
        fprintf(stderr, "Window could not be created : %s\n", SDL_GetError());
        image_free(img);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    if (!surface) {
        fprintf(stderr, "Surface could not be obtained: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        image_free(img);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // 3. Write on 32-bit locked surface
    if (SDL_MUSTLOCK(surface)) {
        if (SDL_LockSurface(surface) < 0) {
            fprintf(stderr, "Surface dont lock: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            image_free(img);
            SDL_Quit();
            return EXIT_FAILURE;
        }
    }

    Uint32 *pixels = (Uint32 *)surface->pixels;
    int pitch = surface->pitch / sizeof(Uint32);

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Pixel p = img->pixels[y][x];
            Uint32 color = SDL_MapRGBA(surface->format, p.r, p.g, p.b, p.a);
            pixels[y * pitch + x] = color;
        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    SDL_UpdateWindowSurface(window);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Delay(16); // ~60 FPS
    }

    // 5. Clear
    image_free(img);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}