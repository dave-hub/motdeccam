#include "lib/configuration.h"
#include "lib/bitmap.h"
#include "lib/bitmap_thr.h"
#include "lib/gmmodel.h"
#include "lib/gmmodel_thr.h"
#include "lib/entitydet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

//globals
struct SysConfig *conf = NULL;
int running = 1;

//function declarations
void handle_mot_det();

void log_motion_event(char *timestamp,
                      long pixel_change_count,
                      double change_percent);

void log_error(char *errstring);

void log_event(char *evstring);

char *get_full_timestamp();

char *get_date_timestamp();

char *get_time_timestamp();

int capture_img(char *filename);

int capture_video(char *filename);

void set_motdec_info(int running);

void sighandler(int sig);

// ---------
// FUNCTIONS
// ---------

int main(int argc, char **argv) {
    char *command;
    
    //check if no args
    if (argc < 2) {
        puts("-- USAGE --");
        puts("start [cfg]      - Start the motion detection. Optional [cfg] for loading config");
        puts("set <name> <val> - Sets the system variable <name> to <val>.");
        puts("help             - Display help message.");
        return 0;
    }
    
    command = argv[1];
    
    //PARSE INPUT
    //start
    if (strstr(command, "start") != NULL) {
        //INIT
        char *cfgpath = "cfg/default.cfg";
        
        //check if config file specified
        if (argc == 3 && is_valid_file(argv[2])){
            cfgpath = argv[2];
        }
        
        //initialise config
        conf = malloc(sizeof(struct SysConfig));
        if (!conf) {
            puts("Memory error, closing");
            exit(1);
        }
        
        int eno;
        //load default config or init with defaults
        if ((eno = load_config(conf, cfgpath)) != 0) {
            printf("Error (%d): could not load %s, using defaults.\n",eno, cfgpath);
            init_config(conf, 0.2, 30, 1, 1, 1, 10, 
                        "log.txt", "logs/", "/dev/video0",
                        "/bin/ffmpeg", "640x480",
                        3, 0.6, 0.05, 12.0, 3.0,
                        0, -1, -1, -1, -1, -1, -1);
        } else {
            printf("Loaded config: %s\n", cfgpath);
        }

        //setup interrupt handler
        signal(SIGINT, sighandler);
        signal(SIGTERM, sighandler);
        
        //set info file and log start of program
        set_motdec_info(1);
        log_event("Starting motdec...");
        
        //start
        handle_mot_det();
    } 
    //set
    else if (strstr(command, "set") != NULL) {
        char *token, *cfgpath, *name, *val;
        struct SysConfig *config;
        int ret;
        
        //check correct num args
        if (argc < 5) {
            puts("Error: expected 3 arguments for 'set'");
            puts("Usage: set <cfgpath> <name> <value>");
            return 1;
        }
        
        cfgpath = argv[2];
        name = argv[3];
        val = argv[4];
        
        //load config
        config = malloc(sizeof(struct SysConfig));
        if (!config) {
            puts("Error: Couldn't allocate memory for config");
            return 1;
        }
        if (load_config(config, cfgpath) != 0) {
            printf("Error: couldn't load specified config file %s\n", cfgpath);
            return 1;
        }
        
        //call set
        ret = set(config, name, val);
        
        //handle errors
        if (ret == 0) {
            printf("Variable '%s' has been set to '%s'\n", name, val);
        } else if (ret == 1) {
            puts("Error: Unknown variable name");
        } else if (ret == 2) {
            puts("Error: change_percent_threshold must be 0.0 - 1.0");
        } else if (ret == 3) {
            puts("Error: pixel_change_threshold must be 0 - 255");
        } else if (ret == 4) {
            puts("Error: raw_img_output must be 0 - 1");
        } else if (ret == 5) {
            puts("Error: diff_img_output must be 0 - 1");
        } else if (ret == 6) {
            puts("Error: segmap_img_output must be 0 - 1");
        } else if (ret == 7) {
            puts("Error: median_img_count must be 1 - 255");
        } else if (ret == 8) {
            puts("Error: logs_path must be a valid path to directory");
        } else if (ret == 9) {
            puts("Error: logfile_path must be a valid path to file");
        } else if (ret == 10) {
            puts("Error: video_device must be a valid path to a video device.");
        } else if (ret == 11) {
            puts("Error: ffmpeg_path must be a valid path to ffmpeg executable.");
        } else if (ret == 12) {
            puts("Error: resolution must be of form widthxheight (e.g. 640x480)");
        } else if (ret == 13) {
            puts("Error: gmm_k_val must be 1 - 5");
        } else if (ret == 14) {
            puts("Error: gmm_t_val must be 0.0 - 1.0");
        } else if (ret == 15) {
            puts("Error: gmm_alpha must be 0.0 - 1.0");
        } else if (ret == 16) {
            puts("Error: gmm_init_var must be 0 - 255");
        } else if (ret == 17) {
            puts("Error: gmm_min_var must be 0 - 255");
        }
        
        //save config
        if (save_config(config, cfgpath) != 0) {
            puts("Error: couldn't save config to specified path.");
            return 1;
        }
        
        return 0;
    } 
    //help
    else if (strstr(command, "help") != NULL) {
        puts("\n-- USAGE --");
        puts("start            - Start the motion detection.");
        puts("set <name> <val> - Sets the system variable <name> to <val>.");
        puts("help             - Display this message.");
        puts("\n-- INFO --");
        puts("Program that logs motion events tracked through a webcam.");
        puts("The program takes images using a shell script 'img_capture.sh'.");
        puts("You can edit this file to change how the picture is taken.");
        puts("Event statistics are logged in the default 'log.txt' file.");
        puts("The 'logs/' directory stores the images captured for each event.");
        puts("\n-- SYSTEM VARIABLES --");
        puts("Each system variable can be changed by using the 'set' command.");
        puts("Defined below are the variable names and values.\n");
        puts(" <name> (min <val> - max <val>) [abbreviation]\n");
        puts(" change_percent_threshold (0.0 - 1.0) [cpt]");
        puts("  - the percentage of changed pixels above which there is considered motion");
        puts(" pixel_change_threshold (0 - 255) [pct]");
        puts("  - the value above which the change in a pixel is considered motion");
        puts(" raw_img_output (0 - 1) [rio]");
        puts("  - output raw images from motion events");
        puts(" diff_img_output (0 - 1) [dio]");
        puts("  - output 'difference' images from motion events");
        puts(" segmap_img_output (0 - 1) [sio]");
        puts("  - output 'segmentation map' images from motion events");
        puts(" median_img_count (1 - 255) [mic]");
        puts("  - number of images in a median model");
        puts(" logs_path (path to file) [logs]");
        puts("  - path to the file where events are logged");
        puts(" logfile_path (path to directory) [logf]");
        puts("  - path to the folder where images from motion events are stored");
        puts(" video_device (path to video device) [vdev]");
        puts("  - path to video device to use for image capture. e.g. /dev/video0");
        puts(" ffmpeg_path (path to executable) [ffmp]");
        puts("  - path to ffmpeg executable used for image capture. e.g. /bin/ffmpeg");
        puts(" resolution (of form widthxheight) [res]");
        puts("  - resolution of image capture. e.g. 640x480");
        puts(" gmm_k_val (1 - 5) [gmk]");
        puts("  - number of distributions used to model each pixel in the gaussian model.");
        puts(" gmm_t_val (0.0 - 1.0) [gmt]");
        puts("  - T, the proportion of data that accounts for the background in the gaussian model.");
        puts(" gmm_alpha (0.0 - 1.0) [gma]");
        puts("  - the learning rate for the gaussian model.");
        puts(" gmm_init_var (0 - 255) [giv]");
        puts("  - the initial variance to set each gaussian distribution to.");
        puts(" gmm_min_var (0 - 255) [gmv]");
        puts("  - the minimum variance that each distribution can have.");
        puts("\nUse 'set' and the name or abbreviation of a variable to change the value.");
        puts("Values given must be in the range specified above.");
        puts(" -- -- --\n");
        return 0;
    }
    
    free_conf(conf);
}

//loop for image capture and motion detection
void handle_mot_det() {
    struct GaussianModel *model;
    struct EntityFilter filter;
    struct BMP *bg, *change, *segmap, *black;
    int i;
    unsigned int imgw, imgh;
    
    //get filter from config
    filter = get_config_filter(conf);
    
    //create motdecimg.bmp file in tmp
    //capture call only overwrites image
    FILE *fp = fopen("/tmp/motdecimg.bmp", "ab+");
    if (fp) fclose(fp);
    
    //take initial base image
    if (capture_img("/tmp/motdecimg.bmp") != 0) {
        log_error("Error: Error capturing image.");
        free_gaussian_model(model);
        log_event("Stopping motdec...");
        set_motdec_info(0);
        return;
    }
    
    sleep(1);
    
    //take initial base image again
    if (capture_img("/tmp/motdecimg.bmp") != 0) {
        log_error("Error: Error capturing image.");
        free_gaussian_model(model);
        log_event("Stopping motdec...");
        set_motdec_info(0);
        return;
    }
    
    puts("\nTraining model on background scene...");
    puts("Keep scene free from foreground objects.\n");
    
    sleep(3);
    
    //load base image to init model
    bg = load_BMP("/tmp/motdecimg.bmp");
    if (!bg) {
        log_error("Error: Unable to load base image.");
        free_gaussian_model(model);
        log_event("Stopping motdec...");
        set_motdec_info(0);
        return;
    }
    
    imgw = bg->image_header->width;
    imgh = bg->image_header->height;
    
    //init gaussian model with base image
    model = init_gaussian_model(bg, 
                                conf->gmm_k_val,
                                conf->gmm_t_val,
                                conf->gmm_alpha,
                                conf->gmm_init_var,
                                conf->gmm_min_var);
    
    free_BMP(bg);
    
    //create black image for use as segmap in training
    black = init_BMP(imgw, imgh);
    
    
    //train model for 10 frames
    for (i = 0; i < 10; i++) {
        //take initial base image
        if (capture_img("/tmp/motdecimg.bmp") != 0) {
            log_error("Error: Error capturing image.");
            free_gaussian_model(model);
            log_event("Stopping motdec...");
            set_motdec_info(0);
            return;
        }
        
        //load base image to init model
        bg = load_BMP("/tmp/motdecimg.bmp");
        if (!bg) {
            log_error("Error: Unable to load base image.");
            free_gaussian_model(model);
            log_event("Stopping motdec...");
            set_motdec_info(0);
            return;
        }
        
        //update and normalise
        update_gaussian_model_thr(model, bg, black);
        normalize_priors_thr(model);
        printf("Training: %d\%\n", i*10);
        
        free(bg);
    }
    
    free(black);
    
    puts("Training complete, system is now active.");
    
    //enter loop
    while (running) {
        
        //take change image
        if (capture_img("/tmp/motdecimg.bmp") != 0) {
            log_error("Error: Error capturing image.");
            printf("Error no: %d", errno);
            free(bg);
            free(change);
            free(segmap);
            break;
        }
        
        //load change image
        change = load_BMP("/tmp/motdecimg.bmp");
        if (!change) {
            log_error("Error: Unable to load change image.");
            free_BMP(change);
            break;
        }
        
        //check for motion
        long pixel_change_count;
        double change_percent;
        struct Pixel p;
        
        pixel_change_count = 0L;
        change_percent = 0.0;
        
        //generate segmap
        segmap = generate_gaussian_seg_map_thr(model, change);
        
        if (conf->do_ent_filtering) {
            filter_entities(segmap, filter, 0);
        }
                
        //count foreground pixels
        pixel_change_count = count_pixels_thr(segmap, make_pixel(255,255,255));
        
        //calculate change percent and compare to threshold
        change_percent = (double) ((double) pixel_change_count / (double) (imgw * imgh));
                
        if (change_percent > conf->change_percent_threshold) {
            
            //get timestamp
            char *fullts, *datets, *timets, *datetsdir, *timetsdir;
            
            fullts = get_full_timestamp();
            datets = get_date_timestamp();
            timets = get_time_timestamp();
            datetsdir = malloc(strlen(datets) + strlen(conf->logs_path) + 2);
            if (!datetsdir) {
                printf("Error: Memory Error.");
                break;
            }
            strcpy(datetsdir, conf->logs_path);
            strcat(datetsdir, datets);
            
            //create datestamped folder for event
            if (mkdir(datetsdir, 0777) != 0) {
                //check if fail because folder exists
                if (errno != EEXIST) {
                    log_error("Error: Unable to make new folder for motion event.");
                    free_BMP(bg);
                    free_BMP(change);
                    free_BMP(segmap);
                    free(fullts);
                    free(datets);
                    free(timets);
                    free(datetsdir);
                    free(timetsdir);
                    break;
                }
            }
            
            timetsdir = malloc(strlen(datetsdir) + 100);
            if (!datetsdir) {
                printf("Error: Memory Error.");
                break;
            }
            strcpy(timetsdir, datetsdir);
            strcat(timetsdir, "/");
            strcat(timetsdir, timets);
            
            //create timestamped folder for event
            if (mkdir(timetsdir, 0777) != 0) {
                //check if fail because folder exists
                if (errno != EEXIST) {
                    log_error("Error: Unable to make new folder for motion event.");
                    free_BMP(bg);
                    free_BMP(change);
                    free_BMP(segmap);
                    free(fullts);
                    free(datets);
                    free(timets);
                    free(datetsdir);
                    free(timetsdir);
                    break;
                }
            }
            
            //check for raw_img_output
            if (conf->raw_img_output) {
                char *bgdir, *changedir;
                //generate background
                bg = generate_gaussian_background_thr(model);
                
                bgdir = malloc(strlen(timetsdir) + 100);
                changedir = malloc(strlen(timetsdir) + 100);
                strcpy(bgdir, timetsdir);
                strcpy(changedir, timetsdir);
                strcat(bgdir, "/bg.bmp");
                strcat(changedir, "/change.bmp");
                if (!save_BMP(bg, bgdir) ||
                    !save_BMP(change, changedir)){
                    log_error("Error: Unable to save raw images.");
                    free_BMP(bg);
                    free_BMP(change);
                    free_BMP(segmap);
                    free(fullts);
                    free(datets);
                    free(timets);
                    free(datetsdir);
                    free(timetsdir);
                    break;
                }
                free(bgdir);
                free(changedir);
                free_BMP(bg);
            }
        
            //check for segmap_img_output (same a seg map)
            if (conf->segmap_img_output) {
                char *motdir;
                motdir = malloc(strlen(timetsdir) + 100);
                strcpy(motdir, timetsdir);
                strcat(motdir, "/segmap.bmp");
                if (!save_BMP(segmap, motdir)){
                    log_error("Error: Unable to save segentation map image.");
                    free_BMP(bg);
                    free_BMP(change);
                    free_BMP(segmap);
                    free(fullts);
                    free(datets);
                    free(timets);
                    free(datetsdir);
                    free(timetsdir);
                    break;
                }
                free(motdir);
            }
            
            //log event to stdout
            log_motion_event(fullts, pixel_change_count, change_percent);
            
            //record event
            char videopath[256];
            strcpy(videopath, timetsdir);
            strcat(videopath, "/");
            strcat(videopath, "output.mp4");
            printf("\nCAPTURING VIDEO PLEASE WAIT 15s\n");
            capture_video(videopath);
            printf("Video output to: %s\nResuming system...\n\n", videopath);
            
            free(fullts);
            free(datets);
            free(timets);
            free(datetsdir);
            free(timetsdir);
        }
        
        //print_mixture(model, 1, 1);
        //update model with newest image
        update_gaussian_model_thr(model, change, segmap);
        normalize_priors_thr(model);
        
        //free created images
        free_BMP(change);
        free_BMP(segmap);
        //break;
    }
    
    free_gaussian_model(model);
    log_event("Stopping motdec...");
    set_motdec_info(0);
}

//log event to common log file
void log_motion_event(char *timestamp,
                      long pixel_change_count,
                      double change_percent) {
    FILE *file;
    char buffer [255];
    sprintf(buffer, "%s | Pixels Changed: %lu | Change Percentage: %4.2f\n", timestamp, pixel_change_count, (change_percent * 100));
    file = fopen(conf->logfile_path, "a+");
    fputs(buffer, file);
    printf("%s", buffer);
    fclose(file);
}

//log error to common log file
void log_error(char *errstring) {
    log_event(errstring);
}

//log event string to common log file
void log_event(char *evstring) {
    FILE *file;
    char buffer[255];
    char *timestamp;
    timestamp = get_full_timestamp();
    sprintf(buffer, "%s | %s\n", timestamp, evstring);
    file = fopen(conf->logfile_path, "a+");
    fputs(buffer, file);
    printf("%s", buffer);
    fclose(file);
    free(timestamp);
}
    
//returns the current date-time as yyyy.mm.dd-hh:mm:ss
char *get_full_timestamp() {
    time_t rt;
    struct tm * ti;
    char *timestamp, buffer[50];
    int len;

    //get raw time and convert to a time info struct
    time(&rt);
    ti = localtime(&rt);
    
    //format time string with mkdir command
    sprintf(buffer,
            "%d.%02d.%02d-%02d:%02d:%02d",
            ti->tm_year + 1900,
            ti->tm_mon + 1,
            ti->tm_mday, 
            ti->tm_hour,
            ti->tm_min,
            ti->tm_sec);
    
    //copy temp buffer to pointer
    len = strlen(buffer);
    timestamp = malloc(len+1);
    strcpy(timestamp, buffer);
    
    return timestamp;
}

//returns the current date as yyyy.mm.dd
char *get_date_timestamp() {
    time_t rt;
    struct tm * ti;
    char *timestamp, buffer[50];
    int len;

    //get raw time and convert to a time info struct
    time(&rt);
    ti = localtime(&rt);
    
    //format time string with mkdir command
    sprintf(buffer,
            "%d.%02d.%02d",
            ti->tm_year + 1900,
            ti->tm_mon + 1,
            ti->tm_mday);
    
    //copy temp buffer to pointer
    len = strlen(buffer);
    timestamp = malloc(len+1);
    strcpy(timestamp, buffer);
    
    return timestamp;
}

//returns the current time as hh:mm:ss
char *get_time_timestamp() {
    time_t rt;
    struct tm * ti;
    char *timestamp, buffer[50];
    int len;

    //get raw time and convert to a time info struct
    time(&rt);
    ti = localtime(&rt);
    
    //format time string with mkdir command
    sprintf(buffer,
            "%02d:%02d:%02d",
            ti->tm_hour,
            ti->tm_min,
            ti->tm_sec);
    
    //copy temp buffer to pointer
    len = strlen(buffer);
    timestamp = malloc(len+1);
    strcpy(timestamp, buffer);
    
    return timestamp;
}

//capture image through fork using ffmpeg
int capture_img(char *filename) {
    pid_t pid = fork();
    char *args[14];
    
    //TODO should sanitize filename
    
    // ffmpeg args
    args[0] = conf->ffmpeg_path;
    //-y -loglevel panic
    args[1] = "-y";
    args[2] = "-loglevel";
    args[3] = "panic";
    // -f video4linux2 
    args[4] = "-f";
    args[5] = "video4linux2";
    // -i
    args[6] = "-i";
    // copy given video device to args list
    args[7] = conf->video_device;
    // -vframes 2 
    args[8] = "-vframes";
    args[9] = "1";
    // -s
    args[10] = "-s";
    // copy given resolution to args list
    args[11] = conf->resolution;
    // /tmp/img.bmp
    args[12] = filename;
    args[13] = (char *) NULL;
    
    //fork error return
    if (pid == -1) {
        return -2;
    //parent - wait
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return status;
    //child - exec
    } else {
        execv(conf->ffmpeg_path, args);
        _exit(EXIT_FAILURE);
    }
    
    return 0;
}

//capture 15 second of video through fork using ffmpeg
int capture_video(char *filename) {

    pid_t pid = fork();
    char *args[16];
    
    //TODO should sanitize filename
    
    // ffmpeg args
    args[0] = conf->ffmpeg_path;
    //-y -loglevel panic
    args[1] = "-y";
    args[2] = "-loglevel";
    args[3] = "panic";
    //-framerate 20
    args[4] = "-framerate";
    args[5] = "20";
    //-video_size
    args[6] = "-video_size";
    args[7] = conf->resolution;
    //-t 60
    args[8] = "-t";
    args[9] = "15";
    // -i videodevice
    args[10] = "-i";
    args[11] = conf->video_device;
    //-movflags faststart
    args[12] = "-movflags";
    args[13] = "+faststart";
    // logs/date/timevideopath.mp4
    args[14] = filename;
    args[15] = (char *) NULL;
    
    //fork error return
    if (pid == -1) {
        return -2;
    //parent - wait
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return status;
    //child - exec
    } else {
        execv(conf->ffmpeg_path, args);
        _exit(EXIT_FAILURE);
    }
    
    return 0;
}

//sets tmp/motdec.info 
void set_motdec_info(int running) {
    FILE *fp;
    
    //open file
    fp = fopen("/tmp/motdec.info", "w+");
    if (!fp)
        log_error("Error: Cannot open /tmp/motdec.info");

    //write xml to file
    fprintf(fp, "<?xml version='1.0'?>");
    fprintf(fp, "<info>");
    if (running) {
        fprintf(fp, "<running>true</running>");
    } else {
        fprintf(fp, "<running>false</running>");
    }
    fprintf(fp, "<logfile>%s</logfile>", conf->logfile_path);
    fprintf(fp, "<logsdir>%s</logsdir>", conf->logs_path);
    fprintf(fp, "</info>");
    //close file
    fclose(fp);
}

//handles signals
void sighandler(int sig) {
    printf("\nSignal caught\n");
    running = 0;
}
