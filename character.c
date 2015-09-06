#include <stdlib.h>
#include "binheap.h"
#include "pair.h"
#include "npc.h"
#include "pc.h"
#include "dungeon.h"
#include "character.h"

void character_delete(void *v)
{
  character_t *c;

    if(v){
      c = v;

      if (c->npc) {
        npc_delete(c->npc);
        free(c);
      }
    }
}

int32_t compare_characters_by_next_turn(const void *character1,
                                        const void *character2)
{
	int turn_diff = (((character_t *) character1)->next_turn - ((character_t *) character2)->next_turn);
	int rank_diff = (((character_t *) character1)->rank - ((character_t *) character2)->rank);
    return (turn_diff!=0) ? turn_diff : rank_diff;
}

//if the first can see the second
uint32_t can_see(_game *g, character_t *first, character_t *second){
	//in the same room
	if((g->dungeon[first->position.y][first->position.x].id==g->dungeon[second->position.y][second->position.x].id)){
		return 1;
	}
	//check if they can see from the corridor
	else{
		int i = 0;
		int xdiff = abs(first->position.x-second->position.x);
		int ydiff = abs(first->position.y-second->position.y);
		if(first->position.y==second->position.y){
			if(first->position.x<second->position.x){
				for(i=first->position.x; i<first->position.x+xdiff; i++){
					if(g->dungeon[first->position.y][i].terrain==ter_wall) return 0;
				}
			}
			else{
				for(i=first->position.x; i<first->position.x-xdiff; i--){
					if(g->dungeon[first->position.y][i].terrain==ter_wall) return 0;
				}
			}
			return 1;
		}
		if(first->position.x==second->position.x){
			if(first->position.y<second->position.y){
				for(i=first->position.y; i<first->position.y+ydiff; i++){
					if(g->dungeon[i][first->position.x].terrain==ter_wall) return 0;
				}
			}
			else{
				for(i=first->position.y; i<first->position.y-ydiff; i--){
					if(g->dungeon[i][first->position.x].terrain==ter_wall) return 0;
				}
			}
			return 1;
		}
		return 0;
	}
}