#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>
#include<ncurses.h>
#include "binheap.h"
#include "pair.h"
#include "dungeon.h"
#include "character.h"
#include "npc.h"
#include "pc.h"
#include "descriptions.h"
#include "object.h"


#ifdef WINDOWS
#define home getenv("HOMEPATH")
#else
#define home getenv("HOME")
#endif	

typedef struct path {
  binheap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2]; 
  uint16_t cost;
} path_t;

void clean_game(_game *g){
    int x,y;
    for(y=0; y<Y; y++){
        for(x=0; x<X; x++){
            if(g->character[y][x]) character_delete(g->character[y][x]);
            if(g->object[y][x]) delete_object(g->object[y][x]);
        }
    }
}

//prints out the gameboard
void printdungeon(_game *g){
	erase();
	refresh();
	
	int i,j;
	for(i=0; i<Y; i++){
		for(j=0; j<X; j++){
			if(g->character[i][j]){
                attron(COLOR_PAIR(g->character[i][j]->color));
				mvaddch(i,j,g->character[i][j]->symbol);
				attroff(COLOR_PAIR(g->character[i][j]->color));
			}
            else if(g->object[i][j]){
                attron(COLOR_PAIR(get_color(g->object[i][j])));
                mvaddch(i,j,get_symbol(g->object[i][j]));
                attroff(COLOR_PAIR(get_color(g->object[i][j])));
            }
			else{
				switch (g->dungeon[i][j].terrain){
				case ter_wall:
					mvaddch(i,j,'#');
					break;
				case ter_floor_hall:
					mvaddch(i,j,'.');
					break;
				case ter_floor_room:
					mvaddch(i,j,'.');
					break;
				case ter_debug:
					mvaddch(i,j,'*');
					break;
				case ter_wall_immutable:
					mvaddch(i,j,'-');
					break;
				case ter_stair_down:
					mvaddch(i,j,'>');
					break;
				case ter_stair_up:
					mvaddch(i,j,'<');
					break;
				}
			}
		}
	}
	refresh();
}

void simulate(_game *g){
    print_view(g, 0);
	while(pc_is_alive(g) && dungeon_has_npcs(g)){
		  character_t *c;

		  do {
			c = binheap_remove_min(&g->next_turn);
            if(c){
                if (!c->alive) {
                  if (g->character[c->position.y][c->position.x] == c) {
                    g->character[c->position.y][c->position.x] = NULL;
                  }
                  if (c != g->pc) {
                    character_delete(c);
                  }

                  continue;
                }
                
                if (c == g->pc) {
                  pc_next_pos(g);
                  c->next_turn += (100 / pc_total_speed(g));
                } else if(c->npc) {
                  move_monster(g, c);
                  c->next_turn += (100 / c->speed);
                }

                binheap_insert(&g->next_turn, c);
            }
		  } while (binheap_peek_min(&g->next_turn) != g->pc);
		  print_view(g, 0);
	}
	if(pc_is_alive(g)) printf("\nPC wins!\n");
	else printf("\nMonsters win! %d monsters left\n", g->nummon);
	binheap_delete(&g->next_turn);
}

static int32_t dist_cmp(const void *key, const void *with) {
	return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

//connects cell at (sourceX, sourceY) to cell at (targetX, targetY) with shortest path
pair_t dijkstras(_game *g, pair_t from, pair_t to){
	static path_t path[Y][X], *p;
	static uint32_t initialized = 0;
	binheap_t heap;
	uint32_t x, y;

	if (!initialized) {
		for (y = 0; y < Y; y++) {
			for (x = 0; x < X; x++) {
				path[y][x].pos[0] = y;
				path[y][x].pos[1] = x;
			}
		}
		initialized = 1;
	}
  
	for (y = 0; y < Y; y++) {
		for (x = 0; x < X; x++) {
			path[y][x].cost = 10000;
		}
	}

	path[from.y][from.x].cost = 0;

	binheap_init(&heap, dist_cmp, NULL);

	for (y = 0; y < Y; y++) {
		for (x = 0; x < X; x++) {
			if (g->dungeon[y][x].terrain != ter_wall_immutable && g->dungeon[y][x].terrain != ter_wall) {
				path[y][x].hn = binheap_insert(&heap, &path[y][x]);
			} else {
				path[y][x].hn = NULL;
			}
		}
	}
	while((p = binheap_remove_min(&heap))){
		p->hn=NULL;
		
		if(p->pos[0]==to.y&&p->pos[1]==to.x){
			for (x = to.x, y = to.y; (x != from.x) || (y != from.y); p = &path[y][x], x = p->from[1], y = p->from[0]) {
				//g->dungeon[y][x].terrain = ter_debug;
				if(path[y][x].from[1]==from.x && path[y][x].from[0]==from.y) break;
			}
			
			pair_t ret = {x,y};

			binheap_delete(&heap);
			return ret;
		}

		if ((path[p->pos[0] - 1][p->pos[1] - 1].hn) &&
			(path[p->pos[0] - 1][p->pos[1] - 1].cost > p->cost + 1)) {
		  path[p->pos[0] - 1][p->pos[1] - 1].cost = p->cost + 1;
		  path[p->pos[0] - 1][p->pos[1] - 1].from[0] = p->pos[0];
		  path[p->pos[0] - 1][p->pos[1] - 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] - 1]
											   [p->pos[1] - 1].hn);
		}
		if ((path[p->pos[0] - 1][p->pos[1]    ].hn) &&
			(path[p->pos[0] - 1][p->pos[1]    ].cost > p->cost + 1)) {
		  path[p->pos[0] - 1][p->pos[1]    ].cost = p->cost + 1;
		  path[p->pos[0] - 1][p->pos[1]    ].from[0] = p->pos[0];
		  path[p->pos[0] - 1][p->pos[1]    ].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] - 1]
											   [p->pos[1]    ].hn);
		}
		if ((path[p->pos[0] - 1][p->pos[1] + 1].hn) &&
			(path[p->pos[0] - 1][p->pos[1] + 1].cost > p->cost + 1)) {
		  path[p->pos[0] - 1][p->pos[1] + 1].cost = p->cost + 1;
		  path[p->pos[0] - 1][p->pos[1] + 1].from[0] = p->pos[0];
		  path[p->pos[0] - 1][p->pos[1] + 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] - 1]
											   [p->pos[1] + 1].hn);
		}
		if ((path[p->pos[0]    ][p->pos[1] - 1].hn) &&
			(path[p->pos[0]    ][p->pos[1] - 1].cost > p->cost + 1)) {
		  path[p->pos[0]    ][p->pos[1] - 1].cost = p->cost + 1;
		  path[p->pos[0]    ][p->pos[1] - 1].from[0] = p->pos[0];
		  path[p->pos[0]    ][p->pos[1] - 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0]    ]
											   [p->pos[1] - 1].hn);
		}
		if ((path[p->pos[0]    ][p->pos[1] + 1].hn) &&
			(path[p->pos[0]    ][p->pos[1] + 1].cost > p->cost + 1)) {
		  path[p->pos[0]    ][p->pos[1] + 1].cost = p->cost + 1;
		  path[p->pos[0]    ][p->pos[1] + 1].from[0] = p->pos[0];
		  path[p->pos[0]    ][p->pos[1] + 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0]    ]
											   [p->pos[1] + 1].hn);
		}
		if ((path[p->pos[0] + 1][p->pos[1] - 1].hn) &&
			(path[p->pos[0] + 1][p->pos[1] - 1].cost > p->cost + 1)) {
		  path[p->pos[0] + 1][p->pos[1] - 1].cost = p->cost + 1;
		  path[p->pos[0] + 1][p->pos[1] - 1].from[0] = p->pos[0];
		  path[p->pos[0] + 1][p->pos[1] - 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] + 1]
											   [p->pos[1] - 1].hn);
		}
		if ((path[p->pos[0] + 1][p->pos[1]    ].hn) &&
			(path[p->pos[0] + 1][p->pos[1]    ].cost > p->cost + 1)) {
		  path[p->pos[0] + 1][p->pos[1]    ].cost = p->cost + 1;
		  path[p->pos[0] + 1][p->pos[1]    ].from[0] = p->pos[0];
		  path[p->pos[0] + 1][p->pos[1]    ].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] + 1]
											   [p->pos[1]    ].hn);
		}
		if ((path[p->pos[0] + 1][p->pos[1] + 1].hn) &&
			(path[p->pos[0] + 1][p->pos[1] + 1].cost > p->cost + 1)) {
		  path[p->pos[0] + 1][p->pos[1] + 1].cost = p->cost + 1;
		  path[p->pos[0] + 1][p->pos[1] + 1].from[0] = p->pos[0];
		  path[p->pos[0] + 1][p->pos[1] + 1].from[1] = p->pos[1];
		  binheap_decrease_key(&heap, path[p->pos[0] + 1]
											   [p->pos[1] + 1].hn);
		}
	}

	pair_t ret = {-1,-1};
	return ret;
}

//converts 32 bit int to Big Endian
uint32_t swap32(uint32_t value){
	int n = 1;
	if(*(char*) &n == 1) { 
		uint32_t b[4];
		b[0]= (value & 0x000000ff) << 24;
		b[1]= (value & 0x0000ff00) << 8;
		b[2]= (value & 0x00ff0000) >> 8;
		b[3]= (value & 0xff000000) >> 24;
		value = b[0] | b[1] | b[2] | b[3];
	}
	return value;
}

//converts 16 bit int to Big Endian
uint16_t swap16(uint16_t value){
	int n = 1;	
	if(*(char*) &n == 1) { 
		uint16_t b[2];
		b[0]= (value & 0x00ff) << 8;
		b[1]= (value & 0xff00) >> 8;
		value = b[0] | b[1];
	}
	return value;
}

//adds the cell ids to rooms 
void add_cell_id(_game *g){
	int i;
	for(i=0; i<totalRooms; i++){
		int j, k;
		for(j=g->rooms[i]->pair.y; j<g->rooms[i]->pair.y+g->rooms[i]->height; j++){
			for(k=g->rooms[i]->pair.x; k<g->rooms[i]->pair.x+g->rooms[i]->width; k++){
				g->dungeon[j][k].id=g->rooms[i]->id;
			}
		}
	}
}

//connects 2 rooms with a hallway
void makePath(struct cell dungeon[Y][X], int x, int y, struct room *target){
	int _pathX = target->pair.x-x;
	int _pathY = target->pair.y-y;
	int currentX = x;
	int currentY = y;
	int i, j, _negX = 1, _negY = 1;
	if(_pathX<0)  _negX=-1;
	if(_pathY<0)  _negY=-1;
	int skip = rand()%6;
	
	if(skip!=1){
		for(i=0; i<=(_pathX*_negX); i++){
			currentX=x+i*_negX;
			if(dungeon[currentY][currentX].room==0){
				dungeon[currentY][currentX].hardness=0;
				dungeon[currentY][currentX].corridor=1;
				dungeon[currentY][currentX].terrain=ter_floor_hall;
			}
			if(rand()%3==1) i=_pathX*_negX;
		}
	}
	
	if(skip!=2){
		for(j=0; j<=(_pathY*_negY); j++){
			currentY=y+j*_negY;
			if(dungeon[currentY][currentX].room==0){
				dungeon[currentY][currentX].hardness=0;
				dungeon[currentY][currentX].corridor=1;
				dungeon[currentY][currentX].terrain=ter_floor_hall;
			}
			if(rand()%3==1) j=_pathY*_negY;
		}
	}
	if(currentX!=target->pair.x || currentY!=target->pair.y){
		makePath(dungeon, currentX, currentY, target);
	}
	
}

//iterates over all rooms and uses makePath to connect them
void connect_rooms(_game *g){
	int i;
	for(i=1; i<totalRooms; i++){
		makePath(g->dungeon, g->rooms[i-1]->pair.x, g->rooms[i-1]->pair.y, g->rooms[i]);
	}
}

//removes room with roomNum from the game board
void remove_room(struct cell dungeon[Y][X], int roomNum){
	int i,j;
	
	for(i=0; i<Y; i++){
		for(j=0; j<X; j++){
			if(dungeon[i][j].id==roomNum){
				dungeon[i][j].hardness=1;
				dungeon[i][j].id = -1;
				dungeon[i][j].room=0;
				dungeon[i][j].terrain=ter_wall;
				if(i==0 || j==0 || i==Y-1 || j==X-1) dungeon[i][j].terrain=ter_wall_immutable;
			}
		}
	}
}

//checks to see if you can place a cell at (x,y) with id of room_id
//returns 1 if valid, 0 if invalid
int valid_placement(struct cell dungeon[Y][X], int y, int x, int room_id){
	if(dungeon[y][x].ismutable==0) return 0; //trying to place room on immutable piece
	if(dungeon[y][x].id != -1 && dungeon[y][x].id != room_id) return 0; //trying to place room on another room
	
	//checks to make sure no other room is within 3 units from any part of this room
	if(x>3 && dungeon[y][x-3].id != -1 && dungeon[y][x-3].id != room_id) return 0; //left 3
	if(y>3 && dungeon[y-3][x].id != -1 && dungeon[y-3][x].id != room_id) return 0; //up 3
	if(x<X-3 && dungeon[y][x+3].id != -1 && dungeon[y][x+3].id != room_id) return 0; //right 3
	if(y<Y-3 && dungeon[y+3][x].id != -1 && dungeon[y+3][x].id != room_id) return 0; //down 3
	if(y>3 && x>3 && dungeon[y-3][x-3].id != -1 && dungeon[y-3][x-3].id != room_id) return 0; //left 3 up 3
	if(y<Y-3 && x>3 && dungeon[y+3][x-3].id != -1 && dungeon[y+3][x-3].id != room_id) return 0; //left 3 down 3
	if(y>3 && x<X-3 && dungeon[y-3][x+3].id != -1 && dungeon[y-3][x+3].id != room_id) return 0; //right 3 down 3
	if(y<Y-3 && x<X-3 && dungeon[y+3][x+3].id != -1 && dungeon[y+3][x+3].id != room_id) return 0; //right 3 up 3
	return 1;
}

//tries to place newRoom with top left corner being (x,y)
//returns 1 if placed correctly, 0 if no possible place to put room
int place_room(int x, int y, struct cell dungeon[Y][X], struct room *newRoom, uint32_t stairUp, uint32_t stairDn){
	newRoom->pair.x=x;
	newRoom->pair.y=y;
	newRoom->width = (rand() % 10) + 8;
	newRoom->height = (rand() % 10) + 5;
	pair_t randDn, randUp = {0,0};
	if(stairUp==newRoom->id){ //if this is the room to have upstairs find random spot inside room
		randUp.x = rand()%newRoom->width + newRoom->pair.x; 
		randUp.y = rand()%newRoom->height + newRoom->pair.y;
	}
	if(stairDn==newRoom->id){ //if this is the room to have downstairs find random spot inside room
		randDn.x = rand()%newRoom->width + newRoom->pair.x;
		randDn.y = rand()%newRoom->height + newRoom->pair.y;
	}
	int i,j;
	for(i=0; i<newRoom->height; i++){
		for(j=0; j<newRoom->width; j++){
			if(valid_placement(dungeon, y+i, x+j, newRoom->id)==1){ //if part of the room can be placed there...
				dungeon[y+i][x+j].hardness=0;
				dungeon[y+i][x+j].id=newRoom->id;
				dungeon[y+i][x+j].room=1;
				dungeon[y+i][x+j].terrain=ter_floor_room;
				if(randUp.x==(x+j) && randUp.y==(y+i)) dungeon[y+i][x+j].terrain=ter_stair_up;
				if(randDn.x==(x+j) && randDn.y==(y+i)) dungeon[y+i][x+j].terrain=ter_stair_down;
			}
			else{ //otherwise remove all parts of that room that have been place on the board and go to the next spot
				remove_room(dungeon, newRoom->id);
				return 0;
			}
		}
	}
	return 1;
}

//finds a random place to put the newRoom
int find_place_room(struct cell dungeon[Y][X], struct room *newRoom, uint32_t stairUp, uint32_t stairDn){
	int i = 0;
	int j = 0;
	int count = 0;
	while(place_room(i,j,dungeon,newRoom, stairUp, stairDn)==0 && count<1000){ //if the whole room got placed..
		i = rand() % X;
		j = rand() % Y; 
		newRoom->pair.x=i;
		newRoom->pair.y=j;
		count++;
	}
	if(count>=1000) return 0;
	return 1;
}

//initializes member variables of each cell on the board
void initialize(_game *g){
	int i,j;
	for(i=0; i<Y; i++){
		for(j=0; j<X; j++){
			g->dungeon[i][j].ismutable=1;
			g->dungeon[i][j].hardness=1;
			g->dungeon[i][j].id=-1;
			g->dungeon[i][j].room=0;
			g->dungeon[i][j].corridor=0;
			g->dungeon[i][j].visited=0;
			g->dungeon[i][j].terrain=ter_wall;
			g->dungeon[i][j].pair.x=j;
			g->dungeon[i][j].pair.y=i;
			g->character[i][j]=NULL;
            g->object[i][j]=NULL;
			if(i==0 || j==0 || i==Y-1 || j==X-1){ //set walls to be immutable
				g->dungeon[i][j].ismutable=0;
				g->dungeon[i][j].hardness=255;
				g->dungeon[i][j].terrain=ter_wall_immutable;
			}
		}
	}
}

//generate the dungeon with all of the rooms
int generate(_game *g){
	int i;
	struct room *newRoom;
	uint32_t stairUp = rand()%totalRooms; //random room to have upstairs
	uint32_t stairDn = rand()%totalRooms; //random room to have downstairs
	for(i=0; i<totalRooms; i++){
		newRoom = malloc(sizeof(*newRoom));
		newRoom->id=i;
		g->rooms[i]=newRoom;
		if(find_place_room(g->dungeon, g->rooms[i], stairUp, stairDn)==0){
			initialize(g);
			return 0;
		}
	}
	return 1;
}

//generates a dungeon and saves it to the disk
void save(_game *g){
		FILE *f = malloc(sizeof(FILE));
		char* path = "/.rlg229/dungeon";
		char* both = malloc(strlen(home) + strlen(path) + 2);
		strcpy(both, home);
		strcat(both, path);
		f=fopen(both, "wb");
		if(f){
		
			int i, j;
			fwrite("RLG229", 1, strlen("RLG229"), f);
			printf("printed RLG229\n");
			
			uint32_t version = swap32(6);
			fwrite(&version, sizeof(version), 1, f);
			printf("version: %d\n", swap32(version));
			
			uint32_t size = swap32(76819+(totalRooms*4)+2+4+4+2+(g->nummon*36)-14);
			fwrite(&size, sizeof(size), 1, f);
			printf("size: %d\n", swap32(size));
			
			uint8_t a = 0;
			uint8_t b = 1;
			uint8_t c = 2;
			
			for(i=0; i<Y; i++){
				for(j=0; j<X; j++){
					if(g->dungeon[i][j].hardness!=0) fwrite(&a,1,1,f);
					else fwrite(&b,1,1,f); 
					if(g->dungeon[i][j].id==-1) fwrite(&a,1,1,f);
					else fwrite(&b,1,1,f);
					if(g->dungeon[i][j].hardness==0 && g->dungeon[i][j].id==-1) fwrite(&b,1,1,f);
					else fwrite(&a,1,1,f);
					fwrite(&(g->dungeon[i][j].hardness), 1, 1, f);
					if(g->dungeon[i][j].terrain==ter_stair_up) fwrite(&c,1,1,f);
					else if(g->dungeon[i][j].terrain==ter_stair_down) fwrite(&b,1,1,f);
					else fwrite(&a,1,1,f);
				}
			}
			
			uint16_t total = totalRooms;
			total = swap16(total);
			fwrite(&total, sizeof(total), 1, f);
			printf("totalsize: %d\n", swap16(total));
			
			for(i=0; i<totalRooms; i++){
				uint8_t cornerX = g->rooms[i]->pair.x;
				uint8_t cornerY = g->rooms[i]->pair.y;
				uint8_t width = g->rooms[i]->width;
				uint8_t height = g->rooms[i]->height;			
				fwrite(&cornerX,sizeof(cornerX),1,f);
				fwrite(&cornerY,sizeof(cornerY),1,f);
				fwrite(&width,sizeof(width),1,f); 
				fwrite(&height,sizeof(height),1,f);
				printf("Room %2d: (%3d,%3d) width: %2d height: %2d\n", i, cornerX, cornerY, width, height);
			}
			
			uint8_t pcX = g->pc->position.x;
			uint8_t pcY = g->pc->position.y;
			fwrite(&pcX,1,1,f);
			fwrite(&pcY,1,1,f);	
			printf("PC at (%d,%d)\n", pcX, pcY);
			
			uint32_t next_turn = swap32(g->pc->next_turn);
			fwrite(&next_turn,sizeof(next_turn),1,f);
			printf("PC next turn in %d turns\n", swap32(next_turn));
			
			uint32_t sequence_number = swap32(g->nummon+1);
			fwrite(&sequence_number, sizeof(sequence_number), 1, f);
			printf("Game sequence number: %d\n", swap32(sequence_number));
			
			uint16_t num_monsters = g->nummon;
			num_monsters=swap16(num_monsters);
			fwrite(&num_monsters, sizeof(num_monsters), 1, f);
			printf("%d monsters left\n", swap16(num_monsters));
			
			for(i=0; i<Y; i++){
				for(j=0; j<X; j++){
					if(g->character[i][j] && g->character[i][j]->npc){
						uint8_t npcX = g->character[i][j]->position.x;
						uint8_t npcY = g->character[i][j]->position.y;
						uint8_t lastKnownX = g->character[i][j]->npc->pc_last_known_position.x;
						uint8_t lastKnownY = g->character[i][j]->npc->pc_last_known_position.y;
						uint32_t npcRank = swap32(g->character[i][j]->rank);
						uint32_t npcTurn = swap32(g->character[i][j]->next_turn);
						uint32_t npcSpeed = swap32(g->character[i][j]->speed);
						char symbol = g->character[i][j]->symbol;
						fwrite(&symbol, 1, 1, f);
						fwrite(&npcX,sizeof(npcX),1,f);
						fwrite(&npcY,sizeof(npcY),1,f);
						
						if(has_characteristic(g->character[i][j], SMART) || has_characteristic(g->character[i][j], TELEPATH)) fwrite(&b, 1, 1, f);
						else fwrite(&a, 1, 1, f);
						
						if(has_characteristic(g->character[i][j], TELEPATH)) fwrite(&b, 1, 1, f);
						else fwrite(&a, 1, 1, f);
						
						fwrite(&lastKnownX, 1, 1, f);
						fwrite(&lastKnownY, 1, 1, f);
						
						fwrite(&npcRank, sizeof(npcRank), 1, f);
						fwrite(&npcTurn, sizeof(npcTurn), 1, f);
						fwrite(&npcSpeed, sizeof(npcSpeed), 1, f);
						fwrite(&a, 1, 16, f);
						printf("Monster at (%3d,%3d) speed: %2d, Rank: %2d, PcLastKnown: (%3d,%3d)\n", npcX, npcY, swap32(npcSpeed), swap32(npcRank), lastKnownX, lastKnownY);
					}
				}
			}
			fclose(f);
		}	
}

//loads a dungeon file from the disk and prints it
void load(_game *g){
	FILE *f = malloc(sizeof(FILE));
	char* path = "/.rlg229/dungeon";
	char* both = malloc(strlen(home) + strlen(path) + 2);
	strcpy(both, home);
	strcat(both, path);
	f=fopen(both, "rb");
	
	if(!g->pc) g->pc=malloc(sizeof(*g->pc));
	g->pc->npc=NULL;
	g->pc->pc = malloc(sizeof(*g->pc->pc));
	g->pc->alive=1;

	if(f){

		char marker[6];
		uint32_t version;
		uint32_t filesize;

		int i, j;
		fread(&marker, sizeof(marker), 1, f);
		printf("Name: %s\n", marker);
		
		fread(&version, 4, 1, f);
		version = swap32(version);
		printf("Version: %d\n", version);
		
		fread(&filesize, 4, 1, f);
		filesize=swap32(filesize);
		printf("File size: %d\n", filesize);
		
		for(i=0; i<Y; i++){
			for(j=0; j<X; j++){
				int openspace;
				fread(&openspace, 1, 1, f);
				fread(&(g->dungeon[i][j].room), 1, 1, f);
				if(g->dungeon[i][j].room==1) g->dungeon[i][j].terrain=ter_floor_room;
				fread(&(g->dungeon[i][j].corridor), 1, 1, f);
				if(g->dungeon[i][j].corridor==1) g->dungeon[i][j].terrain=ter_floor_hall;
				fread(&(g->dungeon[i][j].hardness), 1, 1, f);
				if(g->dungeon[i][j].hardness==255) g->dungeon[i][j].terrain=ter_wall_immutable;
				else if(g->dungeon[i][j].hardness!=0) g->dungeon[i][j].terrain=ter_wall;
				int stairs;
				fread(&stairs, 1, 1, f);
				if(stairs==1) g->dungeon[i][j].terrain=ter_stair_down;
				else if(stairs==2) g->dungeon[i][j].terrain=ter_stair_up;
				g->dungeon[i][j].id=-1;
				g->character[i][j]=NULL;
			}
		}
		
		uint16_t total;
		fread(&total, 2, 1, f);
		total=swap16(total);
		printf("Total Rooms: %d\n", total);
		
		for(i=0; i<total; i++){
			struct room *new = malloc(sizeof(struct room));
			new->id=i;
			uint8_t position;
			fread(&position, 1, 1, f);
			new->pair.x=position;
			fread(&position, 1, 1, f);
			new->pair.y=position;
			fread(&new->width, 1, 1, f);
			fread(&new->height, 1, 1, f);
			g->rooms[i]=new;
			printf("Room %2d: (%3d, %3d) width: %2d, height: %2d\n", i, g->rooms[i]->pair.x, g->rooms[i]->pair.y, g->rooms[i]->width, g->rooms[i]->height);
		}
		
		uint8_t toRead;
		fread(&toRead, 1, 1, f);
		g->pc->position.x=toRead;
		fread(&toRead, 1, 1, f);
		g->pc->position.y=toRead;
		g->pc->symbol='@';
		g->character[g->pc->position.y][g->pc->position.x]=g->pc;
		printf("PC at (%d, %d)\n", g->pc->position.x, g->pc->position.y);
		
		uint32_t toRead2;
		fread(&toRead2,sizeof(toRead2),1,f);
		g->pc->next_turn = swap32(toRead2);
		printf("PC next turn in %d turns\n", g->pc->next_turn);
		
		fread(&toRead2, sizeof(toRead2), 1, f);
		printf("Game sequence number: %d\n", swap32(toRead2));
		
		uint16_t num_monsters;
		fread(&num_monsters, sizeof(num_monsters), 1, f);
		g->nummon=swap16(num_monsters);
		printf("%d monsters left\n", g->nummon);	
		
		for(i=0; i<g->nummon; i++){
			character_t *m = malloc(sizeof(*m));
			
			m->pc=NULL;
			m->npc=malloc(sizeof(*m->npc));
						
			char temp;
			fread(&temp, sizeof(char), 1, f);
			m->symbol=temp;
			
			uint8_t position;
			fread(&position,1,1,f);
			m->position.x=position;
			fread(&position,1,1,f);
			m->position.y=position;
						
			uint8_t smart = 0;
			uint8_t telepath = 0;
			fread(&smart, 1, 1, f);				
			fread(&telepath, 1, 1, f);
			if(telepath) m->npc->characteristics=0x00000004;
			else if(smart)  m->npc->characteristics=0x00000002;
			else  m->npc->characteristics=0x00000001;
				
			fread(&position, 1, 1, f);
			m->npc->pc_last_known_position.x=position;
			fread(&position, 1, 1, f);
			m->npc->pc_last_known_position.y=position;
			
			uint32_t next;
							
			fread(&next, sizeof(m->rank), 1, f);
			m->rank=swap32(next);
			fread(&next, sizeof(m->next_turn), 1, f);
			m->next_turn=swap32(next);
			fread(&next, sizeof(m->speed), 1, f);
			m->speed=swap32(next);
			
			int blank;
			fread(&blank, 1, 16, f);
			
			m->alive=1;
			g->character[m->position.y][m->position.x]=m;
			printf("Monster at (%3d,%3d) speed: %2d, Rank: %2d, PcLastKnown: (%3d,%3d) next turn: %d\n", 
							m->position.x, m->position.y, m->speed, m->rank, m->npc->pc_last_known_position.x, m->npc->pc_last_known_position.y, m->next_turn);
			binheap_insert(&g->next_turn, m);
		}
		add_cell_id(g);
		fclose(f);
		g->pc->pc->mode = 0;
		g->pc->speed = 10;
		binheap_insert(&g->next_turn, g->pc);
		simulate(g);
	}
}

void io_init_terminal(void)    
{
	initscr();      
	raw();      
	noecho();     
	curs_set(0);      
	keypad(stdscr, TRUE);    
    start_color();
    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

int main(int argc, char *argv[]){
	srand(time(NULL));
	_game g;
	binheap_init(&(g.next_turn),compare_characters_by_next_turn, NULL);
	
	io_init_terminal();
	
	if(argc<2 || argc>3){
		fprintf(stderr, "\nInvalid number of parameters.\n");
	}
	else if(argc ==2 && !strcmp(argv[1], "--load")){
		load(&g);
	}
	else if(argc== 3 && !strcmp(argv[1], "--nummon")){
		uint32_t nummon;
		sscanf (argv[2],"%d",&nummon);
		initialize(&g);
		while(generate(&g)!=1){
			generate(&g);
		}
		connect_rooms(&g);
		
        char* mpath = "/.rlg229/monster_desc.txt";
		char* mboth = malloc(strlen(home) + strlen(mpath) + 2);
		strcpy(mboth, home);
		strcat(mboth, mpath);
        
        char* opath = "/.rlg229/object_desc.txt";
		char* oboth = malloc(strlen(home) + strlen(opath) + 2);
		strcpy(oboth, home);
		strcat(oboth, opath);
        
        parse_descriptions(&g, mboth, oboth);
        
        config_pc(&g);
        gen_monsters(&g, nummon);
        gen_objects(&g, 40);
        
		simulate(&g);
		printdungeon(&g);
        destroy_descriptions(&g);
        clean_game(&g);
	}
	else{
		fprintf(stderr, "\nInvalid switches. Expecting either --save, --nummon #, --load, or --load --save\n");
	}
    //endwin();
	return 0;
}