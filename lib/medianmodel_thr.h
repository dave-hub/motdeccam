// ----------
// STRUCTURES
// ----------

struct JobBackgroundMM {
    struct MedianModel *model;
    struct BMP *bg;
    int step;
};

struct JobUpdateMM {
    struct MedianModel *model;
    struct BMP *seg_map;
    struct BMP *img;
    struct BMP *bg;
    struct BMP *new_img;
    int step;
};

//function declarations
struct BMP *generate_median_background_thr(struct MedianModel *model);

struct BMP *generate_median_seg_map_thr(struct MedianModel *model,
                                        struct BMP *img,
                                        unsigned char threshold);

void update_median_model_thr(struct MedianModel *model,
                             struct BMP *seg_map,
                             struct BMP *img);

//job declarations
void *do_job_background_mm(void *job_struct);


void *do_job_update_mm(void *job_struct);

struct JobBackgroundMM *create_job_background_mm(struct MedianModel *model,
                                                 struct BMP *bg,
                                                 int step);


struct JobUpdateMM *create_job_update_mm(struct MedianModel *model,
                                         struct BMP *seg_map,
                                         struct BMP *img,
                                         struct BMP *bg,
                                         struct BMP *new_img,
                                         int step);


struct JobBackgroundMM *create_job_background_mm(struct MedianModel *model,
                                                 struct BMP *bg,
                                                 int step);

// ---------
// FUNCTIONS
// ---------

//generates the median background image from the model
//runs 4 threads
struct BMP *generate_median_background_thr(struct MedianModel *model) {
    struct BMP *bg;

    bg = init_BMP(model->image_header->width, model->image_header->height);
    if (!bg)
        return NULL;
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobBackgroundMM *t1_job, *t2_job, *t3_job, *t4_job;
    
    //create jobs
    t1_job = create_job_background_mm(model, bg, 0);
    t2_job = create_job_background_mm(model, bg, 1);
    t3_job = create_job_background_mm(model, bg, 2);
    t4_job = create_job_background_mm(model, bg, 3);
    
    //create threads
    if (pthread_create(&t1, NULL, do_job_background_mm, t1_job) ||
        pthread_create(&t2, NULL, do_job_background_mm, t2_job) ||
        pthread_create(&t3, NULL, do_job_background_mm, t3_job) ||
        pthread_create(&t4, NULL, do_job_background_mm, t4_job)) {
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
    
    return bg;
}

//generates the segmentation map between the given image and model
//thresholding the difference image with the given value
//runs 4 threads
struct BMP *generate_median_seg_map_thr(struct MedianModel *model,
                                        struct BMP *img,
                                        unsigned char threshold) {
    struct BMP *bg, *diff, *seg_map;
    bg = generate_median_background_thr(model);
    diff = get_difference_thr(img, bg);
    greyscale_BMP_thr(diff);
    seg_map = segment_BMP_thr(diff, threshold);
    
    free_BMP(bg);
    free_BMP(diff);
    return seg_map;
}

//updates the model with the given seg_map and img
//runs 4 threads
void update_median_model_thr(struct MedianModel *model,
                             struct BMP *seg_map,
                             struct BMP *img) {
    struct BMP *new_img, *bg;
    struct CPQueue *tmp, *tl;
    int pd_size;
    
    pd_size = get_scanline_size(model->image_header->width) * model->image_header->height;
    
    new_img = clone_BMP(img);
    
    bg = generate_median_background_thr(model);
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobUpdateMM *t1_job, *t2_job, *t3_job, *t4_job;
    
    //create jobs
    t1_job = create_job_update_mm(model, seg_map, img, bg, new_img, 0);
    t2_job = create_job_update_mm(model, seg_map, img, bg, new_img, 1);
    t3_job = create_job_update_mm(model, seg_map, img, bg, new_img, 2);
    t4_job = create_job_update_mm(model, seg_map, img, bg, new_img, 3);
    
    //create threads
    if (pthread_create(&t1, NULL, do_job_update_mm, t1_job) ||
        pthread_create(&t2, NULL, do_job_update_mm, t2_job) ||
        pthread_create(&t3, NULL, do_job_update_mm, t3_job) ||
        pthread_create(&t4, NULL, do_job_update_mm, t4_job)) {
        return;
    }
    
    //wait for threads to join
    if (pthread_join(t1, NULL) ||
        pthread_join(t2, NULL) ||
        pthread_join(t3, NULL) ||
        pthread_join(t4, NULL)) {
        return;
    }
    
    //free job structs
    free(t1_job);
    free(t2_job);
    free(t3_job);
    free(t4_job);
    
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

//job functions
void *do_job_background_mm(void *job_struct) {
    struct MedianModel *model;
    struct BMP *bg;
    struct CPQueue *tmp;
    
    int step, i, j, k, pd_size;
    
    //unpack job struct
    struct JobBackgroundMM *job = (struct JobBackgroundMM *) job_struct;
    model = job->model;
    bg = job->bg;
    step = job->step;
    
    unsigned char vals[model->n];
    
    pd_size = get_scanline_size(model->image_header->width) * model->image_header->height;

    for (i = step; i < pd_size; i+=4) {
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
}

void *do_job_update_mm(void *job_struct) {
    struct MedianModel *model;
    struct BMP *seg_map, *img, *bg, *new_img;
    int i, step, pd_size;
    
    //unpack job_struct
    struct JobUpdateMM *job = (struct JobUpdateMM *) job_struct;
    model = job->model;
    seg_map = job->seg_map;
    img = job->img;
    bg = job->bg;
    new_img = job->new_img;
    step = job->step;
    
    pd_size = get_scanline_size(model->image_header->width) * model->image_header->height;
    
    //at each pixel where seg_map[i] == 255 (motion) replace with median
    //from the background
    for (i = step; i < pd_size; i += 4) {
        if (seg_map->pixel_data[i] == 255) {
            new_img->pixel_data[i] = bg->pixel_data[i];
        }
    }
}
    
struct JobBackgroundMM *create_job_background_mm(struct MedianModel *model,
                                                 struct BMP *bg,
                                                 int step) {
    struct JobBackgroundMM *job;
    job = malloc(sizeof(struct JobBackgroundMM));
    if (!job)
        return NULL;
    job->model = model;
    job->bg = bg;
    job->step = step;
    return job;
}


struct JobUpdateMM *create_job_update_mm(struct MedianModel *model,
                                         struct BMP *seg_map,
                                         struct BMP *img,
                                         struct BMP *bg,
                                         struct BMP *new_img,
                                         int step) {
    struct JobUpdateMM *job;
    job = malloc(sizeof(struct JobUpdateMM));
    if (!job)
        return NULL;
    job->model = model;
    job->seg_map = seg_map;
    job->img = img;
    job->bg = bg;
    job->new_img = new_img;
    job->step = step;
    return job;
}
