#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "binheap.h"

#define DUNGEON_X              80
#define DUNGEON_Y              21

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

typedef struct vertex {
	pair_t position;
	uint32_t hardness;
	uint32_t distance; 
}vertex_t;

pair_t findPC(dungeon_t *d){
	return NULL;
}

void dijkstra_nontunneling(dungeon_t *d){
	binheap_t *h;
	binheap_init(h, compare_int(const void *key, const void *with), binheap_delete(binheap_t *h));

	pair_t *pc = findPC(d);

	for (y = 0; y < DUNGEON_Y; y++) {
    	for (x = 0; x < DUNGEON_X; x++) {
    		if(x != &pc[dim_x] && y != &pc[dim_y]){}
    			vertex *v;
    			v->position[dim_y] = y;
    			v->position[dim_x] = x; 
    			v->hardness = d->hardness[y][x];
    			v->distance = 255;

    			binheap_insert(h, v);
    		}
 		}
	}

	

}
