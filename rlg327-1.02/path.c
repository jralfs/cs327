#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "binheap.h"
#include "rlg327.h"

typedef struct vertex {
	pair_t position;
	uint32_t hardness;
	uint32_t distance; 
}vertex_t;


int32_t compare_data(const void *key, const void *with){
  	const vertex_t *vKey = key;
  	const vertex_t *wKey = with;
  	return vKey->distance - wKey->distance;
 }

void data_delete(void *v){
	free(v);
}

vertex_t *create_vertex(dungeon_t *d, int x, int y){
	vertex_t *v = malloc(sizeof(*v));
	v->position[dim_y] = y;
	v->position[dim_x] = x; 
	v->hardness = d->hardness[y][x];
	v->distance = 255;
	d->tunnel[y][x] = 'Z';
	return v;
}

void dijkstra_tunneling(dungeon_t *d){
	int x, y;
 	binheap_t *h =  malloc(sizeof(*h));
	binheap_init(h, compare_data, data_delete);

	for (y = 0; y < DUNGEON_Y; y++) {
    	for (x = 0; x < DUNGEON_X; x++) {
    		if(x == d->PC[dim_x] && y == d->PC[dim_x]){
    			d->tunnel[y][x] = '@';
        	}
        	else {
          		binheap_insert(h, create_vertex(d, x, y));
        	}
 		}
	}

  binheap_delete(h);
}
