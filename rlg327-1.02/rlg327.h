#ifndef RLG327_H
# define RLG327_H

# ifdef __cplusplus
extern "C" {
# endif

#define DUNGEON_X   80
#define DUNGEON_Y   21
#define mapxy(x, y) (d->map[y][x])
#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
#define hardnessxy(x, y) (d->hardness[y][x])

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

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
} room_t;

typedef struct dungeon {
  uint32_t num_rooms;
  room_t *rooms;
  terrain_type_t map[DUNGEON_Y][DUNGEON_X];          
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
  char nonTunnel[DUNGEON_Y][DUNGEON_X];
  char tunnel[DUNGEON_Y][DUNGEON_X];
  pair_t PC;
} dungeon_t;


# ifdef __cplusplus
}
# endif

#endif