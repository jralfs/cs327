#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "binheap.h"
#include "rlg327.h"
#include "path.h"

#define INFINITE 255
#define px(p) (p->pos[dim_x])
#define py(p) (p->pos[dim_y])


typedef struct vertex {
  binheap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  uint32_t cost; 
}vertex_t;

void print_binheap(binheap_t *h){
  int i = 0;
  for (i = 0; i < h->size; i++){
    binheap_node_t *n = h->array[i];
    vertex_t *v = n->datum;
    printf("X: %d, Y %d\n", v->pos[dim_x], v->pos[dim_y]);
  }
}

void data_delete(void *v){
  free(v);
}

int32_t compare_data(const void *key, const void *with){
    const vertex_t *vKey = key;
    const vertex_t *wKey = with;
    return vKey->cost - wKey->cost;
 }

void dijkstra_nontunneling(dungeon_t *d)
{
  pair_t from;
  from[dim_x] = d->PC[dim_x];
  from[dim_y] = d->PC[dim_y];
  
  static vertex_t path[DUNGEON_Y][DUNGEON_X];
  static vertex_t *p;
  static uint32_t initialized = 0;
  binheap_t h;
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
	    path[y][x].cost = INFINITE;
	  }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  binheap_init(&h, compare_data, data_delete);

  for (y = 0; y < DUNGEON_Y; y++) {
  	for (x = 0; x < DUNGEON_X; x++) {
    	if (mapxy(x, y) != ter_wall_immutable) {
      		path[y][x].hn = binheap_insert(&h, &path[y][x]);
    	} else {
     		path[y][x].hn = NULL;
    	}
  	}
  }



  while ((p = binheap_remove_min(&h))) {
	  p->hn = NULL;
	  vertex_t *w;

	  w = &path[px(p) - 1][py(p)];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)][py(p)-1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)][py(p)+1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)+1][py(p)];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)+1][py(p)-1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)-1][py(p)-1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)-1][py(p)+1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

	  w = &path[px(p)+1][py(p)+1];
	  if((w->hn) && (w->cost > p->cost) && (hardnessxy(px(p) - 1, py(p)) == 0)){
	    w->from[dim_x] = px(p);
	    w->from[dim_y] = py(p);
	    w->cost = p->cost + 1;
	    binheap_decrease_key(&h, w->hn);
	  }

  }

/*  char cost[62]={'0','1','2','3','4','5','6','7','8','9',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
    'o','p','q','r','s','t','u','v','w','x','y','z',
    'A','B','C','D','E','F','G','H','I','J','K','L',
    'M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
	
  for (y = 0; y < DUNGEON_Y; y++) {
  	for (x = 0; x < DUNGEON_X; x++) {
  		printf("%c", cost[path[y][x].cost]);
  	}
  	printf("\n");
  }*/
}