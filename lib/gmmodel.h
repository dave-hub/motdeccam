#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265358979323846

// ----------
// STRUCTURES
// ----------

struct GaussianPixel {
    double meanr;
    double meang;
    double meanb;
    double variance;
    double prior;
};

struct GaussianMixture {
    struct GaussianPixel **mixture; //k pixels in the mixture
};

struct GaussianModel {
    int width;
    int height;
    int k; //number of guassians in the mixture
    double t; //portion of data for background, max sum of weight of pixels in bg
    double alpha;
    double min_variance;
    double new_dist_variance; //variance of new distributions added to mixture 1.5*init_var
    struct GaussianMixture **map;
};

//function declarations
struct GaussianModel *init_gaussian_model(struct BMP *img,
                                          int k,
                                          double t,
                                          double alpha,
                                          double initial_variance,
                                          double min_variance);

int update_gaussian_model(struct GaussianModel *model,
                           struct BMP *seg_map,
                           struct BMP *img);

struct BMP *generate_gaussian_background(struct GaussianModel *model);

struct BMP *generate_gaussian_seg_map(struct GaussianModel *model,
                             struct BMP *img);

void free_gaussian_model(struct GaussianModel *model);

int normalise_priors(struct GaussianModel *model);

void print_mixture(struct GaussianModel *model,
                   unsigned int x,
                   unsigned int y);

int matches_distribution(struct Pixel p,
                         struct GaussianPixel d);

double new_prior(double old_prior,
                 double alpha,
                 int matched);

double new_mean(double mean,
                double val,
                double var,
                double alpha,
                double t);

double new_variance(double mean,
                    double val,
                    double var,
                    double alpha,
                    double t);

double pdf(double val,
           double mean,
           double var,
           double t);

int index_of_max(double *ratings,
                 int k);

int index_of_min(double *ratings,
                 int k);

double powt(double x,
            double t);

int *get_sorted_indexes(double *initial_list,
                        double *sorted_list,
                        int n);

int index_of(double x,
             double *list,
             int n);

int deccmp(const void *a,
           const void *b);

// ---------
// FUNCTIONS
// ---------

//initializes a GaussianModel using the given image and values
struct GaussianModel *init_gaussian_model(struct BMP *img,
                                          int k,
                                          double t,
                                          double alpha,
                                          double initial_variance,
                                          double min_variance) {
    struct GaussianModel *model;
    struct GaussianMixture *tmpm;
    struct GaussianPixel *tmpp;
    struct Pixel bg_pixel;
    int x, y, i;

    model = malloc(sizeof(struct GaussianModel));
    if (!model)
        return NULL;

    //init model
    model->width = img->image_header->width;
    model->height = img->image_header->height;
    model->k = k;
    model->t = t;
    model->alpha = alpha;
    model->min_variance = min_variance;
    model->new_dist_variance = 1.5*initial_variance;

    //init map
    model->map = malloc(model->width * model->height * sizeof(struct GaussianMixture *));
    if(!model->map)
        return NULL;
    
    //for each point in the map, initialize the mixture and pixels
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
            //get pixel from init image
            bg_pixel = get_pixel(img, x, y);
            //init space for pointers to k mixtures
            model->map[(y*model->width)+x] = malloc(k * sizeof(struct GaussianMixture *));
            if (!model->map[(y*model->width)+x])
                return NULL;
            //malloc mixture
            model->map[(y*model->width)+x]->mixture = malloc(k * sizeof(struct GaussianPixel *));
            //for each distribution in the mixture, malloc for the pixel struct
            for (i = 0; i < k; i++) {
                model->map[(y*model->width)+x]->mixture[i] = malloc(sizeof(struct GaussianPixel));
                if (!model->map[(y*model->width)+x]->mixture[i])
                    return NULL;
                tmpp = model->map[(y*model->width)+x]->mixture[i];
                tmpp->meanr = bg_pixel.red;
                tmpp->meang = bg_pixel.green;
                tmpp->meanb = bg_pixel.blue;
                tmpp->variance = initial_variance;
                tmpp->prior = (1.0 / model->k);
            }
        }
    }
    return model;
}

//generates the segmentation map of the foreground of img using the background model
struct BMP *generate_gaussian_seg_map(struct GaussianModel *model,
                                      struct BMP *img) {
    struct BMP *seg_map;
    struct GaussianMixture *gm;
    struct GaussianPixel *gp; 
    struct Pixel p;
    int x, y, k, is_bg;
    double wsum;
    double *priors, *sorted_priors;
    int *sorted_indexes;
    
    priors = malloc(model->k * sizeof(double));
    sorted_priors = malloc(model->k * sizeof(double));
    
    if (!priors || !sorted_priors)
        return NULL;
    
    //used for limiting the number of distributions considered the background
    wsum = 0;  //sum of weights, the T from the Stauffer and Grimson (1999)
    is_bg = 0; 
    
    //init seg map
    seg_map = init_BMP(model->width, model->height);
    
    //for each point in the map
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
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
            sorted_indexes = get_sorted_indexes(priors, sorted_priors, model->k);

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
            
            //free sorted index list
            free(sorted_indexes);
            sorted_indexes = NULL;
        }
    }
    
    free(priors);
    free(sorted_priors);
    
    return seg_map;
}

//updates the given model based on the given image and it's segmentation map
//will return 0 if errors
int update_gaussian_model(struct GaussianModel *model,
                          struct BMP *seg_map,
                          struct BMP *img) {
    struct GaussianMixture *gm;
    struct GaussianPixel *gp;
    struct Pixel p, cp;
    int x, y, k, worst, matched;
    double meanr, meang, meanb, valr, valg, valb, avg_val, avg_mean;
    double var;
    double prior;
    double *ratings;
    
    matched = 0;
    
    //allocate memory for ratings
    ratings = malloc(model->k * sizeof(double));
    if (!ratings)
        return 0;
    
    //init ratings to 0
    for (k = 0; k < model->k; k++) {
        ratings[k] = 0.0;
    }
    
    //for each place in map
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
            //get gaussian mixture for this coordinate
            gm = model->map[(y*model->width)+x];
            //get segmap pixel at this coordinate
            p = get_pixel(seg_map, x, y);
            //check if pixel is foreground classified
            if (is_foreground(p)) {
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
    free(ratings);
    return 1;
}

//generates the most likely background image based on the model
struct BMP *generate_gaussian_background(struct GaussianModel *model) {
    struct GaussianMixture *tmp;
    struct GaussianPixel *pixel;
    struct Pixel newp;
    struct BMP *bg;
    int x, y, i;
    double ratings[model->k];
    
    bg = init_BMP(model->width, model->height);
    if (!bg)
        return NULL;
    
    //for each mixture within the map, set the pixel value at the same
    //location in the bg to the most likely gaussian by prior/variance
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
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
    
    return bg;
}

//frees the given gaussian model
void free_gaussian_model(struct GaussianModel *model) {
    int x, y, i;
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
            for (i = 0; i < model->k; i++) {
                free(model->map[(y*model->width)+x]->mixture[i]);
            }
            free(model->map[(y*model->width)+x]->mixture);
            free(model->map[(y*model->width)+x]);
        }
    }
    free(model->map);
    free(model);
}

//normalises all priors within the model
//returns 0 for errors
int normalise_priors(struct GaussianModel *model) {
    struct GaussianMixture *gm;
    struct GaussianPixel *gp;
    int x, y, k;
    double sum;
    
    //for each coordinate in map
    for (x = 0; x < model->width; x++) {
        for (y = 0; y < model->height; y++) {
            //get mixture of current coordinate
            gm = model->map[(y*model->width)+x];
            sum = 0;
            
            //sum priors of the mixture
            for (k = 0; k < model->k; k++) {
                sum += gm->mixture[k]->prior;                
            }
            
            //normalise each prior
            for (k = 0; k < model->k; k++) {
                gm->mixture[k]->prior /= sum;
            }
        }
    }
    return 1;
}

//prints each gaussian pixel in the mixture at the given coordinates
void print_mixture(struct GaussianModel *model,
                   unsigned int x,
                   unsigned int y) {
    struct GaussianMixture *gm;
    struct GaussianPixel *gp;
    int k;
    
    gm = model->map[(y*model->width)+x];
    for (k = 0; k < 30; k++) {
        printf("-");
    }
    printf("\n");
    printf("Mixture at (%d, %d)\n", x, y);
    for (k = 0; k < model->k; k++) {
        gp = gm->mixture[k];
        printf("\n--%d--\n", k);
        printf("Mean:     (%f, %f, %f)\n", gp->meanr, gp->meang, gp->meanb);
        printf("Variance: %f\n", gp->variance);
        printf("Prior:    %f\n", gp->prior);
    }
}

//checks whether a given pixel is "matched" by a given distribution (means within 2.5 s.d.)
int matches_distribution(struct Pixel p,
                         struct GaussianPixel d) {
    int minr, maxr, ming, maxg, minb, maxb;
    double v;
    v = d.variance;
    //r
    if (d.meanr < v) {
        minr = 0;
    } else {
        minr = d.meanr - v;
    }
    if (255 - d.meanr < v) {
        maxr = 255;
    } else {
        maxr = d.meanr + v;
    }
    //g
    if (d.meang < v) {
        ming = 0;
    } else {
        ming = d.meang - v;
    }
    if (255 - d.meang < v) {
        maxg = 255;
    } else {
        maxg = d.meang + v;
    }
    //b
    if (d.meanb < v) {
        minb = 0;
    } else {
        minb = d.meanb - v;
    }
    if (255 - d.meanb < v) {
        maxb = 255;
    } else {
        maxb = d.meanb + v;
    }
    return ((d.meanr - (2.5 * d.variance)) < p.red &&
            p.red < (d.meanr + (2.5 * d.variance)) &&
            (d.meang - (2.5 * d.variance)) < p.green &&
            p.green < (d.meang + (2.5 * d.variance)) &&
            (d.meanb - (2.5 * d.variance)) < p.blue &&
            p.blue < (d.meanb + (2.5 * d.variance)));
}

//returns the index of the maximum value of the k elements of ratings
int index_of_max(double *ratings,
                 int k) {
    int index, i;
    double *tmp, max;
    
    i = index = 0;
    max = ratings[i++]; //store first element as max
    tmp = &ratings[i];    //pointer to second element
    
    while (tmp && i < k) {
        if (ratings[i] > max) {
            max = ratings[i];
            index = i;
        }
        tmp = &tmp[++i];
    }
    
    return index;
}

//returns the index of the minimum value of the k elements of ratings
int index_of_min(double *ratings,
                 int k) {
    int index, i;
    double *tmp, min;
    
    i = index = 0;
    min = ratings[i++]; //store first element as min
    tmp = &ratings[i];    //pointer to second element
    
    while (tmp && i < k) {
        if (ratings[i] <= min) {
            min = ratings[i];
            index = i;
        }
        tmp = &tmp[++i];
    }
    
    return index;
}

//returns the updated prior weight value based on the given values
double new_prior(double old_prior,
                 double alpha,
                 int matched) {
    return (1 - alpha) * old_prior + (alpha * matched);
}

//returns the updated mean using the given values
double new_mean(double mean,
                double val,
                double var,
                double alpha,
                double t) {
    double p = alpha * pdf(mean, val, var, t);
    return ((1 - p) * mean) + (p * val);
}

//returns the updated variance using the given values
double new_variance(double mean,
                    double val,
                    double var,
                    double alpha,
                    double t) {
    double p = alpha * pdf(mean, val, var, t);
    return ((1 - p) * var) + (p * powt(val - mean, t) * (val - mean));
}

//the probability density function as defined in the paper
double pdf(double mean,
           double val,
           double var,
           double t) {
    double coeff = 1/(sqrt(fabs(var)) * sqrt(2*PI));
    double power = -0.5 * powt((powt(val - mean, t)*(val - mean))/sqrt(fabs(var)), 2);
    double pdf = coeff * exp(power);
    return pdf;
}

double powt(double x,
            double t) {
    if (x < 0)
        return -pow(-x, t);
    else
        return pow(x, t);
}

//returns a malloc'd list of indexes from the initial list in their order
//within the sorted list.
int *get_sorted_indexes(double *initial_list, double *sorted_list, int n) {
    int i, *ret;
    
    ret = malloc(n * sizeof(int));
    if (!ret)
        return 0;
    
    for (i = 0; i <  n; i++) {
        ret[i] = index_of(sorted_list[i], initial_list, n);
    }
    
    return ret;
}

//returns index of x in list of size n. -1 if doesnt exist
int index_of(double x, double *list, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (list[i] == x)
            return i;
    }
    return -1;
}

//qsort cmp for decreasing sort
int deccmp(const void *a,
           const void *b) {
    double x = *(double *)a, y = *(double *)b;
    if (x > y)
        return -1;
    if (x < y)
        return 1;
    return 0;
}
