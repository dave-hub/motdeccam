#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 4

// ----------
// STRUCTURES
// ----------

struct JobGreyscale {
    struct BMP *bmp;
    int step;
};

struct JobDifference {
    struct BMP *b1;
    struct BMP *b2;
    struct BMP *diff;
    int step;
};

struct JobSegment {
    struct BMP *bmp;
    struct BMP *seg_map;
    unsigned char threshold;
    int step;
};

struct JobCount {
    struct BMP *img;
    struct Pixel p;
    long count;
    int step;
};

// ------------
// DECLARATIONS
// ------------

int greyscale_BMP_thr(struct BMP *bmp);

struct BMP *get_difference_thr(struct BMP *b1,
                               struct BMP *b2);

struct BMP *segment_BMP_thr(struct BMP *img,
                            unsigned char threshold);

long count_pixels_thr(struct BMP *img,
                     struct Pixel p);

//job declarations

void *do_job_greyscale(void *job_struct);

void *do_job_difference(void *job_struct);

void *do_job_segment(void *job_struct);

void *do_job_count(void *job_struct);

struct JobGreyscale *create_job_greyscale(struct BMP *bmp,
                                          int step);

struct JobDifference *create_job_difference(struct BMP *b1,
                                            struct BMP *b2,
                                            struct BMP *diff,
                                            int step);

struct JobSegment *create_job_segment(struct BMP *b1,
                                      struct BMP *seg_map,
                                      unsigned char threshold,
                                      int step);

struct JobCount *create_job_count(struct BMP *img,
                                  struct Pixel p,
                                  int step);

// ---------
// FUNCTIONS
// ---------

//convert BMP image to greyscale
int greyscale_BMP_thr(struct BMP *bmp) {
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobGreyscale *t1_job, *t2_job, *t3_job, *t4_job;
                
    //create jobs
    t1_job = create_job_greyscale(bmp, 0);
    t2_job = create_job_greyscale(bmp, 1);
    t3_job = create_job_greyscale(bmp, 2);
    t4_job = create_job_greyscale(bmp, 3);
            
    //create threads
    if (pthread_create(&t1, NULL, do_job_greyscale, t1_job) ||
        pthread_create(&t2, NULL, do_job_greyscale, t2_job) ||
        pthread_create(&t3, NULL, do_job_greyscale, t3_job) ||
        pthread_create(&t4, NULL, do_job_greyscale, t4_job)) {
        return 0;
    }
            
    //wait for threads to join
    if (pthread_join(t1, NULL) ||
        pthread_join(t2, NULL) ||
        pthread_join(t3, NULL) ||
        pthread_join(t4, NULL)) {
        return 0;
    }
    
    //free job structs
    free(t1_job);
    free(t2_job);
    free(t3_job);
    free(t4_job);
    
    return 1;
}

//creates image from the difference in pixel values between b1 and b2
struct BMP *get_difference_thr(struct BMP *b1,
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
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobDifference *t1_job, *t2_job, *t3_job, *t4_job;
    
    //create jobs
    t1_job = create_job_difference(b1, b2, diff, 0);
    t2_job = create_job_difference(b1, b2, diff, 1);
    t3_job = create_job_difference(b1, b2, diff, 2);
    t4_job = create_job_difference(b1, b2, diff, 3);
        
    //create threads
    if (pthread_create(&t1, NULL, do_job_difference, t1_job) ||
        pthread_create(&t2, NULL, do_job_difference, t2_job) ||
        pthread_create(&t3, NULL, do_job_difference, t3_job) ||
        pthread_create(&t4, NULL, do_job_difference, t4_job)) {
        return NULL;
    }
    
    //wait for threads to join
    if (pthread_join(t1, NULL) ||
        pthread_join(t2, NULL) ||
        pthread_join(t3, NULL) ||
        pthread_join(t4, NULL)) {
        return NULL;
    }
    
    //free job structs
    free(t1_job);
    free(t2_job);
    free(t3_job);
    free(t4_job);
    
    //write scanline_size to diff
    diff->scanline_size = get_scanline_size(b1->image_header->width);

    //return pointer to new BMP
    return diff;
}

//creates an image where in the given image if a pixel value
//is above the threshold, the output pixel is white, else
//the pixel is black.
//intended for use with a greyscaled 'difference image'
struct BMP *segment_BMP_thr(struct BMP *img,
                            unsigned char threshold) {
    struct BMP *seg_map;
    seg_map = init_BMP(img->image_header->width, img->image_header->height);
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobSegment *t1_job, *t2_job, *t3_job, *t4_job;
    
    //create jobs
    t1_job = create_job_segment(img, seg_map, threshold, 0);
    t2_job = create_job_segment(img, seg_map, threshold, 1);
    t3_job = create_job_segment(img, seg_map, threshold, 2);
    t4_job = create_job_segment(img, seg_map, threshold, 3);
    
    //create threads
    if (pthread_create(&t1, NULL, do_job_segment, t1_job) ||
        pthread_create(&t2, NULL, do_job_segment, t2_job) ||
        pthread_create(&t3, NULL, do_job_segment, t3_job) ||
        pthread_create(&t4, NULL, do_job_segment, t4_job)) {
        return NULL;
    }
    
    //wait for threads to join
    if (pthread_join(t1, NULL) ||
        pthread_join(t2, NULL) ||
        pthread_join(t3, NULL) ||
        pthread_join(t4, NULL)) {
        return NULL;
    }
    
    //free job structs
    free(t1_job);
    free(t2_job);
    free(t3_job);
    free(t4_job);
    
    return seg_map;
}

//counts the number of pixels matching p in img
long count_pixels_thr(struct BMP *img,
                     struct Pixel p) {
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobCount *t1_job, *t2_job, *t3_job, *t4_job;
    int full_count;
    
    full_count = 0;
    
    //create jobs
    t1_job = create_job_count(img, p, 0);
    t2_job = create_job_count(img, p, 1);
    t3_job = create_job_count(img, p, 2);
    t4_job = create_job_count(img, p, 3);
    
    //create threads
    if (pthread_create(&t1, NULL, do_job_count, t1_job) ||
        pthread_create(&t2, NULL, do_job_count, t2_job) ||
        pthread_create(&t3, NULL, do_job_count, t3_job) ||
        pthread_create(&t4, NULL, do_job_count, t4_job)) {
        return -1;
    }
    
    //wait for threads to join
    if (pthread_join(t1, NULL) ||
        pthread_join(t2, NULL) ||
        pthread_join(t3, NULL) ||
        pthread_join(t4, NULL)) {
        return -1;
    }
    
    //gather counts from each job
    full_count = t1_job->count + t2_job->count + t3_job->count + t4_job->count;
    
    //free job structs
    free(t1_job);
    free(t2_job);
    free(t3_job);
    free(t4_job);
    
    return full_count;
}

//job functions

void *do_job_greyscale(void *job_struct) {
    struct JobGreyscale *job = (struct JobGreyscale *) job_struct;
    struct BMP *bmp;
    int step;
    int x, y;
    
    bmp = job->bmp;
    step = job->step;
    
    for (y = 0; y < bmp->image_header->height; y++) {
        for (x = step; x < bmp->image_header->width; x += NUM_THREADS) {
            set_pixel(bmp, x, y, greyscale_pixel(get_pixel(bmp, x, y)));
        }
    }
    return NULL;
}

void *do_job_difference(void *job_struct) {
    struct JobDifference *job = (struct JobDifference *) job_struct;
    struct BMP *b1, *b2, *diff;
    int step;
    int pdsize;
    int i;
    
    b1 = job->b1;
    b2 = job->b2;
    diff = job->diff;
    step = job->step;
    pdsize = b1->image_header->height * get_scanline_size(b1->image_header->width);
    
    
    for (i = step; i < pdsize; i += NUM_THREADS) {
        diff->pixel_data[i] = abs(b1->pixel_data[i] - b2->pixel_data[i]);
    }
    
    return NULL;
}

void *do_job_segment(void *job_struct) {
    struct JobSegment *job = (struct JobSegment *) job_struct;
    struct BMP *bmp, *seg_map;
    unsigned char threshold;
    int step;
    int pdsize;
    int i;
    
    bmp = job->bmp;
    seg_map = job->seg_map;
    threshold = job->threshold;
    step = job->step;
    pdsize = bmp->image_header->height * get_scanline_size(bmp->image_header->width);
    
    for (i = step; i < pdsize; i += NUM_THREADS) {
        if (bmp->pixel_data[i] > threshold) {
            seg_map->pixel_data[i] = 255;
        } else {
            seg_map->pixel_data[i] = 0;
        }   
    }
    
    return NULL;
}

void *do_job_count(void *job_struct) {
    struct JobCount *job = (struct JobCount *) job_struct;
    struct BMP *img;
    struct Pixel p, t;
    int step;
    int x, y;
    int count;
    
    img = job->img;
    p = job->p;
    step = job->step;
    
    count = 0;
    
    for (y = 0; y < img->image_header->height; y++) {
        for (x = step; x < img->image_header->width; x += NUM_THREADS) {
            t = get_pixel(img, x, y);
            if (t.red == p.red && t.green == p.green && t.blue == p.blue) {
                count++;
            }
        }
    }
    job->count = count;
    return NULL;
}

struct JobGreyscale *create_job_greyscale(struct BMP *bmp,
                                          int step) {
    struct JobGreyscale *job;
    job = malloc(sizeof(struct JobGreyscale));
    if (!job)
        return NULL;
    job->bmp = bmp;
    job->step = step;
    return job;
}

struct JobDifference *create_job_difference(struct BMP *b1,
                                            struct BMP *b2,
                                            struct BMP *diff,
                                            int step) {
    struct JobDifference *job;
    job = malloc(sizeof(struct JobDifference));
    if (!job)
        return NULL;
    job->b1 = b1;
    job->b2 = b2;
    job->diff = diff;
    job->step = step;
    return job;
}

struct JobSegment *create_job_segment(struct BMP *bmp,
                                      struct BMP *seg_map,
                                      unsigned char threshold,
                                      int step) {
    struct JobSegment *job;
    job = malloc(sizeof(struct JobSegment));
    if (!job)
        return NULL;
    job->bmp = bmp;
    job->seg_map = seg_map;
    job->threshold = threshold;
    job->step = step;
    return job;
}

struct JobCount *create_job_count(struct BMP *img,
                                  struct Pixel p,
                                  int step) {
    struct JobCount *job;
    job = malloc(sizeof(struct JobCount));
    if (!job)
        return NULL;
    job->img = img;
    job->p = p;
    job->count = 0;
    job->step = step;
    return job;
}
