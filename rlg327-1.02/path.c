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

vertex_t *create_vertex(dungeon_t *d, int x, int y, uint32_t distance){
	vertex_t *v = malloc(sizeof(*v));
	v->position[dim_y] = y;
	v->position[dim_x] = x; 
	v->hardness = d->hardness[y][x];
	v->distance = distance;
	return v;
}

vertex_t *get_neightbors(vertex_t *v){
	//vertex_t *array[8];
	return NULL;
}

void print_heap(binheap_t *h){
	int i = 0;
	for (i = 0; i < h->size; i++){
		binheap_node_t *n = h->array[i];
		vertex_t *v = n->datum;
		printf("X: %d, Y %d\n", v->position[dim_x], v->position[dim_y]);
	}
}

void init_dijkstra_tunnel(dungeon_t *d, binheap_t *h){
	int x, y;
	for (y = 0; y < DUNGEON_Y; y++) {
		for (x = 0; x < DUNGEON_X; x++) {
			if(x == d->PC[dim_x] && y == d->PC[dim_x]){
				binheap_insert(h, create_vertex(d, x, y, 0));
				d->tunnel[y][x] = '@';
    		}
    		else if(d->map[y][x] != ter_wall_immutable) {
      			binheap_insert(h, create_vertex(d, x, y, 255));
    			d->tunnel[y][x] = 'Z';
    		}
    		else {
    			d->tunnel[y][x] = ' ';
    		}
		}
	}
}

void dijkstra_tunneling(dungeon_t *d){
 	binheap_t *h =  malloc(sizeof(*h));
	binheap_init(h, compare_data, data_delete);
	init_dijkstra_tunnel(d, h);
	
	while(!binheap_is_empty(h)){
		vertex_t *v = binheap_remove_min(h);
		get_neightbors(v);

	}

	binheap_delete(h);
}
