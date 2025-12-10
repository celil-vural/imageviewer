### ImageViewer
ImageViewer is a lightweight and simple image viewer project that I started 
developing in the C programming language. At the moment, it can only display PPM images,
but I plan to extend its capabilities to support many other common image formats in the
future.

#### Planned Features
- [ ] Support for widely used image formats
- [ ] Basic image editing tools (crop, resize, rotate)
- [ ] Slideshow functionality
- [ ] User-friendly GUI
- [ ] Cross-platform compatibility

#### Building and Running
To build the project, ensure you have the SDL2 library installed on your system. You can compile the code using the following command:

For fish:

```bash
gcc -o iv iv.c (sdl2-config --cflags --libs)
```

For bash/zsh:

```bash
gcc -o iv iv.c `sdl2-config --cflags --libs`
```

To run the image viewer, use the following command:

```bash
./iv <path_to_ppm_image>.ppm
```