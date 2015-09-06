#include <stdlib.h>
#include <ncurses.h>
#include "string.h"
#include "binheap.h"
#include "dungeon.h"
#include "pc.h"
#include "npc.h"
#include "character.h"
#include "object.h"
#include "descriptions.h"
#include "dice.h"

int32_t pc_total_health(_game *g);
int32_t pc_total_speed(_game *g);
int32_t pc_total_damage(_game *g);

void pc_delete(pc_t *pc)
{
  free(pc);
}

uint32_t pc_is_alive(_game *g)
{
  return g->pc->alive;
}

void config_pc(_game *g)
{
  if(!g->pc) g->pc = malloc(sizeof (*g->pc));
  g->pc->symbol = '@';
  g->pc->color = 2;
  int room = rand()%totalRooms;
  g->pc->position.y = rand()%g->rooms[room]->height+g->rooms[room]->pair.y;
  g->pc->position.x = rand()%g->rooms[room]->width+g->rooms[room]->pair.x;
  g->pc->speed = 10;
  g->pc->next_turn = 0;
  g->pc->alive = 1;
  g->pc->rank = 0;
  g->pc->damage = new_dice(1000,1,4);
  g->pc->health = 100;
  g->pc->pc = malloc(sizeof (*g->pc->pc));
  strncpy(g->pc->pc->name, "Isabella Garcia-Shapiro", sizeof (g->pc->pc->name));
  strncpy(g->pc->pc->catch_phrase,
          "Whatcha doin'?", sizeof (g->pc->pc->name));
  g->pc->pc->mode = 0;
  g->pc->pc->inventory_size = 0;
  g->pc->npc = NULL;
  g->pc->pc->weapon = NULL;
  g->pc->pc->offhand = NULL;
  g->pc->pc->ranged = NULL;
  g->pc->pc->armor = NULL;
  g->pc->pc->helmet = NULL;
  g->pc->pc->cloak = NULL;
  g->pc->pc->gloves = NULL;
  g->pc->pc->boots = NULL;
  g->pc->pc->amulet = NULL;
  g->pc->pc->light = NULL;
  g->pc->pc->ring[0] = NULL;
  g->pc->pc->ring[1] = NULL;
  if(g->character[g->pc->position.y][g->pc->position.x]){
	g->nummon--;
	npc_delete(g->character[g->pc->position.y][g->pc->position.x]->npc);
  }
  g->character[g->pc->position.y][g->pc->position.x] = g->pc;

  binheap_insert(&g->next_turn, g->pc);
}

//0=center, 1=up, 2=right, 3=down, 4=left
void print_view(_game *g, uint32_t view){
	erase();
	//refresh();
	int x,y=0;
	switch(view){
		case 0:
			x = g->pc->position.x-40;
			y = g->pc->position.y-12;
			break;
		case 1:
			x = g->pc->position.x-40;
			y = g->pc->position.y-36;
			break;
		case 2:
			x = g->pc->position.x+40;
			y = g->pc->position.y-12;
			break;
		case 3:
			x = g->pc->position.x-40;
			y = g->pc->position.y+12;
			break;
		case 4:
			x = g->pc->position.x-120;
			y = g->pc->position.y-12;
			break;
	}
	if(x<0) x=0;
	if(x+80>X) x = X-80;
	if(y<0) y=0;
	if(y+24>Y) y = Y-24;
	int tempX=x;
	int i,j;
	for(i=10; i<34; i++){
		x=tempX;
		for(j=40; j<120; j++){
			if(g->character[y][x]){
                attron(COLOR_PAIR(g->character[y][x]->color));
				mvaddch(i,j,g->character[y][x]->symbol);
                attroff(COLOR_PAIR(g->character[y][x]->color));
			}
            else if(g->object[y][x]){
                attron(COLOR_PAIR(get_color(g->object[y][x])));
                mvaddch(i,j, get_symbol(g->object[y][x]));
                attroff(COLOR_PAIR(get_color(g->object[y][x])));
            }
			else{
				switch (g->dungeon[y][x].terrain){
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
			x++;
		}
		y++;
	}
    
    mvaddstr(0,0, "w->wear an item");
    mvaddstr(2,0, "t->take an item off");
    mvaddstr(4,0, "d->drop an item");
    mvaddstr(6,0, "x->expunge an item");
    mvaddstr(8,0, "s->view your player stats");
    
	refresh();
}

void clean_board(_game *g){
	int i, j = 0;
	for(i=0; i<Y; i++){
		for(j=0; j<X; j++){
			g->character[i][j]=NULL;
		}
	}
	binheap_delete(&g->next_turn);
	binheap_init(&(g->next_turn), compare_characters_by_next_turn, NULL);
}

void pc_new_floor(_game *g){
  g->character[g->pc->position.y][g->pc->position.x]=NULL;
  int room = rand()%totalRooms;
  g->pc->position.y = rand()%g->rooms[room]->height+g->rooms[room]->pair.y;
  g->pc->position.x = rand()%g->rooms[room]->width+g->rooms[room]->pair.x;
  if(g->character[g->pc->position.y][g->pc->position.x]){
	g->nummon--;
	npc_delete(g->character[g->pc->position.y][g->pc->position.x]->npc);
  }
  g->character[g->pc->position.y][g->pc->position.x] = g->pc;
  g->pc->next_turn=0;
  binheap_insert(&g->next_turn, g->pc);
}

void go_downstairs(_game *g){
	clean_board(g);
	initialize(g);
	while(generate(g)!=1){
		generate(g);
	}
	connect_rooms(g);
	gen_monsters(g, g->nummon);
    gen_objects(g, 10);
	pc_new_floor(g);
}

void go_upstairs(_game *g){
	clean_board(g);
	initialize(g);
	while(generate(g)!=1){
		generate(g);
	}
	connect_rooms(g);
	gen_monsters(g, g->nummon);
    gen_objects(g, 10);
	pc_new_floor(g);
}

void view_stats(_game *g){
    erase();
    
    mvaddstr(26,70, "Player Stats");
    
    mvaddstr(28,70, "Name -> ");
    mvaddstr(28,78, g->pc->pc->name);
    mvaddstr(30,70, "Catchphrase -> ");
    mvaddstr(30,85, g->pc->pc->catch_phrase);
    
    char target_health[5];
    sprintf(target_health, "%d", pc_total_health(g));
    mvaddstr(32,70, "Health -> ");
    mvaddstr(32,80, target_health);
    
    char target_speed[5];
    sprintf(target_speed, "%d", pc_total_speed(g));
    mvaddstr(34,70, "Speed -> ");
    mvaddstr(34,79, target_speed);
    
    char target_damage[5];
    sprintf(target_damage, "%d", pc_total_damage(g));
    mvaddstr(36,70, "Damage -> ");
    mvaddstr(36,80, target_damage);
    
    mvaddstr(39,70,"Press ESC to go back");
   
    int done = 0;
    while(!done){
            char next = getch();
            if(next==27){
                print_view(g, 0);
                done=1;
            }
    }
}

void format_inventory(_game *g){
    uint32_t i;
    for(i = 0; i<g->pc->pc->inventory_size; i++){
        uint32_t j;
        for(j=i; j<g->pc->pc->inventory_size+1; j++){
            if(g->pc->pc->inventory[j]){
                g->pc->pc->inventory[i]=g->pc->pc->inventory[j];
                if(i!=j) g->pc->pc->inventory[j]=NULL;
                break;
            }
        }
    }
}

void print_error(int y, int x, char* message){
    mvaddstr(y,x,message);
}

void swap_object(_game *g, uint32_t index){
    object_t *o = g->pc->pc->inventory[index];
    if(get_type(o)==objtype_WEAPON){
        if(g->pc->pc->weapon){
            g->pc->pc->inventory[index]=g->pc->pc->weapon;
            g->pc->pc->weapon=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->weapon=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_OFFHAND){
        if(g->pc->pc->offhand){
            g->pc->pc->inventory[index]=g->pc->pc->offhand;
            g->pc->pc->offhand=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->offhand=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_RANGED){
        if(g->pc->pc->ranged){
            g->pc->pc->inventory[index]=g->pc->pc->ranged;
            g->pc->pc->ranged=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->ranged=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_ARMOR){
        if(g->pc->pc->armor){
            g->pc->pc->inventory[index]=g->pc->pc->armor;
            g->pc->pc->armor=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->armor=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_HELMET){
        if(g->pc->pc->helmet){
            g->pc->pc->inventory[index]=g->pc->pc->helmet;
            g->pc->pc->helmet=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->helmet=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_CLOAK){
        if(g->pc->pc->cloak){
            g->pc->pc->inventory[index]=g->pc->pc->cloak;
            g->pc->pc->cloak=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->cloak=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_GLOVES){
        if(g->pc->pc->gloves){
            g->pc->pc->inventory[index]=g->pc->pc->gloves;
            g->pc->pc->gloves=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->gloves=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_BOOTS){
        if(g->pc->pc->boots){
            g->pc->pc->inventory[index]=g->pc->pc->boots;
            g->pc->pc->boots=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->boots=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_AMULET){
        if(g->pc->pc->amulet){
            g->pc->pc->inventory[index]=g->pc->pc->amulet;
            g->pc->pc->amulet=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->amulet=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_LIGHT){
        if(g->pc->pc->light){
            g->pc->pc->inventory[index]=g->pc->pc->light;
            g->pc->pc->light=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->light=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
    }
    if(get_type(o)==objtype_RING){
        
        if(g->pc->pc->ring[0]){
            g->pc->pc->inventory[index]=g->pc->pc->ring[0];
            g->pc->pc->ring[0]=o;
        }
        else if(g->pc->pc->ring[1]){
            g->pc->pc->inventory[index]=g->pc->pc->ring[1];
            g->pc->pc->ring[1]=o;
        }
        else{
            g->pc->pc->inventory[index]=NULL;
            g->pc->pc->ring[0]=o;
            g->pc->pc->inventory_size--;
            format_inventory(g);
        }
        
    }
    
}

int take_off_item(_game *g, object_t *o){
    if(g->pc->pc->inventory_size<10){
        g->pc->pc->inventory[g->pc->pc->inventory_size++]=o;
        return 1;
    }
    return 0;
}

void wear_item(_game *g){
    erase();
    mvaddstr(0,70, "Pick an item to wear");
    mvaddstr(2,70, "0 -> ");
    mvaddstr(4,70, "1 -> ");
    mvaddstr(6,70, "2 -> ");
    mvaddstr(8,70, "3 -> ");
    mvaddstr(10,70, "4 -> ");
    mvaddstr(12,70, "5 -> ");
    mvaddstr(14,70, "6 -> ");
    mvaddstr(16,70, "7 -> ");
    mvaddstr(18,70, "8 -> ");
    mvaddstr(20,70, "9 -> ");
    mvaddstr(22,70, "Press ESC to go back");
    
    uint32_t i;
    for(i=0; i<g->pc->pc->inventory_size; i++){
        mvaddstr(2 + i*2, 75, get_name(g->pc->pc->inventory[i]));
    }
    for(i=g->pc->pc->inventory_size; i<10; i++){
        mvaddstr(2 + i*2, 75, "empty");
    }
    refresh();
    
    int done = 0;
    
    while(!done){
        char ch = getch();
        if(ch==27){
            print_view(g, 0);
            done=1;
        }
        else if(ch=='0'){
            if(g->pc->pc->inventory_size>0){
                swap_object(g, 0);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 0");
        }
        else if(ch=='1'){
            if(g->pc->pc->inventory_size>1){
                swap_object(g, 1);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 1");
        }
        else if(ch=='2'){
            if(g->pc->pc->inventory_size>2){
                swap_object(g, 2);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 2");
        }
        else if(ch=='3'){
            if(g->pc->pc->inventory_size>3){
                swap_object(g, 3);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 3");
        }
        else if(ch=='4'){
            if(g->pc->pc->inventory_size>4){
                swap_object(g, 4);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 4");
        }
        else if(ch=='5'){
            if(g->pc->pc->inventory_size>5){
                swap_object(g, 5);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 5");
        }
        else if(ch=='6'){
            if(g->pc->pc->inventory_size>6){
                swap_object(g, 6);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 6");
        }
        else if(ch=='7'){
            if(g->pc->pc->inventory_size>7){
                swap_object(g, 7);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 7");
        }
        else if(ch=='8'){
            if(g->pc->pc->inventory_size>8){
                swap_object(g, 8);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 8");
        }
        else if(ch=='9'){
            if(g->pc->pc->inventory_size>9){
                swap_object(g, 9);
                done=1;
            }
            else print_error(26, 70, "There is no object in slot 9");
        }
        else{
            print_error(24,70, "Invalid Entry");
        }
        refresh();
    }
    print_view(g, 0);
}

void remove_item(_game *g){
    erase();
    mvaddstr(0,70, "Pick an item to take off");
    mvaddstr(2,70, "a -> ");
    mvaddstr(4,70, "b -> ");
    mvaddstr(6,70, "c -> ");
    mvaddstr(8,70, "d -> ");
    mvaddstr(10,70, "e -> ");
    mvaddstr(12,70, "f -> ");
    mvaddstr(14,70, "g -> ");
    mvaddstr(16,70, "h -> ");
    mvaddstr(18,70, "i -> ");
    mvaddstr(20,70, "j -> ");
    mvaddstr(22,70, "k -> ");
    mvaddstr(24,70, "l -> ");
    mvaddstr(26,70, "Press ESC to go back");
    
    if(g->pc->pc->weapon) mvaddstr(2,75, get_name(g->pc->pc->weapon));
    else mvaddstr(2, 75, "empty");
    if(g->pc->pc->offhand) mvaddstr(4,75, get_name(g->pc->pc->offhand));
    else mvaddstr(4, 75, "empty");
    if(g->pc->pc->ranged) mvaddstr(6,75, get_name(g->pc->pc->ranged));
    else mvaddstr(6, 75, "empty");
    if(g->pc->pc->armor) mvaddstr(8,75, get_name(g->pc->pc->armor));
    else mvaddstr(8, 75, "empty");
    if(g->pc->pc->helmet) mvaddstr(10,75, get_name(g->pc->pc->helmet));
    else mvaddstr(10, 75, "empty");
    if(g->pc->pc->cloak) mvaddstr(12,75, get_name(g->pc->pc->cloak));
    else mvaddstr(12, 75, "empty");
    if(g->pc->pc->gloves) mvaddstr(14,75, get_name(g->pc->pc->gloves));
    else mvaddstr(14, 75, "empty");
    if(g->pc->pc->boots) mvaddstr(16,75, get_name(g->pc->pc->boots));
    else mvaddstr(16, 75, "empty");
    if(g->pc->pc->amulet) mvaddstr(18,75, get_name(g->pc->pc->amulet));
    else mvaddstr(18, 75, "empty");
    if(g->pc->pc->light) mvaddstr(20,75, get_name(g->pc->pc->light));
    else mvaddstr(20, 75, "empty");
    if(g->pc->pc->ring[0]) mvaddstr(22,75, get_name(g->pc->pc->ring[0]));
    else mvaddstr(22, 75, "empty");
    if(g->pc->pc->ring[1]) mvaddstr(24,75, get_name(g->pc->pc->ring[1]));
    else mvaddstr(24, 75, "empty");
    refresh();
    
    int done = 0;
    
    while(!done){
        char ch = getch();
        if(ch==27){
            print_view(g, 0);
            done=1;
        }
        else if(ch=='a'){
            if(g->pc->pc->weapon){
                if(take_off_item(g, g->pc->pc->weapon)){
                    g->pc->pc->weapon=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this weapon");
            }
            else print_error(30,70, "There is no item in slot a");
        }
        else if(ch=='b'){
            if(g->pc->pc->offhand){
                if(take_off_item(g, g->pc->pc->offhand)){
                    g->pc->pc->offhand=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot remove this offhand object");
            }
            else print_error(30,70, "There is no item in slot b");
        }
        else if(ch=='c'){
            if(g->pc->pc->ranged){
                if(take_off_item(g, g->pc->pc->ranged)){
                    g->pc->pc->ranged=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this ranged object");
            }
            else print_error(30,70, "There is no item in slot c");
        }
        else if(ch=='d'){
            if(g->pc->pc->armor){
                if(take_off_item(g, g->pc->pc->armor)){
                    g->pc->pc->armor=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this armor");
            }
            else print_error(30,70, "There is no item in slot d");
        }
        else if(ch=='e'){
            if(g->pc->pc->helmet){
                if(take_off_item(g, g->pc->pc->helmet)){
                    g->pc->pc->helmet=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this helmet");
            }
            else print_error(30,70, "There is no item in slot e");
        }
        else if(ch=='f'){
            if(g->pc->pc->cloak){
                if(take_off_item(g, g->pc->pc->cloak)){
                    g->pc->pc->cloak=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this cloak");
            }
            else print_error(30,70, "There is no item in slot f");
        }
        else if(ch=='g'){
            if(g->pc->pc->gloves){
                if(take_off_item(g, g->pc->pc->gloves)){
                    g->pc->pc->gloves=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off these gloves");
            }
            else print_error(30,70, "There is no item in slot g");
        }
        else if(ch=='h'){
            if(g->pc->pc->boots){
                if(take_off_item(g, g->pc->pc->boots)){
                    g->pc->pc->boots=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off these boots");
            }
            else print_error(30,70, "There is no item in slot h");
        }
        else if(ch=='i'){
            if(g->pc->pc->amulet){
                if(take_off_item(g, g->pc->pc->amulet)){
                    g->pc->pc->amulet=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this amulet");
            }
            else print_error(30,70, "There is no item in slot i");
        }
        else if(ch=='j'){
            if(g->pc->pc->light){
                if(take_off_item(g, g->pc->pc->light)){
                    g->pc->pc->light=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this light");
            }
            else print_error(30,70, "There is no item in slot j");
        }
        else if(ch=='k'){
            if(g->pc->pc->ring[0]){
                if(take_off_item(g, g->pc->pc->ring[0])){
                    g->pc->pc->ring[0]=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this ring");
            }
            else print_error(30,70, "There is no item in slot k");
        }
        else if(ch=='l'){
            if(g->pc->pc->ring[1]){
                if(take_off_item(g, g->pc->pc->ring[1])){
                    g->pc->pc->ring[1]=NULL;
                    done=1;
                }
                else print_error(30,70, "Inventory is full, cannot take off this ring");
            }
            else print_error(30,70, "There is no item in slot l");
        }
        else{
            print_error(28,70, "Invalid command");
        }
        refresh();
    }
    print_view(g, 0);
}

void drop_item(_game *g){
    erase();
    mvaddstr(0,70, "Pick an item to drop");
    mvaddstr(2,70, "0 -> ");
    mvaddstr(4,70, "1 -> ");
    mvaddstr(6,70, "2 -> ");
    mvaddstr(8,70, "3 -> ");
    mvaddstr(10,70, "4 -> ");
    mvaddstr(12,70, "5 -> ");
    mvaddstr(14,70, "6 -> ");
    mvaddstr(16,70, "7 -> ");
    mvaddstr(18,70, "8 -> ");
    mvaddstr(20,70, "9 -> ");
    mvaddstr(22,70, "Press ESC to go back");
    
    uint32_t i;
    for(i=0; i<g->pc->pc->inventory_size; i++){
        mvaddstr(2 + i*2, 75, get_name(g->pc->pc->inventory[i]));
    }
    for(i=g->pc->pc->inventory_size; i<10; i++){
        mvaddstr(2 + i*2, 75, "empty");
    }
    refresh();
    
    int done = 0;
    
    while(!done){
        char ch = getch();
        if(ch==27){
            done=1;
        }
        else if(ch=='0'){
            if(g->pc->pc->inventory_size>0){
                object_t *o = g->pc->pc->inventory[0];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[0]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='1'){
            if(g->pc->pc->inventory_size>1){
                object_t *o = g->pc->pc->inventory[1];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[1]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='2'){
            if(g->pc->pc->inventory_size>2){
                object_t *o = g->pc->pc->inventory[2];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[2]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='3'){
            if(g->pc->pc->inventory_size>3){
                object_t *o = g->pc->pc->inventory[3];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[3]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='4'){
            if(g->pc->pc->inventory_size>4){
                object_t *o = g->pc->pc->inventory[4];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[4]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='5'){
            if(g->pc->pc->inventory_size>5){
                object_t *o = g->pc->pc->inventory[5];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[5]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='6'){
            if(g->pc->pc->inventory_size>6){
                object_t *o = g->pc->pc->inventory[6];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[6]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='7'){
            if(g->pc->pc->inventory_size>7){
                object_t *o = g->pc->pc->inventory[7];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[7]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='8'){
            if(g->pc->pc->inventory_size>8){
                object_t *o = g->pc->pc->inventory[8];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[8]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else if(ch=='9'){
            if(g->pc->pc->inventory_size>9){
                object_t *o = g->pc->pc->inventory[9];
                if(!g->object[g->pc->position.y][g->pc->position.x]){
                    g->object[g->pc->position.y][g->pc->position.x]=o;
                    g->pc->pc->inventory[9]=NULL;
                    g->pc->pc->inventory_size--;
                    format_inventory(g);
                    done=1;
                }
                else print_error(26,70,"Cannot drop this object on top of another object");
            }
        }
        else{
            mvaddstr(24,70, "Invalid Entry");
        }
        refresh();
    }
    print_view(g, 0);
}

void expunge_item(_game *g){
    erase();
    mvaddstr(0,70, "Pick an item to expunge");
    mvaddstr(2,70, "0 -> ");
    mvaddstr(4,70, "1 -> ");
    mvaddstr(6,70, "2 -> ");
    mvaddstr(8,70, "3 -> ");
    mvaddstr(10,70, "4 -> ");
    mvaddstr(12,70, "5 -> ");
    mvaddstr(14,70, "6 -> ");
    mvaddstr(16,70, "7 -> ");
    mvaddstr(18,70, "8 -> ");
    mvaddstr(20,70, "9 -> ");
    mvaddstr(22,70, "Press ESC to go back");
    
    uint32_t i;
    for(i=0; i<g->pc->pc->inventory_size; i++){
        mvaddstr(2 + i*2, 75, get_name(g->pc->pc->inventory[i]));
    }
    for(i=g->pc->pc->inventory_size; i<10; i++){
        mvaddstr(2 + i*2, 75, "empty");
    }
    refresh();
    
    int done = 0;
    
    while(!done){
        char ch = getch();
        if(ch==27){
            done=1;
        }
        else if(ch=='0'){
            if(g->pc->pc->inventory_size>0){
                delete_object(g->pc->pc->inventory[0]);
                g->pc->pc->inventory[0]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='1'){
            if(g->pc->pc->inventory_size>1){
                delete_object(g->pc->pc->inventory[1]);
                g->pc->pc->inventory[1]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='2'){
            if(g->pc->pc->inventory_size>2){
                delete_object(g->pc->pc->inventory[2]);
                g->pc->pc->inventory[2]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='3'){
            if(g->pc->pc->inventory_size>3){
                delete_object(g->pc->pc->inventory[3]);
                g->pc->pc->inventory[3]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='4'){
            if(g->pc->pc->inventory_size>4){
                delete_object(g->pc->pc->inventory[4]);
                g->pc->pc->inventory[4]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='5'){
            if(g->pc->pc->inventory_size>5){
                delete_object(g->pc->pc->inventory[5]);
                g->pc->pc->inventory[5]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='6'){
            if(g->pc->pc->inventory_size>6){
                delete_object(g->pc->pc->inventory[6]);
                g->pc->pc->inventory[6]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='7'){
            if(g->pc->pc->inventory_size>7){
                delete_object(g->pc->pc->inventory[7]);
                g->pc->pc->inventory[7]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='8'){
            if(g->pc->pc->inventory_size>8){
                delete_object(g->pc->pc->inventory[8]);
                g->pc->pc->inventory[8]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(56,70,"No item in specified slot");
        }
        else if(ch=='9'){
            if(g->pc->pc->inventory_size>9){
                delete_object(g->pc->pc->inventory[9]);
                g->pc->pc->inventory[9]=NULL;
                g->pc->pc->inventory_size--;
                format_inventory(g);
                done=1;
            }
            else print_error(26,70,"No item in specified slot");
        }
        else{
            mvaddstr(24,70, "Invalid Entry");
        }
        refresh();
    }
    print_view(g, 0);
}

void pc_next_pos(_game *g)
{
	pair_t next_move = {g->pc->position.x, g->pc->position.y};
	while(next_move.x==g->pc->position.x && next_move.y==g->pc->position.y){
		char next = getch();
		if(next=='7' || next=='y'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y-1][next_move.x-1].terrain!=ter_wall_immutable && g->dungeon[next_move.y-1][next_move.x-1].terrain!=ter_wall){
					next_move.x = next_move.x-1;
					next_move.y = next_move.y-1;
				}
			}
		}
		else if(next=='8' || next=='k'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y-1][next_move.x].terrain!=ter_wall_immutable && g->dungeon[next_move.y-1][next_move.x].terrain!=ter_wall){
					next_move.y = next_move.y-1;
				}
			}
			else{
				print_view(g, 1);
			}
		}
		else if(next=='9' || next=='u'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y-1][next_move.x+1].terrain!=ter_wall_immutable && g->dungeon[next_move.y-1][next_move.x+1].terrain!=ter_wall){
					next_move.x = next_move.x+1;
					next_move.y = next_move.y-1;
				}
			}
		}
		else if(next=='6' || next=='l'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y][next_move.x+1].terrain!=ter_wall_immutable && g->dungeon[next_move.y][next_move.x+1].terrain!=ter_wall){
					next_move.x = next_move.x+1;
				}
			}
			else{
				print_view(g, 2);
			}
		}
		else if(next=='3' || next=='n'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y+1][next_move.x+1].terrain!=ter_wall_immutable && g->dungeon[next_move.y+1][next_move.x+1].terrain!=ter_wall){
					next_move.x = next_move.x+1;
					next_move.y = next_move.y+1;
				}
			}
		}
		else if(next=='2' || next=='j'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y+1][next_move.x].terrain!=ter_wall_immutable && g->dungeon[next_move.y+1][next_move.x].terrain!=ter_wall){
					next_move.y = next_move.y+1;
				}
			}
			else{
				print_view(g, 3);
			}
		}
		else if(next=='1' || next=='b'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y+1][next_move.x-1].terrain!=ter_wall_immutable && g->dungeon[next_move.y+1][next_move.x-1].terrain!=ter_wall){
					next_move.x = next_move.x-1;
					next_move.y = next_move.y+1;
				}
			}
		}
		else if(next=='4' || next=='h'){
			if(g->pc->pc->mode==0){
				if(g->dungeon[next_move.y][next_move.x-1].terrain!=ter_wall_immutable && g->dungeon[next_move.y][next_move.x-1].terrain!=ter_wall){
					next_move.x = next_move.x-1;
				}
			}
			else{
				print_view(g, 4);
			}
		}
		else if(next=='>'){
			if(g->pc->pc->mode==0 && g->dungeon[g->pc->position.y][g->pc->position.x].terrain==ter_stair_down){
				go_downstairs(g);
				next_move = g->pc->position;
				print_view(g, 0);
			}
		}
		else if(next=='<'){
			if(g->pc->pc->mode==0 && g->dungeon[g->pc->position.y][g->pc->position.x].terrain==ter_stair_up){
				go_upstairs(g);
				next_move = g->pc->position;
				print_view(g, 0);
			}
		}
		else if(next==' '){
			if(g->pc->pc->mode==0){
				break;
			}
		}
		else if(next=='L'){
			if(g->pc->pc->mode==0){
				g->pc->pc->mode=1;
			}
		}
		else if(next==27){
			if(g->pc->pc->mode==1){
				g->pc->pc->mode=0;
				print_view(g, 0);
			}
		}
		else if(next=='S'){
			save(g);
			binheap_delete(&g->next_turn);
			exit(1);
		}
        else if(next=='w'){
			wear_item(g);
		}
        else if(next=='t'){
			remove_item(g);
		}
        else if(next=='d'){
			drop_item(g);
		}
        else if(next=='x'){
			expunge_item(g);
		}
        else if(next=='s'){
            view_stats(g);
        }
	}
	g->character[g->pc->position.y][g->pc->position.x]=NULL;
	if(g->character[next_move.y][next_move.x] && g->character[next_move.y][next_move.x]!=g->pc){
        g->character[next_move.y][next_move.x]->health-=pc_total_damage(g);
        if(g->character[next_move.y][next_move.x]->health<=0){
            g->character[next_move.y][next_move.x]->alive=0;
            //character_delete(g->character[next_move.y][next_move.x]);
            g->nummon--;
            g->pc->position = next_move;
            g->character[g->pc->position.y][g->pc->position.x]=g->pc;
        }
        else{
            g->character[g->pc->position.y][g->pc->position.x]=g->pc;
        }
	}
    else{
        g->pc->position = next_move;
        g->character[g->pc->position.y][g->pc->position.x]=g->pc;
    }
    
    if(g->object[next_move.y][next_move.x] && g->pc->pc->inventory_size<10){
        g->pc->pc->inventory[g->pc->pc->inventory_size++] = g->object[next_move.y][next_move.x];
        g->object[next_move.y][next_move.x]=NULL;
    }
}

int32_t pc_total_speed(_game *g){
    int32_t total_speed = g->pc->speed;
    if(g->pc->pc->weapon) total_speed+=get_speed(g->pc->pc->weapon);
    if(g->pc->pc->offhand) total_speed+=get_speed(g->pc->pc->offhand);
    if(g->pc->pc->ranged) total_speed+=get_speed(g->pc->pc->ranged);
    if(g->pc->pc->armor) total_speed+=get_speed(g->pc->pc->armor);
    if(g->pc->pc->helmet) total_speed+=get_speed(g->pc->pc->helmet);
    if(g->pc->pc->cloak) total_speed+=get_speed(g->pc->pc->cloak);
    if(g->pc->pc->gloves) total_speed+=get_speed(g->pc->pc->gloves);
    if(g->pc->pc->boots) total_speed+=get_speed(g->pc->pc->boots);
    if(g->pc->pc->amulet) total_speed+=get_speed(g->pc->pc->amulet);
    if(g->pc->pc->light) total_speed+=get_speed(g->pc->pc->light);
    if(g->pc->pc->ring[0]) total_speed+=get_speed(g->pc->pc->ring[0]);
    if(g->pc->pc->ring[1]) total_speed+=get_speed(g->pc->pc->ring[1]);
    return total_speed;
}

int32_t pc_total_health(_game *g){
    int32_t total_health = g->pc->health;
    if(g->pc->pc->weapon) total_health+=get_health(g->pc->pc->weapon);
    if(g->pc->pc->offhand) total_health+=get_health(g->pc->pc->offhand);
    if(g->pc->pc->ranged) total_health+=get_health(g->pc->pc->ranged);
    if(g->pc->pc->armor) total_health+=get_health(g->pc->pc->armor);
    if(g->pc->pc->helmet) total_health+=get_health(g->pc->pc->helmet);
    if(g->pc->pc->cloak) total_health+=get_health(g->pc->pc->cloak);
    if(g->pc->pc->gloves) total_health+=get_health(g->pc->pc->gloves);
    if(g->pc->pc->boots) total_health+=get_health(g->pc->pc->boots);
    if(g->pc->pc->amulet) total_health+=get_health(g->pc->pc->amulet);
    if(g->pc->pc->light) total_health+=get_health(g->pc->pc->light);
    if(g->pc->pc->ring[0]) total_health+=get_health(g->pc->pc->ring[0]);
    if(g->pc->pc->ring[1]) total_health+=get_health(g->pc->pc->ring[1]);
    return total_health;
}

int32_t pc_total_damage(_game *g){
    int32_t total_damage = roll_dice(g->pc->damage);
    if(g->pc->pc->weapon) total_damage+=roll_dice(get_damage(g->pc->pc->weapon));
    if(g->pc->pc->offhand) total_damage+=roll_dice(get_damage(g->pc->pc->offhand));
    if(g->pc->pc->ranged) total_damage+=roll_dice(get_damage(g->pc->pc->ranged));
    if(g->pc->pc->armor) total_damage+=roll_dice(get_damage(g->pc->pc->armor));
    if(g->pc->pc->helmet) total_damage+=roll_dice(get_damage(g->pc->pc->helmet));
    if(g->pc->pc->cloak) total_damage+=roll_dice(get_damage(g->pc->pc->cloak));
    if(g->pc->pc->gloves) total_damage+=roll_dice(get_damage(g->pc->pc->gloves));
    if(g->pc->pc->boots) total_damage+=roll_dice(get_damage(g->pc->pc->boots));
    if(g->pc->pc->amulet) total_damage+=roll_dice(get_damage(g->pc->pc->amulet));
    if(g->pc->pc->light) total_damage+=roll_dice(get_damage(g->pc->pc->light));
    if(g->pc->pc->ring[0]) total_damage+=roll_dice(get_damage(g->pc->pc->ring[0]));
    if(g->pc->pc->ring[1]) total_damage+=roll_dice(get_damage(g->pc->pc->ring[1]));
    return total_damage;
}
