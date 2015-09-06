#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<string.h>
#include<stdint.h>
#include "binheap.h"
#include "pair.h"
#include "dungeon.h"
#include "npc.h"
#include "pc.h"
#include "character.h"
#include "descriptions.h"
#include "dice.h"

void gen_monsters(_game *g, uint32_t nummon){
	uint32_t i;


	g->nummon = nummon;
	
	for(i=0; i<nummon; i++){
		generate_monster(g, i+1);
	}
}

void npc_delete(npc_t *n){
	free(n);
}

void npc_next_pos_rand(_game *g, character_t *m){
	g->character[m->position.y][m->position.x]=NULL;
	int xmoves[] = {-1,0,1,0};
	int ymoves[] = {0,-1,0,1};
	int next = rand()%4;
	int nextX = m->position.x + xmoves[next];
	int nextY = m->position.y + ymoves[next];
	while(g->dungeon[nextY][nextX].terrain==ter_wall_immutable || g->dungeon[nextY][nextX].terrain==ter_wall || g->character[nextY][nextX]!=g->pc){
		next = rand()%4;
		nextX = m->position.x + xmoves[next];
		nextY = m->position.y + ymoves[next];
	}
	if(g->character[nextY][nextX]==g->pc){
        g->pc->health-=roll_dice(m->damage);
		if(g->pc->health<=0){
			g->pc->alive=0;
			pc_delete(g->pc->pc);
            m->position.y=nextY;
            m->position.x=nextX;
		}
	}
    else{
        m->position.y=nextY;
        m->position.x=nextX;
    }
	g->character[m->position.y][m->position.x]=m;
}

void npc_next_pos_line_of_sight(_game *g, character_t *m){
	g->character[m->position.y][m->position.x]=NULL;
	int x_diff = abs(m->position.x-m->npc->pc_last_known_position.x);
	int y_diff = abs(m->position.y-m->npc->pc_last_known_position.y);
    int nextX = m->position.x;
    int nextY = m->position.y;
    
	if(x_diff>y_diff){
		if(m->position.x>m->npc->pc_last_known_position.x && g->dungeon[m->position.y][m->position.x-1].terrain!=ter_wall) nextX--;
		else if(g->dungeon[m->position.y][m->position.x+1].terrain!=ter_wall) nextX++;
	}
	else{
		if(m->position.y>m->npc->pc_last_known_position.y && g->dungeon[m->position.y-1][m->position.x].terrain!=ter_wall) nextY--;
		else if(g->dungeon[m->position.y+1][m->position.x].terrain!=ter_wall) nextY++;
	}
    
    if(g->character[nextY][nextX] && g->character[nextY][nextX]->npc){
        if(g->dungeon[nextY-1][nextX].terrain!=ter_wall && !g->character[nextY-1][nextX]) nextY--;
        else if(g->dungeon[nextY+1][nextX].terrain!=ter_wall && !g->character[nextY+1][nextX]) nextY++;
        else if(g->dungeon[nextY][nextX-1].terrain!=ter_wall && !g->character[nextY][nextX-1]) nextX--;
        else nextX++;
    }
    
	if(g->character[nextY][nextX]==g->pc){
        g->pc->health-=roll_dice(m->damage);
		if(g->pc->health<=0){
			g->pc->alive=0;
			pc_delete(g->pc->pc);
            m->position.y=nextY;
            m->position.x=nextX;
		}
	}
    else{
        m->position.y=nextY;
        m->position.x=nextX;
    }
	g->character[m->position.y][m->position.x]=m;
}

void npc_next_pos_djikstra(_game *g, character_t *m){
	pair_t next = dijkstras(g, m->position, m->npc->pc_last_known_position);
	g->character[m->position.y][m->position.x]=NULL;
    int nextX;
    int nextY;
	if(next.y && next.x){
		nextX=next.x;
		nextY=next.y;
	}
	if(g->character[nextY][nextX] && g->character[nextY][nextX]->npc){
        if(g->dungeon[nextY-1][nextX].terrain!=ter_wall && !g->character[nextY-1][nextX]) nextY--;
        else if(g->dungeon[nextY+1][nextX].terrain!=ter_wall && !g->character[nextY+1][nextX]) nextY++;
        else if(g->dungeon[nextY][nextX-1].terrain!=ter_wall && !g->character[nextY][nextX-1]) nextX--;
        else nextX++;
    }
    
	if(g->character[nextY][nextX]==g->pc){
        g->pc->health-=roll_dice(m->damage);
		if(g->pc->health<=0){
			g->pc->alive=0;
			pc_delete(g->pc->pc);
            m->position.y=nextY;
            m->position.x=nextX;
		}
	}
    else{
        m->position.y=nextY;
        m->position.x=nextX;
    }
	g->character[m->position.y][m->position.x] = m;
}

void move_monster(_game *g, character_t *c){
	  switch (c->npc->characteristics &
          (NPC_SMART | NPC_TELEPATH | NPC_TUNNEL | NPC_PASS_WALL)) {
      case 0:
      case NPC_TUNNEL:
      case NPC_PASS_WALL:
      case NPC_PASS_WALL | NPC_TUNNEL:
        if (can_see(g, c, g->pc)) {
          c->npc->pc_last_known_position.y = g->pc->position.y;
          c->npc->pc_last_known_position.x = g->pc->position.x;
          npc_next_pos_line_of_sight(g, c);
        } else {
          npc_next_pos_rand(g, c);
        }
        break;
      case NPC_SMART:
      case NPC_SMART | NPC_TUNNEL:
      case NPC_PASS_WALL | NPC_SMART:
      case NPC_PASS_WALL | NPC_SMART | NPC_TUNNEL:
        if (can_see(g, c, g->pc)) {
          c->npc->pc_last_known_position.y = g->pc->position.y;
          c->npc->pc_last_known_position.x = g->pc->position.x;
          c->npc->has_seen_pc = 1;
        } else if ((c->npc->pc_last_known_position.y == c->position.y) &&
                   (c->npc->pc_last_known_position.x == c->position.x)) {
          c->npc->has_seen_pc = 0;
        }
        if (c->npc->has_seen_pc) {
          npc_next_pos_line_of_sight(g, c);
        }
        break;
      case NPC_TELEPATH:
      case NPC_TELEPATH | NPC_TUNNEL:
      case NPC_PASS_WALL | NPC_TELEPATH:
      case NPC_PASS_WALL | NPC_SMART | NPC_TELEPATH:
      case NPC_PASS_WALL | NPC_TELEPATH | NPC_TUNNEL:
      case NPC_PASS_WALL | NPC_SMART | NPC_TELEPATH | NPC_TUNNEL:
        /* We could implement a "toward pc" movement function, but this works *
         * just as well, since the line of sight calculations are down        *
         * outside of the next position function.                             */
        c->npc->pc_last_known_position.y = g->pc->position.y;
        c->npc->pc_last_known_position.x = g->pc->position.x;
        npc_next_pos_line_of_sight(g, c);      
        break;
      case NPC_SMART | NPC_TELEPATH:
      case NPC_SMART | NPC_TELEPATH | NPC_TUNNEL:
          npc_next_pos_djikstra(g, c);
        break;
      }
}

uint32_t dungeon_has_npcs(_game *g)
{
  return g->nummon;
}
