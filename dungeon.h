#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdint.h>
#include <stdio.h>
#include "pair.h"
#include "binheap.h"
#include "object.h"

#define X 160
#define Y 96
#define totalRooms 15

struct room {
	uint8_t id; //0,1,2,3,etc.
	uint8_t width; //width of the room
	uint8_t height; //height of the room
	pair_t pair;
};

typedef enum terrain_type {
  ter_debug,
  ter_wall,
  ter_wall_no_room,
  ter_wall_no_floor,
  ter_wall_immutable,
  ter_floor,
  ter_floor_room,
  ter_floor_hall,
  ter_floor_tentative,
  ter_stair_down,
  ter_stair_up
} terrain_type_t;

struct cell{
	uint32_t id; //id of the room on this cell, -1 if no room there
	uint8_t hardness; //used for dijkstras
	uint8_t terrain;
	uint8_t room; //1 if room, 0 if otherwise
	uint8_t corridor; //1 if corridor, 0 if otherwise
	uint32_t ismutable; //1=true, 0=false
	int visited;
	pair_t pair;
};

typedef struct npc npc_t;
typedef struct character character_t;
typedef struct binheap binheap_t;
typedef void *monster_description_t;
typedef void *object_description_t;

typedef struct game{
	struct cell dungeon[Y][X]; //grid of cells that display this game
	struct room *rooms[totalRooms]; //array of rooms for this dungeon
	character_t *pc;
	character_t *character[Y][X];
    object_t *object[Y][X];
	binheap_t next_turn;
	int nummon;
    int numobj;
    monster_description_t monster_descriptions;
    object_description_t object_descriptions;
} _game;

//generates a dungeon and saves it to the disk
void save(_game *g);

//loads a dungeon file from the disk and prints it
void load(_game *g);

pair_t dijkstras(_game *g, pair_t from, pair_t to);

void initialize(_game *g);
int generate(_game *g);
void connect_rooms(_game *g);

#endif