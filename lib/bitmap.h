#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------
// STRUCTURES
// ----------

//struct for the file header of the BMP file.
#pragma pack(1)
struct BMPFileHeader {
    unsigned short type;        //should say "BM"
    unsigned int file_size;     //file size
    unsigned short reserved1;   //unused (0)
    unsigned short reserved2;   //unused (0)
    unsigned int off_bits;      //offset of pixel data
};
#pragma pack()

//struct for the image header of the BMP file. 40 bytes
struct BMPImageHeader {
    unsigned int size;        //header size (40)
    unsigned int width;       //image width
    unsigned int height;      //image height
    unsigned short planes;    //no planes (1)
    unsigned short bit_count; //bits per pixel
    unsigned int cmpr_typ;    //compression type 0=none
    unsigned int size_image;  //may be 0 for uncompressed
    unsigned int x_PPM;       //preffered x pixels per meter
    unsigned int y_PPM;       //preffered y pixels per meter
    unsigned int clr_used;    //number of color map entries
    unsigned int clr_sig;     //number of signigicant colors
};

//struct for the whole bitmap
struct BMP {
    int scanline_size;                   //size of scanlines (rows) in bytes after padding 
    struct BMPFileHeader *file_header;   //contains file information
    struct BMPImageHeader *image_header; //contains image information
    unsigned char *pixel_data;           //array of pixel values.
};

//struct for storing pixel color values 0 - 255
struct Pixel {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

//function declarations
int get_scanline_size(int image_width);

struct BMP *load_BMP(char *path);

struct BMP *init_BMP(unsigned int width,
                     unsigned int height);

int save_BMP(struct BMP *bmp,
             char *path);

void free_BMP(struct BMP *bmp);

struct Pixel get_pixel(struct BMP *bmp,
                       int x,
                       int y);

int set_pixel(struct BMP *bmp,
              int x,
              int y,
              struct Pixel pixel);

struct Pixel make_pixel(unsigned char r,
                        unsigned char g,
                        unsigned char b);

struct Pixel greyscale_pixel(struct Pixel p);

int greyscale_BMP(struct BMP *bmp);

struct BMP *get_difference(struct BMP *b1,
                           struct BMP *b2);

struct BMP *segment_BMP(struct BMP *img,
                        unsigned char threshold);

struct BMP *clone_BMP(struct BMP *img);

int pixels_match(struct Pixel p1,
                 struct Pixel p2);

int is_foreground(struct Pixel p);

int is_background(struct Pixel p);

// ---------
// FUNCTIONS
// ---------

//calculate scanline size from width of image
int get_scanline_size(int image_width) {
    int sl = 3 * image_width;
    while ((sl % 4) != 0)
        sl++; //account for padding up to 4 byte multiple
    return sl;
}

//init bmp of given dimensions, with all pixels 0
struct BMP *init_BMP(unsigned int width,
                     unsigned int height) {
    struct BMP *bmp;
    struct BMPImageHeader *ih;
    struct BMPFileHeader *fh;
    
    bmp = malloc(sizeof(struct BMP));
    ih = calloc(sizeof(struct BMPImageHeader), 1);
    fh = calloc(sizeof(struct BMPFileHeader), 1);
    
    if (!bmp || !ih || !fh) {
        free(bmp);
        free(ih);
        free(fh);
        return NULL;
    }
    
    bmp->scanline_size = get_scanline_size(width);
    bmp->file_header = fh;
    bmp->image_header = ih;
                        
    bmp->pixel_data = calloc((bmp->scanline_size * height), 1);
    if (!bmp->pixel_data) {
        free(bmp);
        free(ih);
        free(fh);
        return NULL;
    }
    
    fh->type = (((short) 77) << 8) | 66;
    fh->file_size = 54 + (bmp->scanline_size * height);
    fh->reserved1 = 0;
    fh->reserved2 = 0;
    fh->off_bits = 54;
    
    ih->size = 40;
    ih->width = width;
    ih->height = height;
    ih->planes = 1;
    ih->bit_count = 24;
    ih->cmpr_typ = 0;
    ih->x_PPM = 0;
    ih->y_PPM = 0;
    ih->clr_used = 0;
    ih->clr_sig = 0;
    
    return bmp;
}

//loads a bmp file from the given path to a structure and returns a pointer
struct BMP *load_BMP(char *path) {
    //initial declarations
    FILE *file;
    struct BMPFileHeader *file_header;
    struct BMPImageHeader *image_header;
    struct BMP *bitmap;
    
    //open file for reading
    file = fopen(path, "rb");
    if (!file)
        return NULL;
    
    //allocate memory for structs
    file_header = malloc(sizeof(struct BMPFileHeader));
    image_header = malloc(sizeof(struct BMPImageHeader));
    bitmap = malloc(sizeof(struct BMP));
    
    if (!file_header || !image_header || !bitmap)
        return NULL;
    
    //read file header (14 bytes) from file into struct
    if (fread(file_header, 14, 1, file) == 0)
        return NULL;
    
    //read image header (40 bytes) from file into struct
    if (fread(image_header, 40, 1, file) == 0)
        return NULL;
    
    //point BitMap header pointers to corresponding structs
    bitmap->file_header = file_header;
    bitmap->image_header = image_header;
    
    //allocate for pixel data
    int pdSize = (file_header->file_size) - 54; //pixel data size
    bitmap->pixel_data = malloc(pdSize * sizeof(unsigned char));
    
    if (!bitmap->pixel_data)
        return NULL;
    
    //read rest of file into pixel data array
    if (fread(bitmap->pixel_data, pdSize, 1, file) == 0)
        return NULL;
    
    //close file
    fclose(file);
    
    //calculate scanline width
    bitmap->scanline_size = get_scanline_size(image_header->width);
    
    //return pointer to BitMap struct
    return bitmap;
}

//saves a BMP to the given path
//returns 1 on succes
int save_BMP(struct BMP *bmp,
             char *path) {
    //open file
    FILE *file;
    file = fopen(path, "wb");
    
    if (!file)
        return 0;
    
    //write file header, image header, and pixel data
    if (fwrite(bmp->file_header, 14, 1, file) == 0)
        return 0;
    if (fwrite(bmp->image_header, 40, 1, file) == 0)
        return 0;
    if (fwrite(bmp->pixel_data, (bmp->file_header->file_size - 54), 1, file) == 0)
        return 0;
    
    //close file
    fclose(file);
    
    return 1;
}

//frees all memory from given BMP structure
void free_BMP(struct BMP *bmp) {
    free(bmp->file_header);
    free(bmp->image_header);
    free(bmp->pixel_data);
    free(bmp);
}

//gets the pixel at the given coordinates from the given BMP
struct Pixel get_pixel(struct BMP *bmp,
                       int x,
                       int y) {
    struct Pixel pixel;
    
    //check if coordinates are within image bounds
    if (x < 0 || y < 0 || x >= bmp->image_header->width || y >= bmp->image_header->height)
        return pixel;    
    //calculate pixel_data array base_index and get values
    int base_index = ((bmp->image_header->height - y - 1) * bmp->scanline_size) + (3 * x);
    pixel.blue  = bmp->pixel_data[base_index];
    pixel.green = bmp->pixel_data[base_index + 1];
    pixel.red   = bmp->pixel_data[base_index + 2];
    return pixel;
}

//set the Pixel at the given coordinate in the given BMP to the given Pixel
//returns 1 if pixel value set else returns 0
int set_pixel(struct BMP *bmp,
              int x,
              int y,
              struct Pixel pixel) {
    //check if coordinates are within image bounds
    if (x < 0 || y < 0 || x >= bmp->image_header->width || y >= bmp->image_header->height)
        return 0;
    
    //calculate pixel_data array base_index and get values
    int base_index = ((bmp->image_header->height - y - 1) * bmp->scanline_size) + (3 * x);
    bmp->pixel_data[base_index]     = pixel.blue;
    bmp->pixel_data[base_index + 1] = pixel.green;
    bmp->pixel_data[base_index + 2] = pixel.red;
    return 1;
}

//create pixel with given rgb values
struct Pixel make_pixel(unsigned char r,
                        unsigned char g,
                        unsigned char b) {
    struct Pixel p;
    p.red   = r;
    p.green = g;
    p.blue  = b;
    return p;
}

//convert Pixel to greyscale
struct Pixel greyscale_pixel(struct Pixel p) {
    struct Pixel g;
    unsigned char avg = (p.red + p.green + p.blue) / 3;
    g.red = avg;
    g.green = avg;
    g.blue = avg;
    return g;
}

//convert BMP image to greyscale
int greyscale_BMP(struct BMP *bmp) {
    int x, y;
    for (x=0; x<bmp->image_header->width; x++)
        for (y=0; y<bmp->image_header->height; y++)
            if (!set_pixel(bmp, x, y, greyscale_pixel(get_pixel(bmp, x, y))))
                return 0;
    return 1;
}

//creates image from the difference in pixel values between b1 and b2
struct BMP *get_difference(struct BMP *b1,
                           struct BMP *b2) {
    //check images are the same dimensions
    if (b1->image_header->width != b2->image_header->width 
        && b1->image_header->height != b2->image_header->height)
        return NULL;
    
    //create new BMP to store the difference images
    struct BMPFileHeader *file_header;
    struct BMPImageHeader *image_header;
    struct BMP *diff;
    
    //allocate memory for structs
    file_header = malloc(sizeof(struct BMPFileHeader));
    image_header = malloc(sizeof(struct BMPImageHeader));
    diff = malloc(sizeof(struct BMP));
    
    if (!file_header || !image_header || !diff)
        return NULL;
    
    //copy header of b1 to new BMP header
    memcpy(file_header, b1->file_header, 14);
    memcpy(image_header, b1->image_header, 40);
    
    //point diff pointers to headers
    diff->file_header  = file_header;
    diff->image_header = image_header;
    
    int pdSize = b1->file_header->file_size - 54;
    diff->pixel_data = malloc(pdSize);
    
    //write difference between b1 and b2 to diff's pixel_data
    int i;
    for (i=0; i<pdSize; i++) {
        diff->pixel_data[i] = abs(b1->pixel_data[i] - b2->pixel_data[i]);
    }

    //write scanline_size to diff
    diff->scanline_size = get_scanline_size(b1->image_header->width);

    //return pointer to new BMP
    return diff;
}

//creates an image where in the given image if a pixel value
//is above the threshold, the output pixel is white, else
//the pixel is black.
//intended for use with a greyscaled 'difference image'
struct BMP *segment_BMP(struct BMP *img,
                        unsigned char threshold) {
    struct BMP *segMap;
    int i, max;
    
    max = get_scanline_size(img->image_header->width) * img->image_header->height;
    segMap = init_BMP(img->image_header->width, img->image_header->height);
    
    for (i = 0; i < max; i++) {
        if (img->pixel_data[i] > threshold) {
            segMap->pixel_data[i] = 255;
        } else {
            segMap->pixel_data[i] = 0;
        }
    }
    
    return segMap;
}

//clones the given BMP image
struct BMP *clone_BMP(struct BMP *img) {
    struct BMP *clone;
    int pd_size = img->image_header->height * img->scanline_size;
    
    clone = init_BMP(img->image_header->width, img->image_header->height);
    
    memcpy(clone->file_header, img->file_header, sizeof(struct BMPFileHeader));
    memcpy(clone->image_header, img->image_header, sizeof(struct BMPImageHeader));
    memcpy(clone->pixel_data, img->pixel_data, pd_size);
    
    return clone;
}

int pixels_match(struct Pixel p1,
                 struct Pixel p2) {
    return (p1.red == p2.red &&
            p1.green == p2.green &&
            p1.blue == p2.blue);
}

int is_foreground(struct Pixel p) {
    return (p.red == 255 && p.green == 255 && p.blue == 255);
}

int is_background(struct Pixel p) {
    return (p.red == 0 && p.green == 0 && p.blue == 0);
}
