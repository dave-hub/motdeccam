//--------
//includes
//--------

#include <stdio.h>

//------------------
//struct definitions
//------------------

struct Entity {
    unsigned char id;
    int mass;
    int minx;
    int maxx;
    int miny;
    int maxy;
};

struct EntityList {
    struct Entity *entity;
    struct EntityList *next;
};

//struct to define the restrictions to filter unwanted entities
struct EntityFilter {
    int min_mass;
    int max_mass;
    int min_width;
    int max_width;
    int min_height;
    int max_height;
};

struct PointList {
    int x;
    int y;
    struct PointList *next;
};

//---------------------
//function declarations
//---------------------

struct EntityList *find_entities(struct BMP *segmap);

void tag_entity(struct BMP *segmap,
                struct Entity *entity);

void target_tag_entity(struct BMP *segmap,
                struct Entity *entity,
                struct Pixel target);

struct EntityList *filter_entities(struct BMP *segmap,
                                   struct EntityFilter filter,
                                   int tag_segmap);

struct EntityList *old_filter_entities(struct BMP *segmap,
                                  struct BMP *tagged_segmap,
                                  struct EntityList *entities,
                                  struct EntityFilter filter);

int passes_filter(struct Entity *entity,
                  struct EntityFilter filter);

struct EntityFilter init_filter(int min_mass,
                                int max_mass,
                                int min_width,
                                int max_width,
                                int min_height,
                                int max_height);

struct EntityFilter get_config_filter(struct SysConfig *conf);

struct Entity *init_entity(int id, 
                           int x, 
                           int y);

struct EntityList *init_entity_list(struct Entity *entity);

struct EntityList *add_entity(struct EntityList *head,
                              struct Entity *entity);

struct PointList *init_point_list(int x,
                                  int y);

struct PointList *pop_point(struct PointList *list);

struct PointList *add_point(struct PointList *head,
                            int x,
                            int y);

void print_entity_list(struct EntityList *el);

void print_entity(struct Entity *e);

void print_point_list(struct PointList *pl);

void print_filter(struct EntityFilter ef);

//-------------------------
//main function definitions
//-------------------------
    
//tags the entities within given segmap BMP and returns list of entities
struct EntityList *find_entities(struct BMP *segmap) {
    struct Entity *new_entity;
    struct EntityList *elist;
    unsigned char id;
    int x, y;
    
    x = y = 0;
    id = 1;
    
    elist = NULL;
    
    
    //iterate through pixel data
    for (y = 0; y < segmap->image_header->height; y++) {
        for (x = 0; x < segmap->image_header->width; x++) {
            //check for foreground pixel
            if (is_foreground(get_pixel(segmap, x, y))) {
                
                //make Entity struct
                new_entity = init_entity(id, x, y);
                
                //check
                
                //add to list
                elist = add_entity(elist, new_entity); 
                
                //tag new entity
                tag_entity(segmap, new_entity);
                
                id += 1;
            }
        }
    }
    return elist;
}

//tags any region of 255 connected to the entity initial start point
//with the entity's id, and fills info in Entity struct
//standard version for white segmaps
void tag_entity(struct BMP *segmap,
                struct Entity *entity) {
    target_tag_entity(segmap, entity, make_pixel(255,255,255));
}

//tags any region of 255 connected to the entity initial start point
//with the entity's id, and fills info in Entity struct
//target pixel are the pixels considered part of the entity, to be tagged
//usually 255 for normal segmaps or the id for retagging/blackout operations
void target_tag_entity(struct BMP *segmap,
                struct Entity *entity,
                struct Pixel target) {
    struct Pixel tag;
    struct PointList *queue;
    int x, y;
    unsigned int id;
    
    //get start point from initial entity struct
    x = entity->minx;//minx or maxx works as they are equal
    y = entity->miny;
    
    id = entity->id;
    tag = make_pixel(id, id, id); //pixel used to tag in segmap
    
    //pop initial point to queue
    queue = init_point_list(x, y);
    
    //while queue still has points
    while (queue != NULL) {
        //store values from head
        x = queue->x;
        y = queue->y;

        //update entity with new point
        entity->mass += 1;
        if (x < entity->minx) {
            entity->minx = x;
        } else if (x > entity->maxx) {
            entity->maxx = x;
        }
        if (y < entity->miny) {
            entity->miny = y;
        } else if (y > entity->maxy) {
            entity->maxy = y;
        }
        
        //pop
        queue = pop_point(queue);
        
        //tag current position
        set_pixel(segmap, x, y, tag);
        
        //add children
        //right
        if (x < segmap->image_header->width-1 &&
            pixels_match(target, get_pixel(segmap, x+1, y))) {
            queue = add_point(queue, x+1, y);
        }
        //down
        if (y < segmap->image_header->height-1 &&
            pixels_match(target, get_pixel(segmap, x, y+1))) {
            queue = add_point(queue, x, y+1);
        }
        //left
        if (x > 0 &&
            pixels_match(target, get_pixel(segmap, x-1, y))) {
            queue = add_point(queue, x-1, y);
        }
        //up
        if (y > 0 &&
            pixels_match(target, get_pixel(segmap, x, y-1))) {
            queue = add_point(queue, x, y-1);
        }
    }
    
    return;
}

//searches segmap for 'entities' and returns list of those that passed filter
//entities that fail to pass filter are blacked out of segmap
//if tag_segmap 1, entities will be tagged with pixels as their id values
//if tag_segmap 1, note there is a max id range of 0-255 for pixel tagging
struct EntityList *filter_entities(struct BMP *segmap,
                                   struct EntityFilter filter,
                                   int tag_segmap) {
    struct EntityList *tmp;
    struct Pixel p;
    struct Entity *new_entity;
    struct EntityList *elist;
    unsigned char id;
    int x, y;
    
    x = y = 0;
    id = 1;
    
    elist = NULL;
    
    //iterate through pixel data to initially tag
    for (y = 0; y < segmap->image_header->height; y++) {
        for (x = 0; x < segmap->image_header->width; x++) {
            //check for foreground pixel
            if (is_foreground(get_pixel(segmap, x, y))) {
                //make Entity struct
                new_entity = init_entity(id, x, y);
                
                //tag new entity
                tag_entity(segmap, new_entity);
                
                //check if entity should be filtered
                if (!passes_filter(new_entity, filter)) {
                    new_entity->id = 0;  //retag with 0 (blackout)
                    new_entity->mass = 0;
                    new_entity->minx = new_entity->maxx = x;
                    new_entity->miny = new_entity->maxy = y;
                    target_tag_entity(segmap, new_entity, make_pixel(id,id,id));
                } else {
                    //add to list
                    elist = add_entity(elist, new_entity); 
                    id++;
                }
            }
        }
    }
    if (!tag_segmap) {
        //for each pixel in segmap
        for (y = 0; y < segmap->image_header->height; y++) {
            for (x = 0; x < segmap->image_header->width; x++) {
                //get pixel
                p = get_pixel(segmap, x, y);
                //if 0,0,0 then do next pixel
                if (is_background(p)) {
                    continue;
                }
                
                //iterate through filtered list
                tmp = elist;
                
                while (tmp != NULL) {
                    //if this pixel is tagged as this entity, black out the pixel on segmap
                    if (p.red == tmp->entity->id &&
                        p.green == tmp->entity->id &&
                        p.blue == tmp->entity->id) {
                        set_pixel(segmap, x, y, make_pixel(255,255,255));
                        break;
                    }
                    tmp = tmp->next;
                }
            }
        }
    }
    return elist;
}

//filters entities from the segmap and tagged_segmap
//returns list of remaining entities
//filtered entities are 'blacked out' from both segmap and tagged_segmap
struct EntityList *old_filter_entities(struct BMP *segmap,
                                       struct BMP *tagged_segmap,
                                       struct EntityList *entities,
                                       struct EntityFilter filter) {
    struct EntityList *tmp, *filtered, *passed;
    struct Pixel p;
    int x, y;
    
    filtered = passed = NULL;
    
    tmp = entities;
    
    //iterate through entity list to collect seperated list
    while (tmp != NULL) {
        //if entity passes filter, add to passed list
        if (passes_filter(tmp->entity, filter)) {
            printf("passed %d\n", tmp->entity->id);
            passed = add_entity(passed, tmp->entity);
        //else add to filtered list for later use
        } else {
            printf("filtered %d\n", tmp->entity->id);
            filtered = add_entity(filtered, tmp->entity);
        }
        tmp = tmp->next;
    }
    
    //for each pixel in segmap
    for (y = 0; y < segmap->image_header->height; y++) {
        for (x = 0; x < segmap->image_header->width; x++) {
            //get pixel
            p = get_pixel(tagged_segmap, x, y);
            //if 0,0,0 then do next pixel
            if (is_background(p)) {
                continue;
            }
            
            //iterate through filtered list
            tmp = filtered;
            
            while (tmp != NULL) {
                //if this pixel is tagged as this entity, black out the pixel on segmap
                if (p.red == tmp->entity->id &&
                    p.green == tmp->entity->id &&
                    p.blue == tmp->entity->id) {
                    set_pixel(segmap, x, y, make_pixel(0,0,0));
                    set_pixel(tagged_segmap, x, y, make_pixel(0,0,0));
                    break;
                }
                tmp = tmp->next;
            }
        }
    }
    
    //return list of entities that remain
    return passed;
}

//returns 1 if the given entity does not violate any filter rules
int passes_filter(struct Entity *e,
                  struct EntityFilter f) {
    int width = (e->maxx - e->minx)+1;
    int height = (e->maxy - e->miny)+1;
    return ((f.min_mass == -1 || e->mass >= f.min_mass) &&
            (f.max_mass == -1 || e->mass <= f.max_mass) &&
            (f.min_width == -1 || width >= f.min_width) &&
            (f.max_width == -1 || width <= f.max_width) &&
            (f.min_height == -1 || height >= f.min_height) &&
            (f.max_height == -1 || height <= f.max_height));
}

//----------------------
//struct based functions
//----------------------

//inits a new entity struct with the given id and at the given point
struct Entity *init_entity(int id, 
                           int x, 
                           int y) {
    struct Entity *e;
    e = malloc(sizeof(struct Entity));
    
    if (!e) {
        return e;
    }
    
    e->id = id;
    e->mass = 0;
    e->minx = e->maxx = x;
    e->miny = e->maxy = y;
    return e;
}

//creates a filter, -1 means ignore
struct EntityFilter init_filter(int min_mass,
                                int max_mass,
                                int min_width,
                                int max_width,
                                int min_height,
                                int max_height) {
    struct EntityFilter ef;
    ef.min_mass = min_mass;
    ef.max_mass = max_mass;
    ef.min_width = min_width;
    ef.max_width = max_width;
    ef.min_height = min_height;
    ef.max_height = max_height;
    
    return ef;
}

//returns a filter struct made from config settings
struct EntityFilter get_config_filter(struct SysConfig *conf) {
    return init_filter(conf->ent_min_mass, conf->ent_max_mass,
                       conf->ent_min_width, conf->ent_max_width,
                       conf->ent_min_height, conf->ent_max_height);
}

//creates an entity list with the given entity
struct EntityList *init_entity_list(struct Entity *entity) {
    struct EntityList *el;
    el = malloc(sizeof(struct EntityList));
    
    if (!el)
        return NULL;
    
    el->entity = entity;
    el->next = NULL;
    return el;
}

//adds the given entity to the given entity list
//will create new list if head == NULL
//returns ptr to new list
struct EntityList *add_entity(struct EntityList *head,
                              struct Entity *entity) {
    struct EntityList *el, *tmp;
    
    //if list empty init new list and return, else fail
    if (!head) {
        if (!(el = init_entity_list(entity))) {
            return NULL;
        } else {
            return el;
        }
    } else {
        el = head;
    }
    
    //store new head (or original head if it existed as el = head)
    tmp = el;
    
    //seek to end of list
    while(tmp->next != NULL) {
        tmp = tmp->next;
    }
    
    tmp->next = init_entity_list(entity);
    if (!el->next) {
        return NULL;
    }
    return el;
}

//initialises a point list node with the given values
struct PointList *init_point_list(int x,
                                  int y) {
    struct PointList *pl;
    pl = malloc(sizeof(struct PointList));
    if (!pl)
        return NULL;
    pl->x = x;
    pl->y = y;
    pl->next = NULL;
    return pl;
}

//adds a new node at the end of the given list, provided new node is not found inlist
struct PointList *add_point(struct PointList *head,
                            int x,
                            int y) {
    struct PointList *pl, *tmp;
    
    //if list empty init new list and return, else fail
    if (!head) {
        if (!(pl = init_point_list(x, y))) {
            return NULL;
        } else {
            return pl;
        }
    } else {
        pl = head;
    }
    
    tmp = pl;
    
    //seek to end of list
    while (tmp->next != NULL) {
        //check for duplicates
        if (tmp->x == x && tmp->y == y) {
            return pl;
        }
        tmp = tmp->next;
    }
    //check for duplicates
    if (tmp->x == x && tmp->y == y) {
        return pl;
    }
    tmp->next = init_point_list(x, y);
    if (!tmp->next) {
        return NULL;
    }
    return pl;
}

//removes the head of the list and returns the tail
struct PointList *pop_point(struct PointList *list) {
    struct PointList *tl;
    
    if (!list)
        return NULL;

    tl = list->next;
    free(list);
    
    return tl;
}

//print entity list
void print_entity_list(struct EntityList *el) {
    struct EntityList *tmp;
    tmp = el;
    while (tmp != NULL) {
        print_entity(tmp->entity);
        tmp = tmp->next;
    }
}

//print entity details
void print_entity(struct Entity *e) {
    printf("Entity %d\nMass: %d\nx: %d - %d (%d)\ny: %d - %d (%d)\n", 
           e->id, e->mass, 
           e->minx, e->maxx,(e->maxx - e->minx)+1,
           e->miny, e->maxy,(e->maxy - e->miny)+1);
}

//print a point list
void print_point_list(struct PointList *pl) {
    int i = 0;
    struct PointList *tmp;
    tmp = pl;
    while (tmp != NULL) {
        printf("%d: %d, %d\n", i++, tmp->x, tmp->y);
        tmp = tmp->next;
    }
}

//printf filter
void print_filter(struct EntityFilter ef) {
    printf("Entity Filter\nmin_mass %d\nmax_mass %d\nmin_width %d\nmax_width %d\nmin_height %d\nmax_height %d\n",
           ef.min_mass, ef.max_mass, ef.min_width, ef.max_width, ef.min_height, ef.max_height);
}
