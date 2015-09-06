#ifndef OBJECT_H
# define OBJECT_H

#include "dice.h"

# ifdef __cplusplus

# include <string>

# include "descriptions.h"
# include "pair.h"

extern "C" {
# endif

typedef struct game _game;
typedef struct object_t object_t;
typedef enum object_type object_type_t;

void delete_object(object_t *o);
void gen_objects(_game *g, uint32_t numobj);
char get_symbol(object_t *o);
uint32_t get_color(object_t *o);
char* get_name(object_t *o);
int32_t get_speed(object_t *o);
int32_t get_health(object_t *o);
pair_t get_position(object_t *o);
object_type_t get_type(object_t *o);
void destroy_objects(_game *g);
const dice_t *get_damage(object_t *o);

# ifdef __cplusplus

} /* extern "C" */

class object {
 private:
  const std::string &name;
  const std::string &description;
  object_type_t type;
  uint32_t color;
  pair_t position;
  const dice &damage;
  int32_t hit, dodge, defense, weight, speed, attribute, value;
  object *next;
 public:
  object(const object_description &o, pair_t p, object *next);
  ~object();

  friend char get_symbol(object_t *o);
  friend uint32_t get_color(object_t *o);
  friend char* get_name(object_t *o);
  friend int32_t get_speed(object_t *o);
  friend pair_t get_position(object_t *o);
  friend object_type_t get_type(object_t *o);
  friend int32_t get_health(object_t *o);
  friend const dice_t *get_damage(object_t *o);
};

# endif

#endif