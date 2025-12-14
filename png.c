#include "png.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define CHUNK_IHDR 0x49484452
#define CHUNK_IDAT 0x49444154
#define CHUNK_IEND 0x49454E44
#define CHUNK_PLTE 0x504C5445
#define CHUNK_tRNS 0x74524E53

static const uint8_t png_signature[8] = {137,80,78,71,13,10,26,10};

static uint32_t be32(const uint8_t *p) {
    return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}
static uint16_t be16(const uint8_t *p) {
    return (p[0]<<8)|p[1];
}
static void read_exact(FILE *fp, void *buf, size_t n) {
    if (fread(buf,1,n,fp)!=n) {
        fprintf(stderr,"PNG: unexpected EOF\n");
        exit(1);
    }
}

static uint8_t paeth(uint8_t a,uint8_t b,uint8_t c){
    int p=a+b-c;
    int pa=abs(p-a), pb=abs(p-b), pc=abs(p-c);
    if(pa<=pb && pa<=pc) return a;
    if(pb<=pc) return b;
    return c;
}

Image *read_png_image(const char *filename) {
    FILE *fp=fopen(filename,"rb");
    if(!fp){ perror("png"); return NULL; }

    uint8_t sig[8];
    read_exact(fp,sig,8);
    if(memcmp(sig,png_signature,8)!=0){
        fclose(fp); return NULL;
    }

    uint32_t width=0,height=0;
    int bit_depth=0,color_type=0,interlace=0;

    uint8_t *palette=NULL,*trans=NULL;
    int palette_entries=0,trans_entries=0;

    uint8_t *idat=NULL;
    size_t idat_size=0,idat_cap=0;

    while(1){
        uint8_t lb[4],tb[4];
        read_exact(fp,lb,4);
        uint32_t len=be32(lb);
        read_exact(fp,tb,4);
        uint32_t type=be32(tb);

        uint8_t *data=len?malloc(len):NULL;
        if(len) read_exact(fp,data,len);
        fseek(fp,4,SEEK_CUR); // CRC skip

        if(type==CHUNK_IHDR){
            width=be32(data);
            height=be32(data+4);
            bit_depth=data[8];
            color_type=data[9];
            interlace=data[12];
            if(interlace!=0){ free(data); goto fail; }
        }
        else if(type==CHUNK_PLTE){
            palette_entries=len/3;
            palette=realloc(palette,len);
            memcpy(palette,data,len);
        }
        else if(type==CHUNK_tRNS){
            trans_entries=len;
            trans=realloc(trans,len);
            memcpy(trans,data,len);
        }
        else if(type==CHUNK_IDAT){
            if(idat_size+len>idat_cap){
                idat_cap=idat_cap+len+8192;
                idat=realloc(idat,idat_cap);
            }
            memcpy(idat+idat_size,data,len);
            idat_size+=len;
        }
        else if(type==CHUNK_IEND){
            free(data);
            break;
        }
        free(data);
    }
    fclose(fp);

    int bits_per_pixel;
    switch(color_type){
        case 0: bits_per_pixel=bit_depth; break;
        case 2: bits_per_pixel=3*bit_depth; break;
        case 3: bits_per_pixel=bit_depth; break;
        case 4: bits_per_pixel=2*bit_depth; break;
        case 6: bits_per_pixel=4*bit_depth; break;
        default: goto fail;
    }

    size_t scanline_bytes=(width*bits_per_pixel+7)/8;
    size_t expected=(scanline_bytes+1)*height;

    uint8_t *raw=malloc(expected);
    z_stream strm={0};
    strm.next_in=idat;
    strm.avail_in=idat_size;
    strm.next_out=raw;
    strm.avail_out=expected;

    inflateInit(&strm);
    int ret=inflate(&strm,Z_FINISH);
    inflateEnd(&strm);
    free(idat);

    if(ret!=Z_STREAM_END) goto fail;

    Image *img=image_create(width,height);
    if(!img) goto fail;

    uint8_t *prev=calloc(1,scanline_bytes);
    uint8_t *curr=malloc(scanline_bytes);
    int bpp=(bits_per_pixel+7)/8;

    for(uint32_t y=0;y<height;y++){
        uint8_t filter=raw[y*(scanline_bytes+1)];
        uint8_t *src=raw+y*(scanline_bytes+1)+1;

        for(size_t i=0;i<scanline_bytes;i++){
            uint8_t a=(i>=bpp)?curr[i-bpp]:0;
            uint8_t b=prev[i];
            uint8_t c=(i>=bpp)?prev[i-bpp]:0;
            uint8_t x=src[i];
            switch(filter){
                case 0: curr[i]=x; break;
                case 1: curr[i]=x+a; break;
                case 2: curr[i]=x+b; break;
                case 3: curr[i]=x+((a+b)>>1); break;
                case 4: curr[i]=x+paeth(a,b,c); break;
            }
        }

        for(uint32_t x=0;x<width;x++){
            Pixel *p=&img->pixels[y][x];
            p->a=255;
            if(color_type==2){
                p->r=curr[x*3];
                p->g=curr[x*3+1];
                p->b=curr[x*3+2];
            }
            else if(color_type==6){
                p->r=curr[x*4];
                p->g=curr[x*4+1];
                p->b=curr[x*4+2];
                p->a=curr[x*4+3];
            }
            else if(color_type==0){
                p->r=p->g=p->b=curr[x];
            }
            else if(color_type==3){
                int idx=curr[x];
                p->r=palette[idx*3];
                p->g=palette[idx*3+1];
                p->b=palette[idx*3+2];
                if(trans && idx<trans_entries) p->a=trans[idx];
            }
            else if(color_type==4){
                p->r=p->g=p->b=curr[x*2];
                p->a=curr[x*2+1];
            }
        }
        memcpy(prev,curr,scanline_bytes);
    }

    free(prev); free(curr); free(raw); free(palette); free(trans);
    return img;

fail:
    free(palette); free(trans); free(idat); return NULL;
}
