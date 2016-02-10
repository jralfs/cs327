#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include "heap.h"
#include <endian.h>

/* Returns true if random float in [0,1] is less than *
 * numerator/denominator.  Uses only integer math.    */
# define rand_under(numerator, denominator) \
  (rand() < ((RAND_MAX / denominator) * numerator))

/* Returns random integer in [min, max]. */
# define rand_range(min, max) ((rand() % (((max) + 1) - (min))) + (min))

typedef struct corridor_path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  /* Because paths can meander about the dungeon, they can be *
   * significantly longer than DUNGEON_X.                     */
  int32_t cost;
} corridor_path_t;

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

#define DUNGEON_X              80
#define DUNGEON_Y              21
#define MIN_ROOMS              5
#define MAX_ROOMS              9
#define ROOM_MIN_X             4
#define ROOM_MIN_Y             2
#define ROOM_MAX_X             14
#define ROOM_MAX_Y             8

#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (d->map[y][x])
#define hardnesspair(pair) (d->hardness[pair[dim_y]][pair[dim_x]])
#define hardnessxy(x, y) (d->hardness[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_debug,
  ter_wall,
  ter_wall_immutable,
  ter_floor,
  ter_floor_room,
  ter_floor_hall,
} terrain_type_t;

typedef struct room {
  pair_t position;
  pair_t size;
  uint32_t connected;
  uint8_t exists;
} room_t;

typedef struct dungeon {
  uint32_t num_rooms;
  room_t rooms[MAX_ROOMS];
  terrain_type_t map[DUNGEON_Y][DUNGEON_X];
  /* Since hardness is usually not used, it would be expensive to pull it *
   * into cache every time we need a map cell, so we store it in a        *
   * parallel array, rather than using a structure to represent the       *
   * cells.  We may want a cell structure later, but from a performanace  *
   * perspective, it would be a bad idea to ever have the map be part of  *
   * that structure.  Pathfinding will require efficient use of the map,  *
   * and pulling in unnecessary data with each map cell would add a lot   *
   * of overhead to the memory system.                                    */
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
} dungeon_t;

static int32_t corridor_path_cmp(const void *key, const void *with) {
  return ((corridor_path_t *) key)->cost - ((corridor_path_t *) with)->cost;
}

/* This is the same basic algorithm as in move.c, but *
 * subtle differences make it difficult to reuse.     */
static void dijkstra_corridor(dungeon_t *d, pair_t from, pair_t to)
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(x, y) != ter_wall_immutable) {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      } else {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        if (mapxy(x, y) != ter_floor_room) {
          mapxy(x, y) = ter_floor_hall;
          hardnessxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }
    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_x] != from[dim_x]) ? 48  : 0))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_x] != from[dim_x]) ? 48  : 0);
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_y] != from[dim_y]) ? 48  : 0))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost =
        p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_y] != from[dim_y]) ? 48  : 0);
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_y] != from[dim_y]) ? 48  : 0))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost =
        p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_y] != from[dim_y]) ? 48  : 0);
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_x] != from[dim_x]) ? 48  : 0))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        p->cost + (hardnesspair(p->pos) ? hardnesspair(p->pos) : 8) +
         ((p->pos[dim_x] != from[dim_x]) ? 48  : 0);
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

/* Chooses a random point inside each room and connects them with a *
 * corridor.  Random internal points prevent corridors from exiting *
 * rooms in predictable locations.                                  */
static int connect_two_rooms(dungeon_t *d, room_t *r1, room_t *r2)
{
  pair_t e1, e2;

  e1[dim_y] = rand_range(r1->position[dim_y],
                         r1->position[dim_y] + r1->size[dim_y] - 1);
  e1[dim_x] = rand_range(r1->position[dim_x],
                         r1->position[dim_x] + r1->size[dim_x] - 1);
  e2[dim_y] = rand_range(r2->position[dim_y],
                         r2->position[dim_y] + r2->size[dim_y] - 1);
  e2[dim_x] = rand_range(r2->position[dim_x],
                         r2->position[dim_x] + r2->size[dim_x] - 1);

  /*  return connect_two_points_recursive(d, e1, e2);*/
  dijkstra_corridor(d, e1, e2);
  return 0;
}

static int connect_rooms(dungeon_t *d)
{
  uint32_t i;

  for (i = 1; i < d->num_rooms; i++) {
    connect_two_rooms(d, d->rooms + i - 1, d->rooms + i);
  }

  return 0;
}

static int empty_dungeon(dungeon_t *d)
{
  uint8_t x, y;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      mapxy(x, y) = ter_wall;
      hardnessxy(x, y) = rand_range(1, 254);
      if (y == 0 || y == DUNGEON_Y - 1 ||
          x == 0 || x == DUNGEON_X - 1) {
        mapxy(x, y) = ter_wall_immutable;
        hardnessxy(x, y) = 255;
      }
    }
  }

  return 0;
}

static int place_rooms(dungeon_t *d)
{
  pair_t p;
  uint32_t i;
  int success;
  room_t *r;

  for (success = 0; !success; ) {
    success = 1;
    for (i = 0; success && i < d->num_rooms; i++) {
      r = d->rooms + i;
      r->position[dim_x] = 1 + rand() % (DUNGEON_X - 2 - r->size[dim_x]);
      r->position[dim_y] = 1 + rand() % (DUNGEON_Y - 2 - r->size[dim_y]);
      for (p[dim_y] = r->position[dim_y] - 1;
           success && p[dim_y] < r->position[dim_y] + r->size[dim_y] + 1;
           p[dim_y]++) {
        for (p[dim_x] = r->position[dim_x] - 1;
             success && p[dim_x] < r->position[dim_x] + r->size[dim_x] + 1;
             p[dim_x]++) {
          if (mappair(p) >= ter_floor) {
            success = 0;
            empty_dungeon(d);
          } else if ((p[dim_y] != r->position[dim_y] - 1)              &&
                     (p[dim_y] != r->position[dim_y] + r->size[dim_y]) &&
                     (p[dim_x] != r->position[dim_x] - 1)              &&
                     (p[dim_x] != r->position[dim_x] + r->size[dim_x])) {
            mappair(p) = ter_floor_room;
            hardnesspair(p) = 0;
          }
        }
      }
    }
  }

  return 0;
}

static int make_rooms(dungeon_t *d)
{
  uint32_t i;

  memset(d->rooms, 0, sizeof (d->rooms));

  for (i = 0; i < MIN_ROOMS; i++) {
    d->rooms[i].exists = 1;
  }
  for (; i < MAX_ROOMS && rand_under(6, 8); i++) {
    d->rooms[i].exists = 1;
  }
  d->num_rooms = i;

  for (; i < MAX_ROOMS; i++) {
    d->rooms[i].exists = 0;
  }

  for (i = 0; i < d->num_rooms; i++) {
    d->rooms[i].size[dim_x] = ROOM_MIN_X;
    d->rooms[i].size[dim_y] = ROOM_MIN_Y;
    while (rand_under(3, 4) && d->rooms[i].size[dim_x] < ROOM_MAX_X) {
      d->rooms[i].size[dim_x]++;
    }
    while (rand_under(3, 4) && d->rooms[i].size[dim_y] < ROOM_MAX_Y) {
      d->rooms[i].size[dim_y]++;
    }
    /* Initially, every room is connected only to itself. */
    d->rooms[i].connected = i;
  }

  return 0;
}

int gen_dungeon(dungeon_t *d)
{
  /*
  pair_t p1, p2;

  p1[dim_x] = rand_range(1, 158);
  p1[dim_y] = rand_range(1, 94);
  p2[dim_x] = rand_range(1, 158);
  p2[dim_y] = rand_range(1, 94);
  */

  empty_dungeon(d);

  /*
  connect_two_points_recursive(d, p1, p2);
  return 0;
  */

  do {
    make_rooms(d);
  } while (place_rooms(d));
  connect_rooms(d);

  return 0;
}

void render_dungeon(dungeon_t *d)
{
  pair_t p;

  for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
    for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
      switch (mappair(p)) {
      case ter_wall:
      case ter_wall_immutable:
        putchar(' ');
        break;
      case ter_floor:
      case ter_floor_room:
        putchar('.');
        break;
      case ter_floor_hall:
        putchar('#');
        break;
      case ter_debug:
        putchar('*');
        break;
      }
    }
    putchar('\n');
  }
}

void delete_dungeon(dungeon_t *d)
{
}

void init_dungeon(dungeon_t *d)
{
  empty_dungeon(d);
}

int save(FILE* f, char name[], dungeon_t *d, int loaded){
    //printf("\nSaving\n");
    int i, j;
    f = fopen(name, "w");
    //Open file
    if (!f){
        fprintf(stderr, "Couldn't open write file\n");
    }
    //Write file type Marker
    char *fileName = "RLG327";
    if(fwrite(fileName, 5, 1, f) == 0){
        printf("Couldnt write File-Type Marker to file.\n");
        return -1;
    }
    //Write version
    uint32_t version = 0;
    version = htobe32(version);
    if (fwrite(&version, 4, 1, f) == 0){
        printf("Couldnt write version to file.\n");
        return -1;
    }
    
    uint32_t size = 0;
    //Write size
    if (loaded){
        int count = 0;
        for (i = 1; i < 20; i++){
            for (j = 1; j < 79; j++){
                if (mapxy(j,i) == ter_floor_room){
                    count++;
                }
            }
        }
        size = 1496 + 4*count;
        size = htobe32(size);
    }
    else {
        size = 1496 + 4*d->num_rooms;
        size = htobe32(size);
    }
    if (fwrite(&size, 4, 1, f) == 0){
        printf("Couldnt write size to file.\n");
        return -1;
    }
    //Write hardness per cell
    for (i = 1; i < 20; i++){
        for (j = 1; j < 79; j++){
            //printf("X: %d, Y: %d, %d\n", j, i, hardnessxy(j,i));
            if(fwrite(&hardnessxy(j,i), 1, 1, f) == 0){
                //printf("Couldnt write hardness to file.\n");
                return -1;
            }
        }
    }
    if(loaded){
        for (i = 1; i < 20; i++){
            for (j = 1; j < 79; j++){
                if (mapxy(j,i) == ter_floor_room){
                    uint8_t x_pos = j;
                    uint8_t y_pos = i;
                    uint8_t width = 1;
                    uint8_t height = 1;
                    fwrite(&x_pos, 1, 1, f);
                    fwrite(&y_pos, 1, 1, f);
                    fwrite(&width, 1, 1, f);
                    fwrite(&height, 1, 1, f);
                }
            }
        }
    }else {
        //Write room location
        for (i = 0; i < d->num_rooms; i++){
            room_t *r;
            r = &(d->rooms[i]);
            if (fwrite(&(r->position[dim_x]), 1, 1, f) == 0){
                printf("Couldn't write x position of room to file.\n");
                return -1;
            }
            if (fwrite(&(r->position[dim_y]), 1, 1, f) == 0){
                printf("Couldn't write y position of room to file.\n");
                return -1;
            }
            if (fwrite(&(r->size[dim_x]), 1, 1, f) == 0){
                printf("Couldn't write width of room to file.\n");
                return -1;
            }
            if (fwrite(&(r->size[dim_y]), 1, 1, f) == 0){
                printf("Couldn't write height of room to file.\n");
                return -1;
            }
        }
    }
    return 0;
}

int read(FILE* f, dungeon_t *d){
    //printf("\nLoading\n");
    init_dungeon(d);
    char c = '\0';
    char fileType[6];
    uint8_t rowMajorDungeon[1482];
    int i = 0;
    int j = 0;
    int k = 0;
    
    //Read First Five Bits
    for (i = 0; i < 6; i++){
        fread(&c, 1, 1, f);
        fileType[i] = c;
    }
    
    //if (strcmp(fileType, "RLG327") != 0){
    //    fprintf(stderr, "Wrong file-type marker\n");
    //    return -1;
    //}

    //Read version
    uint32_t version;
    fread(&version, 4, 1, f);
    version = be32toh(version);
    
    //Read file size
    uint32_t size;
    fread(&size, 4, 1, f);
    size = be32toh(size);
    
    //Reading row major dungeon hardness per cell
    for (i = 14; i < 1496; i++){
        fread(&rowMajorDungeon[j], 1, 1, f);
        //printf("J: %d, %d\n",j, rowMajorDungeon[j]);
        j++;
    }

    
    //Add hardness to the dungeon from the row-major array
    for (i = 1; i < 20; i++){
        for (j = 1; j < 79; j++){
            hardnessxy(j,i) = rowMajorDungeon[k];
            //printf("X: %d, Y: %d, K: %d, %d\n", j, i, k, hardnessxy(j,i));
            if (hardnessxy(j,i) == 0){
                mapxy(j, i) = ter_floor_hall;
            } else {
                mapxy(j, i) = ter_wall;
            }
            k++;
        }
    }
    
    uint8_t x_pos, y_pos, width, height;
    int numRoomsFile = (size - 1496) / 4;
    for (i = 0; i < numRoomsFile; i++){
        fread(&x_pos, 1, 1, f);
        fread(&y_pos, 1, 1, f);
        fread(&width, 1, 1, f);
        fread(&height, 1, 1, f);
        for(j = y_pos; j < y_pos+height+1; j++){
            for(k = x_pos; k < x_pos+width+1; k++){
                mapxy(k,j) = ter_floor_room;
            }
        }
        //printf("X: %d, Y: %d, W: %d, H: %d\n", x_pos, y_pos, width, height);
    }
    
    render_dungeon(d);

    
    return 0;
}

int main(int argc, char *argv[])
{
    dungeon_t d;
    struct timeval tv;
    uint32_t seed;
    FILE *f = NULL;
    
    if (argc >= 2){
        if(argv[1][0] != '-' || argv[1][1] != '-'){
            fprintf(stderr, "Bad argument format\n");
            return -1;
        }else {
            //Check for save first
            if(strcmp(argv[1], "--save") == 0){
                //Check for extra arguments
                if(argc > 2){
                    if(strcmp(argv[2], "--load") != 0){
                        fprintf(stderr, "Bad argument format\n");
                        return -1;
                    }else {
                        char *name = getenv("HOME");
                        char *directory = "/.rlg327/dungeon";
                        char *filePath = strcat(name, directory);
                        if (argc < 4){
                            f = fopen(filePath, "r");
                            
                            if (!f){
                                fprintf(stderr, "Couldnt open file\n");
                                return -1;
                            }
                            int readResult = read(f, &d);
                            if (readResult != 0){
                                return -1;
                            }
                            int saveResult = save(f, filePath, &d, 1);
                            if(saveResult != 0){
                                return -1;
                            }
                        } else {
                            f = fopen(argv[3], "r");
                            if (!f){
                                fprintf(stderr, "Couldnt open file\n");
                                return -1;
                            }
                            int readResult = read(f, &d);
                            if (readResult != 0){
                                return -1;
                            }
                            int saveResult = save(f, filePath, &d, 1);
                            if(saveResult == -1){
                                return -1;
                            }
                        }
                    }
                }
                
                //Just Save
                else {
                    char *name = getenv("HOME");
                    char *directory = "/.rlg327/dungeon";
                    char *filePath = strcat(name, directory);
                    gettimeofday(&tv, NULL);
                    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
                    
                    printf("Using seed: %u\n", seed);
                    srand(seed);
                    
                    init_dungeon(&d);
                    gen_dungeon(&d);
                    render_dungeon(&d);
                    delete_dungeon(&d);
                    
                    int saveResult = save(f, filePath, &d, 0);
                    if(saveResult == -1){
                        return -1;
                    }
                }
            //Check for load first
            } else if (strcmp(argv[1], "--load") == 0){
                char *name = getenv("HOME");
                char *directory = "/.rlg327/dungeon";
                char *filePath = strcat(name, directory);
                
                //Check for load file name
                if(argc < 3){
                    f = fopen(filePath, "r");
                    
                    if (!f){
                        fprintf(stderr, "Couldnt open file\n");
                        return -1;
                    }
                    
                    int readResult = read(f, &d);
                    if (readResult != 0){
                        return -1;
                    }
                } else {
                    //Check for load file
                    f = fopen(argv[2], "r");
                    if (!f){
                        fprintf(stderr, "Couldnt open file\n");
                        return -2;
                    }
                    
                    int readResult = read(f, &d);
                    if (readResult != 0){
                        return -1;
                    }
                }
                //Check for extra arguments
                if(argc > 3){
                    //Check for save
                    if(strcmp(argv[3],"--save") != 0){
                        fprintf(stderr, "Bad argument format\n");
                        return -1;
                    }else {
                        char *c = "AnnieDungeon.rlg327";
                        int saveResult = save(f, c, &d, 1);
                        if(saveResult == -1){
                            return -1;
                        }
                    }
                }
            }
        }
    }else {
        gettimeofday(&tv, NULL);
        seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;

        printf("Using seed: %u\n", seed);
        srand(seed);

        init_dungeon(&d);
        gen_dungeon(&d);
        render_dungeon(&d);
        delete_dungeon(&d);
    }

    fclose(f);
  return 0;
}
