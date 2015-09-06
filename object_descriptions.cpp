#include <string>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <limits.h>
#include <ncurses.h>
#include <vector>
#include <sstream>
#include <cstdlib>

#include "object.h"
#include "object_descriptions.h"
#include "dungeon.h"
#include "dice.h"

#define OBJECT_FILE_SEMANTIC          "RLG229 OBJECT DESCRIPTION"
#define OBJECT_FILE_VERSION           1U
#define NUM_OBJECT_DESCRIPTION_FIELDS 12

static const struct {
  const char *name;
  const uint32_t value;
} type_lookup[] = {
  { "WEAPON",   OBJ_WEAPON },
  { "OFFHAND",  OBJ_OFFHAND     },
  { "RANGED",   OBJ_RANGED  },
  { "ARMOR",    OBJ_ARMOR    },
  { "HELMET",   OBJ_HELMET    },
  { "CLOAK",    OBJ_CLOAK    },
  { "GLOVES",   OBJ_GLOVES    },
  { "BOOTS",    OBJ_BOOTS    },
  { "RING",     OBJ_RING    },
  { "AMULET",   OBJ_AMULET    },
  { "LIGHT",    OBJ_LIGHT    },
  { "SCROLL",   OBJ_SCROLL    },
  { "BOOK",     OBJ_BOOK    },
  { "FLASK",    OBJ_FLASK    },
  { "GOLD",     OBJ_GOLD    },
  { "AMMUNITION", OBJ_AMMUNITION    },
  { "FOOD",     OBJ_FOOD    },
  { "WAND",     OBJ_WAND    },
  { "CONTAINER", OBJ_CONTAINER    },
  { "STACK",    OBJ_STACK   },
  { 0,        0             }
};

static const struct {
  const uint32_t type;
  const char value;
} symb_lookup[] = {
  { OBJ_WEAPON,     '|' },
  { OBJ_OFFHAND,    ')'    },
  { OBJ_RANGED,     '}'  },
  { OBJ_ARMOR,      '['   },
  { OBJ_HELMET,     ']'   },
  { OBJ_CLOAK,      '('    },
  { OBJ_GLOVES,     '{'    },
  { OBJ_BOOTS,      '/'    },
  { OBJ_RING,       '='    },
  { OBJ_AMULET,     '"'    },
  { OBJ_LIGHT,      '_'   },
  { OBJ_SCROLL,     '~'    },
  { OBJ_BOOK,       '?'    },
  { OBJ_FLASK,      '!'    },
  { OBJ_GOLD,       '$'    },
  { OBJ_AMMUNITION, '/'    },
  { OBJ_FOOD,       ','   },
  { OBJ_WAND,       '-'    },
  { OBJ_CONTAINER,  '%'    },
  { OBJ_STACK,      '&'     },
  { 0,        0             }
};

#define color_lu_entry(color) { #color, COLOR_##color }
static const struct {
  const char *name;
  const uint32_t value;
} colors_lookup[] = {
  /* Same deal here as above in abilities_lookup definition. */
  /* We can use this convenient macro here, but we can't use a *
   * similar macro above because of PASS and TELE.             */
  color_lu_entry(BLACK),
  color_lu_entry(BLUE),
  color_lu_entry(CYAN),
  color_lu_entry(GREEN),
  color_lu_entry(MAGENTA),
  color_lu_entry(RED),
  color_lu_entry(WHITE),
  color_lu_entry(YELLOW),
  { 0, 0 }
};

static inline void eat_whitespace(std::ifstream &f)
{
  while (isspace(f.peek())) {
    f.get();
  }  
}

static inline void eat_blankspace(std::ifstream &f)
{
  while (isblank(f.peek())) {
    f.get();
  }  
}

static uint32_t parse_object_name(std::ifstream &f,
                                   std::string *lookahead,
                                   std::string *name)
{
  /* Always start by eating the blanks.  If we then find a newline, we *
   * know there's an error in the file.  If we eat all whitespace,     *
   * we'd consume newlines and perhaps miss a restart on the next      *
   * line.                                                             */

  eat_blankspace(f);

  if (f.peek() == '\n') {
    return 1;
  }

  getline(f, *name);

  /* We enter this function with the semantic in the lookahead, so we  *
   * read a new one so that we're in the same state for the next call. */
  f >> *lookahead;

  return 0;
}

static uint32_t parse_object_color(std::ifstream &f,
                                    std::string *lookahead,
                                    uint32_t *color)
{
  uint32_t i;

  *color = UINT_MAX;

  eat_blankspace(f);

  if (f.peek() == '\n') {
    return 1;
  }

  f >> *lookahead;

  for (i = 0; colors_lookup[i].name; i++) {
    if (*lookahead == colors_lookup[i].name) {
      *color = colors_lookup[i].value;
      break;
    }
  }

  if (!colors_lookup[i].name) {
    return 1;
  }

  eat_blankspace(f);
  if (f.peek() != '\n') {
    return 1;
  }

  f >> *lookahead;

  return 0;
}

static uint32_t parse_object_desc(std::ifstream &f,
                                   std::string *lookahead,
                                   std::string *desc)
{
  /* DESC is special.  Data doesn't follow on the same line *
   * as the keyword, so we want to eat the newline, too.    */
  eat_blankspace(f);

  if (f.peek() != '\n') {
    return 1;
  }

  f.get();

  while (f.peek() != EOF) {
    getline(f, *lookahead);
    if (lookahead->length() > 77) {
      return 1;
    }

    lookahead->push_back('\n');

    if (*lookahead == ".\n") {
      break;
    }

    *desc += *lookahead;
  }

  /* Strip off the trailing newline */
  desc->erase(desc->length() - 1);

  if (*lookahead != ".\n") {
    return 1;
  }

  f >> *lookahead;

  return 0;
}

static uint32_t parse_dice(std::ifstream &f,
                           std::string *lookahead,
                           dice *d)
{
  int32_t base;
  uint32_t number, sides;

  eat_blankspace(f);

  if (f.peek() == '\n') {
    return 1;
  }

  f >> *lookahead;

  if (sscanf(lookahead->c_str(), "%d+%ud%u", &base, &number, &sides) != 3) {
    return 1;
  }

  d->set(base, number, sides);

  f >> *lookahead;

  return 0;
}

static uint32_t parse_object_dice(std::ifstream &f,
                                    std::string *lookahead,
                                    dice *d)
{
  return parse_dice(f, lookahead, d);
}

static uint32_t parse_object_type(std::ifstream &f,
                                   std::string *lookahead,
                                   uint32_t *type)
{
  uint32_t i;

  *type = 0;

  eat_blankspace(f);

  if (f.peek() == '\n') {
    return 1;
  }

  /* Will not lead to error if an ability is listed multiple times. */
  while (f.peek() != '\n') {
    f >> *lookahead;

    for (i = 0; type_lookup[i].name; i++) {
      if (*lookahead == type_lookup[i].name) {
        *type |= type_lookup[i].value;
        break;
      }
    }

    if (!type_lookup[i].name) {
      return 1;
    }

    eat_blankspace(f);
  }

  f >> *lookahead;

  return 0;
}

static uint32_t parse_object_description(std::ifstream &f,
                                          std::string *lookahead,
                                          std::vector<object_description> *v)
{
  std::string s;
  bool read_name, read_color, read_desc,
       read_speed, read_dam, read_hp, read_attr,
       read_type, read_dodge, read_def, read_val,
       read_weight;
  std::string name, desc;
  uint32_t color, type;
  dice speed, dam, hp, dodge, def, val, weight, attr;
  object_description m;
  int count;

  read_name = read_color = read_desc =
              read_speed = read_dam = read_hp = read_attr = 
              read_type = read_dodge = read_def = read_val = 
              read_weight = false;

  if (*lookahead != "BEGIN") {
    std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
              << "Parse error in object description.\n"
              << "Discarding object." << std::endl;
    do {
      f >> *lookahead;
    } while (*lookahead != "BEGIN" && f.peek() != EOF);
  }
  if (f.peek() == EOF) {
    return 1;
  }
  f >> *lookahead;
  if (*lookahead != "OBJECT") {
    return 1;
  }

  for (f >> *lookahead, count = 0;
       count < NUM_OBJECT_DESCRIPTION_FIELDS;
       count++) {
    /* This could definately be more concise. */
    if        (*lookahead == "NAME")  {
      if (read_name || parse_object_name(f, lookahead, &name)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object name.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_name = true;
    } else if (*lookahead == "DESC")  {
      if (read_desc || parse_object_desc(f, lookahead, &desc)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object description(1).\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_desc = true;
    } else if (*lookahead == "COLOR") {
      if (read_color || parse_object_color(f, lookahead, &color)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object color.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_color = true;
    } else if (*lookahead == "SPEED") {
      if (read_speed || parse_object_dice(f, lookahead, &speed)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object speed.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_speed = true;
    } else if (*lookahead == "TYPE")  {
      if (read_type || parse_object_type(f, lookahead, &type)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object type.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_type = true;
    } else if (*lookahead == "HIT")    {
      if (read_hp || parse_object_dice(f, lookahead, &hp)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object hitpoints.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_hp = true;
    } else if (*lookahead == "DAM")   {
      if (read_dam || parse_object_dice(f, lookahead, &dam)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object damage.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_dam = true;
    } else if (*lookahead == "DODGE")   {
      if (read_dodge || parse_object_dice(f, lookahead, &dodge)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object dodge.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_dodge = true;
    } else if (*lookahead == "WEIGHT")   {
      if (read_weight || parse_object_dice(f, lookahead, &weight)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object weight.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_weight = true;
    } else if (*lookahead == "VAL")   {
      if (read_val || parse_object_dice(f, lookahead, &val)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object value.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_val = true;
    } else if (*lookahead == "DEF")   {
      if (read_def || parse_object_dice(f, lookahead, &def)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object defense.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_def = true;
    } else if (*lookahead == "ATTR")   {
      if (read_attr || parse_object_dice(f, lookahead, &attr)) {
        std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                  << "Parse error in object attribute.\n"
                  << "Discarding object." << std::endl;
        return 1;
      }
      read_attr = true;
    } else                           {
      std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
                << "Parse error in object description.\n"
                << "Discarding object." << std::endl;
      return 1;
    }
  }

  if (*lookahead != "END") {
    return 1;
  }

  eat_blankspace(f);
  if (f.peek() != '\n' && f.peek() != EOF) {
    return 1;
  }
  f >> *lookahead;

  m.set(name, desc, color, type, hp, dam, dodge, def, speed, attr, val, weight);
  v->push_back(m);

  return 0;
}

static uint32_t parse_object_descriptions(std::ifstream &f,
                                           _game *g,
                                           std::vector<object_description> *v)
{
  std::string s;
  std::stringstream expected;
  std::string lookahead;

  expected << OBJECT_FILE_SEMANTIC << " " << OBJECT_FILE_VERSION;

  eat_whitespace(f);

  getline(f, s);

  if (s != expected.str()) {
    std::cerr << "Discovered at " << __FILE__ << ":" << __LINE__ << "\n"
              << "Parse error in object description file.\nExpected: \""
              << expected.str() << "\"\nRead:     \"" << s << "\"\n\nAborting."
              << std::endl;
    return 1;
  }

  f >> lookahead;
  do {
    parse_object_description(f, &lookahead, v);
  } while (f.peek() != EOF);

  return 0;
}

uint32_t parse_object_descriptions(_game *g, char* filepath)
{
  std::ifstream f;
  std::vector<object_description> *v;
  uint32_t retval;

  retval = 0;

  f.open(filepath);

  v = new std::vector<object_description>();
  g->object_descriptions = v;

  if (parse_object_descriptions(f, g, v)) {
    delete v;
    g->object_descriptions = 0;
    retval = 1;
  }

  f.close();

  return retval;
}

uint32_t print_object_descriptions(_game *g)
{
  std::vector<object_description> *v;
  std::vector<object_description>::iterator i;

  v = (std::vector<object_description> *) g->object_descriptions;

  for (i = v->begin(); i != v->end(); i++) {
    i->print(std::cout);
  }

  return 0;
}

 void object_description::set(const std::string &name,
                               const std::string &description,
                               const uint32_t color,
                               const uint32_t type,
                               const dice &hitpoints,
                               const dice &damage,
                               const dice &dodge,
                               const dice &defense,
                               const dice &speed,
                               const dice &attribute,
                               const dice &value,
                               const dice &weight)
{
  this->name = name;
  this->description = description;
  this->color = color;
  this->type=type;
  this->hitpoints = hitpoints;
  this->damage = damage;
  this->dodge = dodge;
  this->defense = defense;
  this->speed = speed;
  this->attribute = attribute;
  this->value = value;
  this->weight = weight;
  
  uint32_t i;
  for (i = 0; symb_lookup[i].type; i++) {
      if (type == symb_lookup[i].type) {
        this->symbol = symb_lookup[i].value;
        break;
      }
  }
    
}

std::string object_description::getName(){
    return name;
}

std::string object_description::getDescription(){
    return description;
}

char object_description::getSymbol(){
    return symbol;
}

uint32_t object_description::getColor(){
    return color;
}

uint32_t object_description::getType(){
    return type;
}

uint32_t object_description::getHitpoints(){
    return hitpoints.roll();
}

uint32_t object_description::getDamage(){
    return damage.roll();
}

uint32_t object_description::getDodge(){
    return dodge.roll();
}

uint32_t object_description::getDefense(){
    return defense.roll();
}

uint32_t object_description::getSpeed(){
    return speed.roll();
}

uint32_t object_description::getAttribute(){
    return attribute.roll();
}

uint32_t object_description::getValue(){
    return value.roll();
}

uint32_t object_description::getWeight(){
    return weight.roll();
}

std::ostream &object_description::print(std::ostream& o)
{
  uint32_t i;

  o << name << std::endl;
  o << description << std::endl;
  o << symbol << std::endl;
  for (i = 0; colors_lookup[i].name; i++) {
    if (color == colors_lookup[i].value) {
      o << colors_lookup[i].name << std::endl;
      break;
    }
  }
  for (i = 0; type_lookup[i].name; i++) {
    if (type & type_lookup[i].value) {
      o << type_lookup[i].name << " ";
    }
  }
  
  o << std::endl << "HITPOINTS: ";
  hitpoints.print(o);
  o << std::endl << "DAMAGE: ";
  damage.print(o);
  o << std::endl << "DEFENSE: ";
  defense.print(o);
  o << std::endl << "SPEED: ";
  speed.print(o);
  o << std::endl << "DODGE: ";
  dodge.print(o);
  o << std::endl << "ATTRIBUTE: ";
  attribute.print(o);
  o << std::endl << "WEIGHT: ";
  value.print(o);
  o << std::endl << "SELL VALUE: ";
  value.print(o);
  o << std::endl;
  o << std::endl;

  return o;
}

uint32_t destroy_object_descriptions(_game *g)
{
  delete (std::vector<object_description> *) g->object_descriptions;
  g->object_descriptions = NULL;

  return 0;
}

void generate_objects(_game *g, uint32_t numobj){
  uint32_t room;
  uint32_t i;
  pair_t p;
    
  std::vector<object_description> *v;

  v = (std::vector<object_description> *) g->object_descriptions;

  for(i=0; i<numobj; i++){
    uint32_t index = rand() % v->size();
    object_description obj = (*v)[index];
	
    object_t *o;
    o = (object_t*) malloc(sizeof(*o));
    
    o->name = (char*)obj.getName().c_str();
    o->description = (char*)obj.getDescription().c_str();
	o->symbol=obj.getSymbol();
    o->color = obj.getColor();
    o->characteristics = obj.getType();
    o->hitpoints = obj.getHitpoints();
    o->damage = obj.getDamage();
    o->dodge = obj.getDodge();
    o->defense = obj.getDefense();
    o->speed = obj.getSpeed();
    o->attribute = obj.getAttribute();
    o->value = obj.getValue();
    o->weight = obj.getWeight();
    

	room = rand()%totalRooms;
	do {
	  p.y = rand()%(g->rooms[room]->height) + g->rooms[room]->pair.y;
	  p.x = rand()%(g->rooms[room]->width) + g->rooms[room]->pair.x;
	} while (g->object[p.y][p.x]);
	o->position.y = p.y;
	o->position.x = p.x;

	g->object[p.y][p.x] = o;
   // printf("Placing a %s at (%d,%d)\n", o->name, o->position.x, o->position.y);
  }
}
