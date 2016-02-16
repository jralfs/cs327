#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "binheap.h"
#include "rlg327.h"
#include "path.h"

typedef struct vertex {
	pair_t position;
	uint32_t hardness;
	uint32_t distance; 
}vertex_t;

typedef enum dir {
	right,
	bot_right,
	bot,
	bot_left,
	left,
	top_left,
	top,
	top_right,
  num_dir
} dir_t;

typedef binheap_node_t* neighbors[num_dir];

void print_binheap(binheap_t *h){
  int i = 0;
  for (i = 0; i < h->size; i++){
    binheap_node_t *n = h->array[i];
    vertex_t *v = n->datum;
    printf("X: %d, Y %d\n", v->position[dim_x], v->position[dim_y]);
  }
}


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

void get_neighbors(vertex_t *v,   
    binheap_node_t* arr[DUNGEON_Y][DUNGEON_X], 
      neighbors n){
  //Right 
  if(v->position[dim_x] > 0 && v->position[dim_x] < 78){
    n[right] = arr[v->position[dim_y]][v->position[dim_x] + 1];
    //Bot Right
    if(v->position[dim_y] < 18){
      n[bot_right] = arr[v->position[dim_y] + 1][v->position[dim_x] + 1];
    }
    //Top Right
    if(v->position[dim_y] > 1){
      n[top_right] = arr[v->position[dim_y] - 1][v->position[dim_x] + 1];
    }
  }
  //Left
  if(v->position[dim_x] > 1 && v->position[dim_x < 79]){
    n[left] = arr[v->position[dim_y]][v->position[dim_x]-1];
    //Bot Left
    if(v->position[dim_y] < 18){
      n[bot_left] = arr[v->position[dim_y] + 1][v->position[dim_x] - 1];
    }
    //Top Left
    if(v->position[dim_y] > 1){
      n[top_right] = arr[v->position[dim_y] - 1][v->position[dim_x] - 1];
    }
  }
  //Top
  if(v->position[dim_y] > 1){
    n[top] = arr[v->position[dim_y] - 1][v->position[dim_x]];
  }
  //Bottom
  if(v->position[dim_y] < 18){
    n[bot] = arr[v->position[dim_y] + 1][v->position[dim_x]];
  }
}

int calc_dist(vertex_t *v){
  if(v != NULL){
    if(v->hardness >= 1 && v->hardness <= 84){
      return 1;
    }
    else if (v->hardness >= 84 && v->hardness <= 170){
      return 2;
    }    
    else if (v->hardness >= 171 && v->hardness <= 254){
      return 3;
    }
    else{
      printf("hardness is 255, don't put in queue\n");
      printf("x: %d, y: %d\n", v->position[dim_x], v->position[dim_y]);
      return 0;
    }
  }
  else {
    printf("Node was null\n");
    return 0;
  }
}

void init_dijkstra_tunnel(dungeon_t *d, binheap_t *h, 	
	binheap_node_t* arr[DUNGEON_Y][DUNGEON_X]){
	int x, y;
	for (y = 0; y < DUNGEON_Y; y++) {
		for (x = 0; x < DUNGEON_X; x++) {
			if(x == d->PC[dim_x] && y == d->PC[dim_y]){
				arr[y][x] = binheap_insert(h, create_vertex(d, x, y, 0));
				d->tunnel[y][x] = '0';
    		}
    		else if(d->hardness[y][x] != 255) {
      		arr[y][x] = binheap_insert(h, create_vertex(d, x, y, 255));
    			d->tunnel[y][x] = 'Z';
    		}
    		else {
    			d->tunnel[y][x] = ' ';
    		}
		}
	}
}

void dijkstra_tunneling(dungeon_t *d){
  int i = 0;
 	binheap_t *h = malloc(sizeof(*h));
	binheap_init(h, compare_data, data_delete);
	binheap_node_t* arr[DUNGEON_Y][DUNGEON_X];
	init_dijkstra_tunnel(d, h, arr);

	while(!binheap_is_empty(h)){
    neighbors n;
		vertex_t *u = binheap_remove_min(h);
		get_neighbors(u, arr, n);

    for (i = 0; i < num_dir; i++){
      if(n[i] != NULL){
        vertex_t *v = (vertex_t *)n[i]->datum;
        int alt = u->distance + calc_dist(v);

        if (alt < v->distance){
          v->distance = alt;
        }
      }
    }

	}
	binheap_delete(h);
}
