#include <stdlib.h>
#include <string.h>

// ----------
// STRUCTURES
// ----------

struct MedianModel {
    struct BMPFileHeader *file_header;
    struct BMPImageHeader *image_header;
    struct CPQueue *bgs;
    int n;
};

struct CPQueue {
    unsigned char *ptr;
    struct CPQueue *next;
};

//function declarations
struct MedianModel *init_median_model(struct BMP *base,
                                      int n);

struct BMP *generate_median_seg_map(struct MedianModel *model,
                                    struct BMP *img,
                                    unsigned char threshold);

void update_median_model(struct MedianModel *model,
                         struct BMP *seg_map,
                         struct BMP *img);

struct BMP *generate_median_background(struct MedianModel *model);

void free_median_model(struct MedianModel *model);

void free_cpqueue(struct CPQueue *cpq);

int uns_char_cmp(const void *p1, 
                 const void *p2);

// ---------
// FUNCTIONS
// ---------

//initializes a median model from the base image with n backgrounds
struct MedianModel *init_median_model(struct BMP *base, 
                                      int n) {
    struct MedianModel *model;
    struct CPQueue *tmp;
    int i, pd_size;
    
    pd_size = get_scanline_size(base->image_header->width) * base->image_header->height;

    //allocate memory for model
    model = malloc(sizeof(struct MedianModel));
    model->file_header = malloc(sizeof(struct BMPFileHeader));
    model->image_header = malloc(sizeof(struct BMPImageHeader));
    model->bgs = malloc(sizeof(struct CPQueue));
    if (!model ||
        !model->file_header ||
        !model->image_header ||
        !model->bgs)
        return NULL;
    model->n = n;
    //copy headers from base's headers
    memcpy(model->file_header, base->file_header, sizeof(struct BMPFileHeader));
    memcpy(model->image_header, base->image_header, sizeof(struct BMPImageHeader));
    //create queue of image data of size n
    tmp = model->bgs;
    for (i = 0; i < n-1; i++) {
        tmp->ptr = calloc(pd_size, 1);
        tmp->next = malloc(sizeof(struct CPQueue));
        if (!tmp->ptr || !tmp->next)
            return NULL;
        tmp = tmp->next;
    }
    tmp->ptr = calloc(pd_size, 1);
    if (!tmp->ptr)
        return NULL;
    tmp->next = NULL;
    
    //for each element in queue, copy base image pixel data to it
    tmp = model->bgs;
    
    while (tmp) {
        memcpy(tmp->ptr, base->pixel_data, pd_size);
        tmp = tmp->next;
    }
    return model;
}

//generates the segmentation map between the given image and model
//thresholding the difference image with the given value
struct BMP *generate_median_seg_map(struct MedianModel *model,
                                    struct BMP *img,
                                    unsigned char threshold) {
    struct BMP *bg, *diff, *seg_map;
    bg = generate_median_background(model);
    diff = get_difference(img, bg);
    greyscale_BMP(diff);
    seg_map = segment_BMP(diff, threshold);
    
    free_BMP(bg);
    free_BMP(diff);
    return seg_map;
}

//updates the given model based on the img and it's segmentation map
void update_median_model(struct MedianModel *model,
                         struct BMP *seg_map,
                         struct BMP *img) {
    struct BMP *new_img, *bg;
    struct CPQueue *tmp, *tl;
    int i, pd_size;
    
    new_img = clone_BMP(img);
    
    bg = generate_median_background(model);
    
    pd_size = get_scanline_size(model->image_header->width) * model->image_header->height;
    
    //at each pixel where seg_map[i] == 255 (motion) replace with median
    //from the background
    for (i = 0; i < pd_size; i++) {
        if (seg_map->pixel_data[i] == 255) {
            new_img->pixel_data[i] = bg->pixel_data[i];
        }
    }
    
    //store head of queue for removal
    tmp = model->bgs;
    
    //point head to next element
    model->bgs = model->bgs->next;
    
    //free pixel data and head element
    free(tmp->ptr);
    free(tmp);
    
    //seek to end of queue
    tmp = model->bgs;
    while (tmp->next) {
        tmp = tmp->next;
    }
    //malloc for new tail element
    tl = malloc(sizeof(struct CPQueue));
    tl->ptr = calloc(pd_size, 1);
    tl->next = NULL;
    
    //copy data from new_img
    memcpy(tl->ptr, new_img->pixel_data, pd_size);
    
    //point end of queue to new element
    tmp->next = tl;
    
    free_BMP(bg);
    free_BMP(new_img);
}

//generates an image from the median values of all backgrounds held
struct BMP *generate_median_background(struct MedianModel *model) {
    struct BMP *bg;
    struct CPQueue *tmp;
    unsigned char vals[model->n];
    int i, j, k, pd_size;
    
    //init bg image
    bg = init_BMP(model->image_header->width, model->image_header->height);
    if (!bg)
        return NULL;
    
    pd_size = get_scanline_size(model->image_header->width) * model->image_header->height;
    
    for (i = 0; i < pd_size; i++) {
        //collect all values at position i.
        tmp = model->bgs;
        j = 0;
        while (tmp) {
            vals[j++] = tmp->ptr[i];
            tmp = tmp->next;
        }
        //calculate median and add to image
        qsort(vals, model->n, sizeof(unsigned char), uns_char_cmp);
        bg->pixel_data[i] = vals[(model->n-1) / 2];
    }
    
    return bg;
}

//frees the given median model
void free_median_model(struct MedianModel *model) {
    free(model->file_header);
    free(model->image_header);
    free_cpqueue(model->bgs);
    free(model);
}

//frees a CPQueue
void free_cpqueue(struct CPQueue *cpq) {
    if (cpq->next != NULL) {
        free_cpqueue(cpq->next);
    }
    free(cpq->ptr);
    free(cpq);
}

//compares 2 unsigned chars, used for qsort
int uns_char_cmp(const void *p1, 
                 const void *p2) {
    return *(unsigned char*)p1 - *(unsigned char*)p2;
}
