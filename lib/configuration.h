// Personal Project 2018 - David Jones - University of Birmingham
// "Developing Video Motion Detection Methods for a Security Camera System"
//
// System Configuration
//
// Allows changing, saving, and loading of various system settings

//--------
//includes
//--------

#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<linux/limits.h>
#include<dirent.h>
#include<errno.h>

//------------------
//struct definitions
//------------------

struct SysConfig {
    double change_percent_threshold; //CPT value (0.0 - 1.0)
    int pixel_change_threshold;      //PCT value (0 - 255)
    int raw_img_output;              //base and change image output (0 - 1)
    int diff_img_output;             //difference image output (0 - 1)
    int segmap_img_output;           //segmentation map image output (0 - 1)
    int median_img_count;            //The number of images for a median model
    char *logs_path;            //The path for where images are stored
    char *logfile_path;         //The path for the log text file
    char *video_device;         //The path to the video device to capture from
    char *ffmpeg_path;          //path to ffmpeg executable
    char *resolution;           //resolution as string widthxheight
    int gmm_k_val;          //num pixels in mixture at each point in map
    double gmm_t_val;       //proportion of pixel data that is background
    double gmm_alpha;       //learning rate
    double gmm_init_var;    //initial variance of pixel distributions
    double gmm_min_var;     //minimum variance of pixel distributions
    int do_ent_filtering;   //enable entity filtering (0 - 1)
    int ent_min_mass;   //minimum mass of entities in segmap not filtered (-1 < )
    int ent_max_mass;   //maximum mass of entities in segmap not filtered (-1 < )
    int ent_min_width;  //minimum width of entities in segmap not filtered (-1 < )
    int ent_max_width;  //maximum width of entities in segmap not filtered (-1 < )
    int ent_min_height; //minimum height of entities in segmap not filtered (-1 < )
    int ent_max_height; //maximum height of entities in segmap not filtered (-1 < )
};

//---------------------
//function declarations
//---------------------

void print_config(struct SysConfig *config);

void output_config(struct SysConfig *config,
                   FILE *output);

int init_config(struct SysConfig *config,
                double cpt,
                int pct,
                int rio,
                int dio,
                int sio,
                int mic,
                char *logs,
                char *logf,
                char *vdev,
                char *ffmp,
                char *res,
                int gmm_k_val,
                double gmm_t_val,
                double gmm_alpha,
                double gmm_init_var,
                double gmm_min_var,
                int do_ent_filtering,
                int ent_min_mass,
                int ent_max_mass,
                int ent_min_width,
                int ent_max_width,
                int ent_min_height,
                int ent_max_height);

int set(struct SysConfig *config,
        char *name,
        char *value);

int load_config(struct SysConfig *config,
                char *path);

int save_config(struct SysConfig *config,
                char *path);

int is_uns_char(char *value);

int is_bool(char *value);

int is_double(char *value);

double str_to_double(char *value_str);

unsigned char str_to_uns_char(char *value_str);

int str_to_filter_val(char *str);

int is_valid_dir(char *dpath);

int is_valid_file(char *fpath);

int is_valid_filter_val(char *str);

int is_uns_int(char *value);

int replace_newline(char *str, char c);

//-------------------------
//main function definitions
//-------------------------

//sets the variable by the given 'name' to the given 'value'
//returns 0 on success
//error codes:
//  1 - unknown variable name
//  2 - change_percent_threshold values must be 0.0 - 1.0
//  3 - pixel_change_threshold values must 0 - 255
//  4 - raw_img_output values must be 0 or 1
//  5 - diff_img_output values must be 0 or 1
//  6 - segmap_img_output value must be 0 or 1
//  7 - median_img_count value must be 1 - 255
//  8 - logs_path must be a valid path
//  9 - logfile_path must be a valid path
// 10 - video_device must be a valid path
// 11 - ffmpeg_path must be a valid path
// 12 - resolution must be in the format (width)x(height)
// 13 - gmm_k_val must be 1 - 10
// 14 - gmm_t_val must be 0.0 - 1.0
// 15 - gmm_alpha must be 0.0 - 1.0
// 16 - gmm_init_var must be 0 - 255
// 17 - gmm_min_var must be 0 - 255
// 18 - do_ent_filtering 0 - 1
// 19 - ent_min_mass must be >= -1
// 21 - ent_max_mass must be >= -1
// 21 - ent_min_width must be >= -1
// 22 - ent_max_width must be >= -1
// 23 - ent_min_height must be >= -1
// 24 - ent_max_height must be >= -1
int set(struct SysConfig *config,
        char *name,
        char *value) {
    char *c;
    //change_percent_threshold
    if ((c = strstr(name, "change_percent_threshold")) != NULL
        || (c = strstr(name, "cpt")) != NULL) {
        if (is_double(value)) {
            config->change_percent_threshold = str_to_double(value);
        } else {
            return 2;
        }
    //pixel_change_threshold
    } else if ((c = strstr(name, "pixel_change_threshold")) != NULL
        || (c = strstr(name, "pct")) != NULL) {
        if (is_uns_char(value)) {
            config->pixel_change_threshold = str_to_uns_char(value);
        } else {
            return 3;
        }
    //raw_img_output
    } else if ((c = strstr(name, "raw_img_output")) != NULL
        || (c = strstr(name, "rio")) != NULL) {
        if (is_bool(value)) {
            config->raw_img_output = (value[0]-'0');
        } else {
            return 4;
        }
    //diff_img_output
    } else if ((c = strstr(name, "diff_img_output")) != NULL
        || (c = strstr(name, "dio")) != NULL) {
        if (is_bool(value)) {
            config->diff_img_output = (value[0]-'0');
        } else {
            return 5;
        }
    //mot_img_output
    } else if ((c = strstr(name, "segmap_img_output")) != NULL
        || (c = strstr(name, "sio")) != NULL) {
        if (is_bool(value)) {
            config->segmap_img_output = (value[0]-'0');
        } else {
            return 6;
        }
    //median_img_count
    } else if ((c = strstr(name, "median_img_count")) != NULL
        || (c = strstr(name, "mic")) != NULL) {
        if (is_uns_char(value)) {
            unsigned char v = str_to_uns_char(value);
            if (v != 0) {
                config->median_img_count = v;
            } else {
                return 7;
            }
        } else {
            return 7;
        }
    //logs_path
    } else if ((c = strstr(name, "logs_path")) != NULL
        || (c = strstr(name, "logs")) != NULL) {
        //remove trailing newline
        int len = strlen(value);
        if (len > 0 && value[len-1] == '\n') {
            value[len-1] = '\0';
        }
        //check if given string is a valid file path
        if (is_valid_dir(value)) {
            //free existing path
            if (config->logs_path != NULL) {
                free(config->logs_path);
                config->logs_path = NULL;
            }
            //malloc for copy
            config->logs_path = malloc(len+1);
            if (!config->logs_path) {
                printf("Error: Memory Error.");
            }
            //copy
            strcpy(config->logs_path, value);
        } else {
            return 8;
        }
    //logfile_path
    } else if ((c = strstr(name, "logfile_path")) != NULL
        || (c = strstr(name, "logf")) != NULL) {
        //remove trailing newline
        int len = strlen(value);
        if (len > 0 && value[len-1] == '\n') {
            value[len-1] = '\0';
        }
        //check if given string is a valid file path
        if (is_valid_file(value)) {
            //free existing path
            if (config->logfile_path != NULL) {
                free(config->logfile_path);
                config->logfile_path = NULL;
            }
            //malloc for copy
            config->logfile_path = malloc(len+1);
            if (!config->logfile_path) {
                printf("Error: Memory Error.");
            }
            //copy
            strcpy(config->logfile_path, value);
        } else {
            return 9;
        }
    //video_device
    } else if ((c = strstr(name, "video_device")) != NULL
        || (c = strstr(name, "vdev")) != NULL) {
        //remove trailing newline
        int len = strlen(value);
        if (len > 0 && value[len-1] == '\n') {
            value[len-1] = '\0';
        }
        //TODO some sort of device verification
        if (1) {
            //free existing path
            if (config->video_device != NULL) {
                free(config->video_device);
                config->video_device = NULL;
            }
            //malloc for copy
            config->video_device = malloc(len+1);
            if (!config->video_device) {
                printf("Error: Memory Error.");
            }
            //copy
            strcpy(config->video_device, value);
        } else {
            return 10;
        }
    //ffmpeg_path
    } else if ((c = strstr(name, "ffmpeg_path")) != NULL
        || (c = strstr(name, "ffmp")) != NULL) {
        //remove trailing newline
        int len = strlen(value);
        if (len > 0 && value[len-1] == '\n') {
            value[len-1] = '\0';
        }
        //TODO some sort of path verification
        if (1) {
            //free existing path
            if (config->ffmpeg_path != NULL) {
                free(config->ffmpeg_path);
                config->ffmpeg_path = NULL;
            }
            //malloc for copy
            config->ffmpeg_path = malloc(len+1);
            if (!config->ffmpeg_path) {
                printf("Error: Memory Error.");
            }
            //copy
            strcpy(config->ffmpeg_path, value);
        } else {
            return 11;
        }
    //resolution
    } else if ((c = strstr(name, "resolution")) != NULL
        || (c = strstr(name, "res")) != NULL) {
        //remove trailing newline
        int len = strlen(value);
        if (len > 0 && value[len-1] == '\n') {
            value[len-1] = '\0';
        }
        //TODO some sort of resolution verification
        if (1) {
            //free existing path
            if (config->resolution != NULL) {
                free(config->resolution);
                config->resolution = NULL;
            }
            //malloc for copy
            config->resolution = malloc(len+1);
            if (!config->resolution) {
                printf("Error: Memory Error.");
            }
            //copy
            strcpy(config->resolution, value);
        } else {
            return 12;
        }
    //gmm_k_val
    } else if ((c = strstr(name, "gmm_k_val")) != NULL
        || (c = strstr(name, "gmk")) != NULL) {
        if (is_uns_char(value)) {
            unsigned char v = str_to_uns_char(value);
            if (v >= 1 && v <= 10) {
                config->gmm_k_val = v;
            } else {
                return 13;
            }
        } else {
            return 13;
        }
    //gmm_t_val
    } else if ((c = strstr(name, "gmm_t_val")) != NULL
        || (c = strstr(name, "gmt")) != NULL) {
        if (is_double(value)) {
            double v = str_to_double(value);
            if (v != 0) {
                config->gmm_t_val = v;
            } else {
                return 14;
            }
        } else {
            return 14;
        }
    //gmm_alpha
    } else if ((c = strstr(name, "gmm_alpha")) != NULL
        || (c = strstr(name, "gma")) != NULL) {
        if (is_double(value)) {
            double v = str_to_double(value);
            if (v != 0) {
                config->gmm_alpha = v;
            } else {
                return 15;
            }
        } else {
            return 15;
        }
    //gmm_init_var
    } else if ((c = strstr(name, "gmm_init_var")) != NULL
        || (c = strstr(name, "giv")) != NULL) {
        if (is_uns_char(value)) {
            unsigned char v = str_to_uns_char(value);
            if (v != 0) {
                config->gmm_init_var = (double) v;
            } else {
                return 16;
            }
        } else {
            return 16;
        }
    //gmm_min_var
    } else if ((c = strstr(name, "gmm_min_var")) != NULL
        || (c = strstr(name, "gmv")) != NULL) {
        if (is_uns_char(value)) {
            unsigned char v = str_to_uns_char(value);
            if (v != 0) {
                config->gmm_min_var = (double) v;
            } else {
                return 17;
            }
        } else {
            return 17;
        }
    //do_ent_filtering
    } else if ((c = strstr(name, "do_ent_filtering")) != NULL
        || (c = strstr(name, "eflt")) != NULL) {
        if (is_bool(value)) {
            int v = value[0] - '0';
            if (v == 0 || v == 1) {
                config->do_ent_filtering = v;
            } else {
                return 18;
            }
        } else {
            return 18;
        }
    //ent_min_mass
    } else if ((c = strstr(name, "ent_min_mass")) != NULL
        || (c = strstr(name, "emnm")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_min_mass = v;
            } else {
                return 19;
            }
        } else {
            return 19;
        }
    //ent_max_mass
    } else if ((c = strstr(name, "ent_max_mass")) != NULL
        || (c = strstr(name, "emxm")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_max_mass = v;
            } else {
                return 20;
            }
        } else {
            return 20;
        }
    //ent_min_width
    } else if ((c = strstr(name, "ent_min_width")) != NULL
        || (c = strstr(name, "emnw")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_min_width = v;
            } else {
                return 21;
            }
        } else {
            return 21;
        }
    //ent_max_width
    } else if ((c = strstr(name, "ent_max_width")) != NULL
        || (c = strstr(name, "emxw")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_max_width = v;
            } else {
                return 22;
            }
        } else {
            return 22;
        }
    //ent_min_height
    } else if ((c = strstr(name, "ent_min_height")) != NULL
        || (c = strstr(name, "emnh")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_min_height = v;
            } else {
                return 23;
            }
        } else {
            return 23;
        }
    //ent_max_height
    } else if ((c = strstr(name, "ent_max_height")) != NULL
        || (c = strstr(name, "emxh")) != NULL) {
        if (is_valid_filter_val(value)) {
            int v = str_to_filter_val(value);
            if (v != 0) {
                config->ent_max_height = v;
            } else {
                return 24;
            }
        } else {
            return 24;
        }
    //unknown variablename
    } else {
        return 1;
    }
    return 0;
}

//outputs the given 'config' to stdout 
void print_config(struct SysConfig *config) {
    output_config(config, stdout);
}

//saves the given 'config' to the file given by 'path'
//returns 0 if save successful
int save_config(struct SysConfig *config,
                char *path) {
    FILE *f;
    //attempt to open file
    f = fopen(path, "w");
    if (f == NULL)
        return 1;
    //write to file
    output_config(config, f);
    //close file
    fclose(f);
    return 0;
}

//outputs text of given 'config' to the given 'output'
void output_config(struct SysConfig *config,
                   FILE *output) {
    fprintf(output, "change_percent_threshold=%f\n", config->change_percent_threshold);
    fprintf(output, "pixel_change_threshold=%d\n", config->pixel_change_threshold);
    fprintf(output, "raw_img_output=%d\n", config->raw_img_output);
    fprintf(output, "diff_img_output=%d\n", config->diff_img_output);
    fprintf(output, "segmap_img_output=%d\n", config->segmap_img_output);
    fprintf(output, "median_img_count=%d\n", config->median_img_count);
    fprintf(output, "logs_path=%s\n", config->logs_path);
    fprintf(output, "logfile_path=%s\n", config->logfile_path);
    fprintf(output, "video_device=%s\n", config->video_device);
    fprintf(output, "ffmpeg_path=%s\n", config->ffmpeg_path);
    fprintf(output, "resolution=%s\n", config->resolution);
    fprintf(output, "gmm_k_val=%d\n", config->gmm_k_val);
    fprintf(output, "gmm_t_val=%f\n", config->gmm_t_val);
    fprintf(output, "gmm_alpha=%f\n", config->gmm_alpha);
    fprintf(output, "gmm_init_var=%d\n", (int) config->gmm_init_var);
    fprintf(output, "gmm_min_var=%d\n", (int) config->gmm_min_var);
    fprintf(output, "do_ent_filtering=%d\n", config->do_ent_filtering);
    fprintf(output, "ent_min_mass=%d\n", config->ent_min_mass);
    fprintf(output, "ent_max_mass=%d\n", config->ent_max_mass);
    fprintf(output, "ent_min_width=%d\n", config->ent_min_width);
    fprintf(output, "ent_max_width=%d\n", config->ent_max_width);
    fprintf(output, "ent_min_height=%d\n", config->ent_min_height);
    fprintf(output, "ent_max_height=%d\n", config->ent_max_height);
}

//initialises the given 'config' with the given values.
//returns 0 if all values given are valid.
int init_config(struct SysConfig *config,
                double cpt,
                int pct,
                int rio,
                int dio,
                int sio,
                int mic,
                char *logs,
                char *logf,
                char *vdev,
                char *ffmp,
                char *res,
                int gmk,
                double gmt,
                double gma,
                double giv,
                double gmv, 
                int eflt,
                int emnm,
                int emxm,
                int emnw,
                int emxw,
                int emnh,
                int emxh) {
    if (!config) {
        config = malloc(sizeof(struct SysConfig));
        if (!config)
            return 1;
    }
    
    config->change_percent_threshold = 0;
    config->pixel_change_threshold = 0;
    config->raw_img_output = 0;
    config->diff_img_output = 0;
    config->segmap_img_output = 0;
    config->median_img_count = 0;
    config->logs_path = NULL;
    config->logfile_path = NULL;
    config->video_device = NULL;
    config->ffmpeg_path = NULL;
    config->resolution = NULL;
    config->gmm_k_val = 0;
    config->gmm_t_val = 0;
    config->gmm_alpha = 0;
    config->gmm_init_var = 0;
    config->gmm_min_var = 0;
    config->do_ent_filtering = 0;
    config->ent_min_mass = 0;
    config->ent_max_mass = 0;
    config->ent_min_width = 0;
    config->ent_max_width = 0;
    config->ent_min_height = 0;
    config->ent_max_height = 0;
    
    if (cpt >= 0 && cpt <= 1)
        config->change_percent_threshold = cpt;
    else return 1;
    if (pct >= 0 && pct <= 255)
        config->pixel_change_threshold = pct;
    else return 1;
    if (rio == 0 || rio == 1)
        config->raw_img_output = rio;
    else return 1;
    if (dio == 0 || dio == 1)
        config->diff_img_output = dio;
    else return 1;
    if (sio == 0 || sio == 1)
        config->segmap_img_output = sio;
    else return 1;
    if (mic >=  1 && mic <= 100)
        config->median_img_count = mic;
    else return 1;
    if (is_valid_dir(logs))
        config->logs_path = logs;
    else return 1;
    if (is_valid_file(logf))
        config->logfile_path = logf;
    else return  1;
    if (1) //TODO device verification
        config->video_device = vdev;
    else return 1;
    if (1) //TODO executable verification
        config->ffmpeg_path = ffmp;
    else return 1;
    if (1) //TODO resolution verification
        config->resolution = res;
    else return 1;
    if (gmk >= 1 && gmk <= 5)
        config->gmm_k_val = gmk;
    else return 1;
    if (gmt >= 0.0 && gmt <= 1.0)
        config->gmm_t_val = gmt;
    else return 1;
    if (gma >= 0.0 && gma <= 1.0)
        config->gmm_alpha = gma;
    else return 1;
    if (giv >= 0.0 && giv <= 255.0)
        config->gmm_init_var = giv;
    else return 1;
    if (gmv >= 0.0 && gmv <= 255.0)
        config->gmm_min_var = gmv;
    else return 1;
    if (eflt >= -1)
        config->do_ent_filtering = eflt;
    else return 1;
    if (emnm >= -1)
        config->ent_min_mass = emnm;
    else return 1;
    if (emxm >= -1)
        config->ent_max_mass = emxm;
    else return 1;
    if (emnw >= -1)
        config->ent_min_width = emnw;
    else return 1;
    if (emxw >= -1)
        config->ent_max_width = emxw;
    else return 1;
    if (emnh >= -1)
        config->ent_min_height = emnh;
    else return 1;
    if (emxh >= -1)
        config->ent_max_height = emxh;
    else return 1;
    return 0;
}

//loads the config from the file specified by 'path' into the given 'config'
//returns 0 on success
//error codes
//  1 - i/o error
//  2 - couldn't set change_percent_threshold
//  3 - couldn't set pixel_change_threshold
//  4 - couldn't set raw_img_output
//  5 - couldn't set diff_img_output
//  6 - couldn't set segmap_img_output
//  7 - couldn't set median_img_count
//  8 - couldn't set logs_path
//  9 - couldn't set logfile_path
// 10 - couldn't set video_device
// 11 - couldn't set ffmpeg_path
// 12 - couldn't set resolution
// 13 - couldn't set gmm_k_val
// 14 - couldn't set gmm_t_val
// 15 - couldn't set gmm_alpha
// 16 - couldn't set gmm_init_var
// 17 - couldn't set gmm_min_var
// 18 - couldn't set do_ent_filtering
// 19 - couldn't set ent_min_mass
// 20 - couldn't set ent_max_mass
// 21 - couldn't set ent_min_width
// 22 - couldn't set ent_max_width
// 23 - couldn't set ent_min_height
// 24 - couldn't set ent_max_height
int load_config(struct SysConfig *config,
                char *path) {
    FILE *f;
    char *line = NULL;
    size_t n = 0;
    
    //attempt to open file
    f = fopen(path, "r");
    if (f == NULL) {
        return 1;
    }
    
    //read lines
    while (getline(&line, &n, f) != -1) {
        if (line[0] != '#' && line[0] != '\n') {   //ignore comment and empty lines
            //change_percent_threshold
            if (strstr(line, "change_percent_threshold=") != NULL) {
                if (set(config, "change_percent_threshold", &line[25]) != 0) {
                    return 2; //unable to set value, return error
                }
            //pixel_change_threshold
            } else if (strstr(line, "pixel_change_threshold=") != NULL) {
                if (set(config, "pixel_change_threshold", &line[23]) != 0) {
                    return 3; //unable to set value, return error
                }
            //raw_img_output
            } else if (strstr(line, "raw_img_output=") != NULL) {
                if (set(config, "raw_img_output", &line[15]) != 0) {
                    return 4; //unable to set value, return error
                }
            //diff_img_output
            } else if (strstr(line, "diff_img_output=") != NULL) {
                if (set(config, "diff_img_output", &line[16]) != 0) {
                    return 5; //unable to set value, return error
                }
            //segmap_img_output
            } else if (strstr(line, "segmap_img_output=") != NULL) {
                if (set(config, "segmap_img_output", &line[18]) != 0) {
                    return 6; //unable to set value, return error
                }
            //median_img_count
            } else if (strstr(line, "median_img_count=") != NULL) {
                if (set(config, "median_img_count", &line[17]) != 0) {
                    return 7; //unable to set value, return error
                }
            //logs_path
            } else if (strstr(line, "logs_path=") != NULL) {
                if (set(config, "logs_path", &line[10]) != 0) {
                    return 8; //unable to set value, return error
                }
            //logfile_path
            } else if (strstr(line, "logfile_path=") != NULL) {
                if (set(config, "logfile_path", &line[13]) != 0) {
                    return 9; //unable to set value, return error
                }
            //video_device
            } else if (strstr(line, "video_device=") != NULL) {
                if (set(config, "video_device", &line[13]) != 0) {
                    return 10; //unable to set value, return error
                }
            //ffmpeg_path
            } else if (strstr(line, "ffmpeg_path=") != NULL) {
                if (set(config, "ffmpeg_path", &line[12]) != 0) {
                    return 11; //unable to set value, return error
                }
            //resolution
            } else if (strstr(line, "resolution=") != NULL) {
                if (set(config, "resolution", &line[11]) != 0) {
                    return 12; //unable to set value, return error
                }
            //gmm_k_val
            } else if (strstr(line, "gmm_k_val=") != NULL) {
                if (set(config, "gmm_k_val", &line[10]) != 0) {
                    return 13; //unable to set value, return error
                }
            //gmm_t_val
            } else if (strstr(line, "gmm_t_val=") != NULL) {
                if (set(config, "gmm_t_val", &line[10]) != 0) {
                    return 14; //unable to set value, return error
                }
            //gmm_alpha
            } else if (strstr(line, "gmm_alpha=") != NULL) {
                if (set(config, "gmm_alpha", &line[10]) != 0) {
                    return 15; //unable to set value, return error
                }
            //gmm_init_var
            } else if (strstr(line, "gmm_init_var=") != NULL) {
                if (set(config, "gmm_init_var", &line[13]) != 0) {
                    return 16; //unable to set value, return error
                }
            //gmm_min_var
            } else if (strstr(line, "gmm_min_var=") != NULL) {
                if (set(config, "gmm_min_var", &line[12]) != 0) {
                    return 17; //unable to set value, return error
                }
            //do_ent_filtering
            } else if (strstr(line, "do_ent_filtering=") != NULL) {
                if (set(config, "do_ent_filtering", &line[17]) != 0) {
                    return 18; //unable to set value, return error
                }
            //ent_min_mass
            } else if (strstr(line, "ent_min_mass=") != NULL) {
                if (set(config, "ent_min_mass", &line[13]) != 0) {
                    return 19; //unable to set value, return error
                }
            //ent_max_mass
            } else if (strstr(line, "ent_max_mass=") != NULL) {
                if (set(config, "ent_max_mass", &line[13]) != 0) {
                    return 20; //unable to set value, return error
                }
            //ent_min_width
            } else if (strstr(line, "ent_min_width=") != NULL) {
                if (set(config, "ent_min_width", &line[14]) != 0) {
                    return 21; //unable to set value, return error
                }
            //ent_max_width
            } else if (strstr(line, "ent_max_width=") != NULL) {
                if (set(config, "ent_max_width", &line[14]) != 0) {
                    return 22; //unable to set value, return error
                }
            //ent_min_height
            } else if (strstr(line, "ent_min_height=") != NULL) {
                if (set(config, "ent_min_height", &line[15]) != 0) {
                    return 23; //unable to set value, return error
                }
            //ent_max_height
            } else if (strstr(line, "ent_max_height=") != NULL) {
                if (set(config, "ent_max_height", &line[15]) != 0) {
                    return 24; //unable to set value, return error
                }
            }
        }
        n = 0;
        free(line);
        line = NULL;
    }
    free(line);
    
    //close file
    fclose(f);
    return 0;
}

//frees the config
void free_conf(struct SysConfig *conf) {
    free(conf->logs_path);
    free(conf->logfile_path);
    free(conf->video_device);
    free(conf->ffmpeg_path);
    free(conf->resolution);
    free(conf);
}

//----------------
//helper functions
//----------------

//checks whether the string in 'value' is in the range 0-255
//returns 1 if value is a number in that range
int is_uns_char(char *value) {
    //check value string is 0 - 255
    return (value[0] >= '0' && value[0] <= '9' &&     //check for 0-9
            (value[1] == '\0' || value[1] == '\n'))
            || (value[0] >= '0' && value[0] <= '9' && //check for 00-99
            value[1] >= '0' && value[1] <= '9' &&
            (value[2] == '\0' || value[2] == '\n'))
            || (value[0] >= '0' && value[0] <= '1' && //check for 000-199
            value[1] >= '0' && value[1] <= '9' && 
            value[2] >= '0' && value[2] <= '9' && 
            (value[3] == '\0' || value[3] == '\n'))
            || (value[0] == '2' &&                //check for 200-249
            value[1] >= '0' && value[1] <= '4' &&
            value[2] >= '0' && value[2] <= '9' &&
            (value[3] == '\0' || value[3] == '\n'))
            || (value[0] == '2' &&                //check for 250-255
            value[1] == '5' &&
            value[2] >= '0' && value[2] <= '5' &&
            (value[3] == '\0' || value[3] == '\n'));
}

//checks whether the string in value is a bool (0 or 1)
//returns 1 if string is a single character, 0 or 1.
int is_bool(char *value) {
    if (value[1] == '\0' || value[1] == '\n')
        return (value[0] == '0' || value[0] == '1');
    else
        return 0;

}

//checks whether the string in value represents a double in the range 0.0 - 1.0.
//returns 1 if value represents a double
int is_double(char *value) {
    if (value[0] == '0' && value[1] == '.') { //0.0 - 0.999...
        if (value[2] == '\0') {
            return 0;
        }
        int i = 2;
        while (value[i] != '\0' && value[i] != '\n') {
            if (value[i] >= '0' && value[i] <= '9') {
                i++;
            } else {
                return 0;
            }
        }
        return 1;
    } else if (value[0] == '1' && value[1] == '.') { //1.000...
        int i = 2;
        while (value[i] != '\0' && value[i] != '\n') {
            if (value[i] == '0') {
                i++;
            } else {
                return 0;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

//converts string to double, assumes is_double(value_str) == 1.
double str_to_double(char *value_str) {
    double val;
    if (value_str[0] == '1')
        return 1.0;
    else
        val = 0.0;
    long fraction = 0;          //read fractional part into long first
    int i = 2;
    while (value_str[i] != '\0' && value_str[i] != '\n') {
        fraction = (fraction * 10) + (value_str[i++] - '0');
    }
    val += fraction / (pow(10.0, (double) (i - 2))); //convert fraction long into an actual decimal value
    return val;
}

//converts string to unsigned char, assumes is_uns_char(value_str) == 1.
unsigned char str_to_uns_char(char *value_str) {
    if (value_str[1] == '\0' || value_str[1] == '\n') {
        return (value_str[0]-'0');
    } else if (value_str[2] == '\0' || value_str[2] == '\n') {
        return ((value_str[0]-'0') * 10) + (value_str[1]-'0');
    } else {
        return ((value_str[0]-'0') * 100) + ((value_str[1]-'0') * 10) + (value_str[2]-'0');
    }
}

//converts string to positive int or -1.
//if an error occurs, -1 is also returned as filters use -1 as their ignore value
int str_to_filter_val(char *str) {
    int i, n;
    if (str[0] == '-') {
        return -1;
    }
    n = 0;
    for (i = 0; i < strlen(str); i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            n = (n*10) + (str[i]-'0');
        } else if (str[i] == '\n') {
            return n;
        } else {
            return -1;
        }
    }
    return n;
}

//checks if dir path string is valid
int is_valid_dir(char *dpath) {
    DIR* dir = opendir(dpath);
    if (!dir) {
        return 0;
    } else {
        closedir(dir);
        return 1;
    }
}

//checks if path string is valid
int is_valid_file(char *fpath) {
    FILE *fp = fopen(fpath, "r");
    if (!fp) {
        return 0;
    } else {
        fclose(fp);
        return 1;
    }
}

//checks if string is -1 or a positive number
int is_valid_filter_val(char *str) {
    return (strlen(str) == 3 && str[0] == '-' && str[1] == '1') || is_uns_int(str);
}

//check is string is a positive number
int is_uns_int(char *value) {
    int i;
    int len;
    len = strlen(value);
    for (i = 0; i < len; i++) {
        if (!((value[i] >= '0' && value[i] <= '9') || value[i] == '\n')) {
            return 0;
        }
    }
    return 1;
}

//replaces first newline character found with c
int replace_newline(char *str, char c) {
    int i;
    for (i = 0; i < strlen(str); i++) {
        if (str[i] == '\n') {
            str[i] = c;
            return 1;
        }
    }
    return 0;
}    
