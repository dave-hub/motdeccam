#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define PI 3.14159265358979323846
#define NUM_THREADS 4

// ----------
// STRUCTURES
// ----------

struct JobUpdateGMM {
    struct GaussianModel *model;
    struct BMP *seg_map;
    struct BMP *img;
    int step;
};

struct JobBackgroundGMM {
    struct GaussianModel *model;
    struct BMP *background;
    int step;
};

struct JobSegmentGMM {
    struct GaussianModel *model;
    struct BMP *img;
    struct BMP *seg_map;
    int step;
};

struct JobNormalizeGMM {
    struct GaussianModel *model;
    int step;
};

// ------------
// DECLARATIONS
// ------------

//function declarations
int update_gaussian_model_thr(struct GaussianModel *model,
                              struct BMP *img,
                              struct BMP *seg_map);

struct BMP *generate_gaussian_background_thr(struct GaussianModel *model);

struct BMP *generate_gaussian_seg_map_thr(struct GaussianModel *model,
                                         struct BMP *img);

int normalize_priors_thr(struct GaussianModel *model);

int *get_sorted_indexes(double *initial_list, 
                        double *sorted_list, 
                        int n);

//job declarations

void *do_job_update_gmm(void *job_struct);

void *do_job_background_gmm(void *job_struct);

void *do_job_segment_gmm(void *job_struct);

void *do_job_normalize_gmm(void *job_struct);

struct JobUpdateGMM *create_job_update_gmm(struct GaussianModel *model,
                                           struct BMP *img,
                                           struct BMP *seg_map,
                                           int step);

struct JobBackgroundGMM *create_job_background_gmm(struct GaussianModel *model,
                                                   struct BMP *background,
                                                   int step);

struct JobSegmentGMM *create_job_segment_gmm(struct GaussianModel *model,
                                             struct BMP *img,
                                             struct BMP *seg_map,
                                             int step);

struct JobNormalizeGMM *create_job_normalize_gmm(struct GaussianModel *model,
                                                 int step);

// ---------
// FUNCTIONS
// ---------

//generates the segmentation map of the foreground of img using the background model
struct BMP *generate_gaussian_seg_map_thr(struct GaussianModel *model,
                                         struct BMP *img) {
    struct BMP *seg_map;
    
    seg_map = init_BMP(model->width, model->height);
    if (!seg_map)
        return NULL;
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobSegmentGMM *t1_job, *t2_job, *t3_job, *t4_job;
        
    //create jobs
    t1_job = create_job_segment_gmm(model, img, seg_map, 0);
    t2_job = create_job_segment_gmm(model, img, seg_map, 1);
    t3_job = create_job_segment_gmm(model, img, seg_map, 2);
    t4_job = create_job_segment_gmm(model, img, seg_map, 3);
            
    //create threads
    if (pthread_create(&t1, NULL, do_job_segment_gmm, t1_job) ||
        pthread_create(&t2, NULL, do_job_segment_gmm, t2_job) ||
        pthread_create(&t3, NULL, do_job_segment_gmm, t3_job) ||
        pthread_create(&t4, NULL, do_job_segment_gmm, t4_job)) {
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

//updates the given model based on the given image and it's segmentation map
//will return 0 if errors
int update_gaussian_model_thr(struct GaussianModel *model,
                              struct BMP *img,
                              struct BMP *seg_map) {
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobUpdateGMM *t1_job, *t2_job, *t3_job, *t4_job;

    //create jobs
    t1_job = create_job_update_gmm(model, img, seg_map, 0);
    t2_job = create_job_update_gmm(model, img, seg_map, 1);
    t3_job = create_job_update_gmm(model, img, seg_map, 2);
    t4_job = create_job_update_gmm(model, img, seg_map, 3);
        
    //create threads
    if (pthread_create(&t1, NULL, do_job_update_gmm, t1_job) ||
        pthread_create(&t2, NULL, do_job_update_gmm, t2_job) ||
        pthread_create(&t3, NULL, do_job_update_gmm, t3_job) ||
        pthread_create(&t4, NULL, do_job_update_gmm, t4_job)) {
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

//generates the most likely background image based on the model
struct BMP *generate_gaussian_background_thr(struct GaussianModel *model) {
    struct BMP *bg;

    bg = init_BMP(model->width, model->height);
    if (!bg)
        return NULL;
    
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobBackgroundGMM *t1_job, *t2_job, *t3_job, *t4_job;
    
    //create jobs
    t1_job = create_job_background_gmm(model, bg, 0);
    t2_job = create_job_background_gmm(model, bg, 1);
    t3_job = create_job_background_gmm(model, bg, 2);
    t4_job = create_job_background_gmm(model, bg, 3);
    
    //create threads
    if (pthread_create(&t1, NULL, do_job_background_gmm, t1_job) ||
        pthread_create(&t2, NULL, do_job_background_gmm, t2_job) ||
        pthread_create(&t3, NULL, do_job_background_gmm, t3_job) ||
        pthread_create(&t4, NULL, do_job_background_gmm, t4_job)) {
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

//normalizes all priors within the model
//returns 0 for errors
int normalize_priors_thr(struct GaussianModel *model) {
    //declare threads and jobs
    pthread_t t1, t2, t3, t4;
    struct JobNormalizeGMM *t1_job, *t2_job, *t3_job, *t4_job;

    //create jobs
    t1_job = create_job_normalize_gmm(model, 0);
    t2_job = create_job_normalize_gmm(model, 1);
    t3_job = create_job_normalize_gmm(model, 2);
    t4_job = create_job_normalize_gmm(model, 3);
        
    //create threads
    if (pthread_create(&t1, NULL, do_job_normalize_gmm, t1_job) ||
        pthread_create(&t2, NULL, do_job_normalize_gmm, t2_job) ||
        pthread_create(&t3, NULL, do_job_normalize_gmm, t3_job) ||
        pthread_create(&t4, NULL, do_job_normalize_gmm, t4_job)) {
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

//returns a malloc'd list of indexes from the initial list in their order
//within the sorted list.
//differs from original as it takes a pointer to resulting array as parameter
//instead of returning a pointer, allows use of stack memory
void get_sorted_indexes_st(int *sorted_indexes, double *initial_list, double *sorted_list, int n) {
    int i;
    
    for (i = 0; i <  n; i++) {
        sorted_indexes[i] = index_of(sorted_list[i], initial_list, n);
    }
}

//job functions

void *do_job_update_gmm(void *job_struct) {
    struct GaussianModel *model;
    struct GaussianMixture *gm;
    struct GaussianPixel *gp;
    struct BMP *seg_map;
    struct BMP *img;
    struct Pixel p, cp;
    int step;
    int x, y, k, worst;
    double meanr, meang, meanb, valr, valg, valb, avg_val, avg_mean;
    double var;
    double prior;
    int matched;
    double wsum;
    
    //get job struct
    struct JobUpdateGMM *job = (struct JobUpdateGMM *) job_struct;
    model = job->model;
    seg_map = job->seg_map;
    img = job->img;
    step = job->step;
    
    matched = 0;
    
    //ratings
    double ratings[model->k];
    
    //init ratings to 0
    for (k = 0; k < model->k; k++) {
        ratings[k] = 0.0;
    }
    for (y = 0; y < model->height; y++) {
        for (x = step; x < model->width; x+=NUM_THREADS) {
            //get gaussian mixture for this coordinate
            gm = model->map[(y*model->width)+x];
            //get segmap pixel at this coordinate
            p = get_pixel(seg_map, x, y);
            //check if pixel is foreground classified
            if (p.red == 255 && p.green == 255 && p.blue == 255) {
                //gather 'ratings' (prior/variance) of pixels at this coordinate
                for (k = 0; k < model->k; k++) {
                    ratings[k] = (gm->mixture[k]->prior / gm->mixture[k]->variance);
                }
                //get the index of the worst rated pixel
                worst = index_of_min(ratings, model->k);
                gp = gm->mixture[worst]; //store pointer to the one to be deleted
                //get pixel from change image
                cp = get_pixel(img, x, y);
                //replace worst rated pixel with newly observed pixel.
                gp->meanr = cp.red;
                gp->meang = cp.green;
                gp->meanb = cp.blue;
                gp->variance = model->new_dist_variance; //initially high variance
                gp->prior = 0.5/model->k; //initially low prior
                gm->mixture[worst] = gp;
            } else {
                //otherwise pixel is background
                p = get_pixel(img, x, y);
                valr = p.red;
                valg = p.green;
                valb = p.blue;
                //for each gaussian in the mixture
                for (k = 0; k < model->k; k++) {
                    gp = gm->mixture[k];
                    //if this is the matched distribution, update all
                    if (!matched && matches_distribution(p, *gp)) {
                        matched = 1;
                        meanr = gp->meanr;
                        meang = gp->meang;
                        meanb = gp->meanb;
                        var = gp->variance;
                        avg_val = (valr + valg + valb) / 3;
                        avg_mean = (meanr + meang + meanb) / 3;
                        gp->meanr = new_mean(meanr, valr, var, model->alpha, model->t);
                        gp->meanb = new_mean(meanb, valb, var, model->alpha, model->t);
                        gp->meang = new_mean(meang, valg, var, model->alpha, model->t);
                        //new variance, enforcing min_variance value
                        gp->variance = new_variance(avg_mean, avg_val, var, model->alpha, model->t);
                        gp->prior = new_prior(gp->prior, model->alpha, 1);
                    //otherwise just update prior
                    } else {
                        gp->prior = new_prior(gp->prior, model->alpha, 0);
                    }
                }
                matched = 0;
            }
        }
    }
    return NULL;
}

void *do_job_background_gmm(void *job_struct) {
    struct BMP *bg;
    struct GaussianModel *model;
    struct GaussianMixture *tmp;
    struct GaussianPixel *pixel;
    struct Pixel newp;
    int step;
    int x, y, i;
    
    //get job struct
    struct JobBackgroundGMM *job = (struct JobBackgroundGMM *) job_struct;
    model = job->model;
    bg = job->background;
    step = job->step;
    
    double ratings[model->k];

    for (y = 0; y < model->height; y++) {
        for (x = step; x < model->width; x+=NUM_THREADS) {
            tmp = model->map[(y*model->width)+x];
            //store ratings
            for (i = 0; i < model->k; i++) {
                ratings[i] = (tmp->mixture[i]->prior / tmp->mixture[i]->variance);
            }
            //get best rated pixel, and set value in bg.
            pixel = tmp->mixture[index_of_max(ratings, model->k)];
                        
            //ensure mean values are in correct range
            newp = make_pixel(pixel->meanr, pixel->meang, pixel->meanb);
            //check red
            if (pixel->meanr > 255.0) {
                newp.red = 255;
            } else if (pixel->meanr < 0.0) {
                newp.red = 0;
            }
            //check green
            if (pixel->meang > 255.0) {
                newp.green = 255;
            } else if (pixel->meang < 0.0) {
                newp.green = 0;
            }
            //check blue
            if (pixel->meanb > 255.0) {
                newp.blue = 255;
            } else if (pixel->meanb < 0.0) {
                newp.blue = 0;
            }
            if (!set_pixel(bg, x, y, newp)) {
                return NULL;
            }
        }
    }
    return NULL;
}

void *do_job_segment_gmm(void *job_struct) {
    struct GaussianModel *model;
    struct GaussianMixture *gm;
    struct GaussianPixel *gp;
    struct BMP *seg_map;
    struct BMP *img;
    struct Pixel p;
    int step;
    int x, y, k;
    int is_bg;
    double wsum;
    
    //get job struct
    struct JobSegmentGMM *job = (struct JobSegmentGMM *) job_struct;
    model = job->model;
    img = job->img;
    seg_map = job->seg_map;
    step = job->step;
    
    //arrays in thread stack memory
    double priors[model->k];
    double sorted_priors[model->k];
    int sorted_indexes[model->k];
    
    for (y = 0; y < model->height; y++) {
        for (x = step; x < model->width; x+=NUM_THREADS) {
            gm = model->map[(y * model->width) + x];
            is_bg = 0;
            wsum = 0;
                    
            //gather priors
            for (k = 0; k < model->k; k++) {
                sorted_priors[k] = priors[k] = gm->mixture[k]->prior;
            }

            //sort priors
            qsort(sorted_priors, model->k, sizeof(double), deccmp);

            //get sorted index list (mallocs)
            get_sorted_indexes_st(sorted_indexes, priors, sorted_priors, model->k);

            //get the pixel at the location
            p = get_pixel(img, x, y);
                    
            for (k = 0; k < model->k; k++) {
                //check we are not yet > T
                if (wsum > model->t) {
                    break;
                }
                        
                gp = gm->mixture[sorted_indexes[k]];
                wsum += gp->prior;
                        
                //check if pixel in img matches the kth distribution
                if (matches_distribution(p, *gp)) {
                    set_pixel(seg_map, x, y, make_pixel(0, 0, 0));
                    is_bg = 1;
                    break;
                }
            }
                    
            //if no distribution matched or > T, mark as foreground
            if (!is_bg) {
                set_pixel(seg_map, x, y, make_pixel(255, 255, 255));
            }
        }
    }
    return NULL;
}

void *do_job_normalize_gmm(void *job_struct) {
    struct GaussianModel *model;
    struct GaussianMixture *gm;
    int step;
    int x, y, k;
    double sum;
    
    //get job struct
    struct JobNormalizeGMM *job = (struct JobNormalizeGMM *) job_struct;
    model = job->model;
    step = job->step;
    
    for (y = 0; y < model->height; y++) {
        for (x = step; x < model->width; x+=NUM_THREADS) {
            //get mixture of current coordinate
            gm = model->map[(y*model->width)+x];
            sum = 0;
                        
            //sum priors of the mixture
            for (k = 0; k < model->k; k++) {
                sum += gm->mixture[k]->prior;                
            }
                        
            //normalize each prior
            for (k = 0; k < model->k; k++) {
                gm->mixture[k]->prior /= sum;
            }
        }
    }
    return NULL;
}

struct JobUpdateGMM *create_job_update_gmm(struct GaussianModel *model,
                                           struct BMP *img,
                                           struct BMP *seg_map,
                                           int step) {
    struct JobUpdateGMM *job;
    job = malloc(sizeof(struct JobUpdateGMM));
    if (!job)
        return NULL;
    job->model = model;
    job->seg_map = seg_map;
    job->img = img;
    job->step = step;
    return job;
}

struct JobBackgroundGMM *create_job_background_gmm(struct GaussianModel *model,
                                                   struct BMP *background,
                                                   int step) {
    struct JobBackgroundGMM *job;
    job = malloc(sizeof(struct JobBackgroundGMM));
    if (!job)
        return NULL;
    job->model = model;
    job->background = background;
    job->step = step;
    return job;
}

struct JobSegmentGMM *create_job_segment_gmm(struct GaussianModel *model,
                                             struct BMP *img,
                                             struct BMP *seg_map,
                                             int step) {
    struct JobSegmentGMM *job;
    job = malloc(sizeof(struct JobSegmentGMM));
    if (!job)
        return NULL;
    job->model = model;
    job->img = img;
    job->seg_map = seg_map;
    job->step = step;
    return job;
}

struct JobNormalizeGMM *create_job_normalize_gmm(struct GaussianModel *model,
                                                 int step) {
    struct JobNormalizeGMM *job;
    job = malloc(sizeof(struct JobNormalizeGMM));
    if (!job)
        return NULL;
    job->model = model;
    job->step = step;
    return job;
}
