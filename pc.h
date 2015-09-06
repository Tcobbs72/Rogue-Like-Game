#ifndef PC_H
# define PC_H

# include <stdint.h>
# include "object.h"

typedef struct game _game;  

typedef struct player {
  char name[40];
  char catch_phrase[80];
  uint32_t mode; //0 = control and 1 = Look
  object_t *weapon;
  object_t *offhand;
  object_t *ranged;
  object_t *armor;
  object_t *helmet;
  object_t *cloak;
  object_t *gloves;
  object_t *boots;
  object_t *amulet;
  object_t *light;
  object_t *ring[2];
  
  object_t *inventory[10];
  uint32_t inventory_size;
} pc_t;

void pc_delete(pc_t *pc);
uint32_t pc_is_alive(_game *g);
void config_pc(_game *g);
void pc_next_pos(_game *g);
void print_view(_game *g, uint32_t view);
int32_t pc_total_speed(_game *g);
int32_t pc_total_health(_game *g);
int32_t pc_total_damage(_game *g);

#endif