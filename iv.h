//
// Created by celilvural on 12/13/25.
//

#ifndef IMGAVIEWER_IV_H
#define IMGAVIEWER_IV_H
typedef enum {
    IMG_UNKNOWN,
    IMG_PNG,
    IMG_JPEG,
    IMG_GIF,
    IMG_BMP,
    IMG_PPM
} ImageFormat;

unsigned char png_sig[8] = {
    137, 80, 78, 71, 13, 10, 26, 10
};

#endif //IMGAVIEWER_IV_H