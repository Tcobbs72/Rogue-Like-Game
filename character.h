#ifndef CHARACTER_H
# define CHARACTER_H

#include <stdint.h>
#include "pair.h"

typedef struct game _game;
typedef struct npc npc_t;
typedef struct player pc_t;
typedef struct dice_t dice_t;

typedef struct character {
  char symbol;
  uint32_t color;
  pair_t position;
  int32_t health;
  const dice_t *damage;
  char* sp;
  uint32_t speed;
  uint32_t abilities;
  uint32_t next_turn;
  uint32_t alive;
  npc_t *npc;
  pc_t *pc;
  uint32_t rank;
} character_t;

int32_t compare_characters_by_next_turn(const void *character1, const void *character2);
void character_delete(void *c);
uint32_t can_see(_game *g, character_t *first, character_t *second);

#endif
