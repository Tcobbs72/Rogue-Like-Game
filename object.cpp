#include <vector>
#include <ncurses.h> /* for COLOR_BLACK and COLOR_WHITE */
#include <stdio.h>
#include <stdlib.h>
#include "object.h"
#include "dungeon.h"

object::object(const object_description &o, pair_t p, object *next) :
  name(o.get_name()),
  description(o.get_description()),
  type(o.get_type()),
  color(o.get_color()),
  damage(o.get_damage()),
  hit(o.get_hit().roll()),
  dodge(o.get_dodge().roll()),
  defense(o.get_defence().roll()),
  weight(o.get_weight().roll()),
  speed(o.get_speed().roll()),
  attribute(o.get_attribute().roll()),
  value(o.get_value().roll()),
  next(next)
{
  position.x = p.x;
  position.y = p.y;
}

object::~object()
{
  if (next) {
    delete next;
  }
}

void delete_object(object_t *o){
    delete ((object *) o);
}

void gen_object(_game *g)
{
  object *o;
  const std::vector<object_description> &v =
    *((std::vector<object_description> *) g->object_descriptions);
  const object_description &od = v[rand()%v.size()];
  uint32_t room;
  pair_t p;

  room = rand()%totalRooms;
  do {
    p.y = rand()%(g->rooms[room]->height) + g->rooms[room]->pair.y;
    p.x = rand()%(g->rooms[room]->width) + g->rooms[room]->pair.x;
  } while (g->character[p.y][p.x]);

  o = new object(od, p, (object *) g->object[p.y][p.x]);

  g->object[p.y][p.x] = (object_t *) o;  
}

void gen_objects(_game *g, uint32_t numobj)
{
  uint32_t i;

  g->numobj = numobj;
  for (i = 0; i < numobj; i++) {
    gen_object(g);
  }
}

char get_symbol(object_t *o)
{
  return ((object *) o)->next ? '&' : object_symbol[((object *) o)->type];
}

uint32_t get_color(object_t *o)
{
  return (((object *) o)->color == COLOR_BLACK ?
          COLOR_WHITE                          :
          ((object *) o)->color);
}

char* get_name(object_t *o){
    return ((char*) ((object *)o)->name.c_str());
}

int32_t get_speed(object_t *o){
    return ((object *)o)->speed;
}

int32_t get_health(object_t *o){
    return ((object *)o)->hit;
}

pair_t get_position(object_t *o){
    return ((object *)o)->position;
}

const dice_t *get_damage(object_t *o){
    dice d = ((object *)o)->damage;
    return new_dice(d.get_base(), d.get_number(), d.get_sides());
}

object_type_t get_type(object_t *o){
    return ((object *)o)->type;
}

void destroy_objects(_game *g)
{
  uint32_t y, x;

  for (y = 0; y < Y; y++) {
    for (x = 0; x < X; x++) {
      if (g->object[y][x]) {
        delete (object *) g->object[y][x];
        g->object[y][x] = 0;
      }
    }
  }
}

