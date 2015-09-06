#ifndef OBJECT_DESCRIPTIONS_H
#define OBJECT_DESCRIPTIONS_H

#include <stdint.h>

typedef struct game _game;

# ifdef __cplusplus

# include <string>
# include "dice.h"

extern "C" {
# endif

uint32_t parse_object_descriptions(_game *g, char* filepath);
uint32_t print_object_descriptions(_game *g);
uint32_t destroy_object_descriptions(_game *g);
void generate_objects(_game *g, uint32_t numobj);

# ifdef __cplusplus
} /* extern "C" */

class object_description {
 private:
  std::string name, description;
  char symbol;
  uint32_t color, type;
  dice hitpoints, damage, dodge, defense, speed, attribute, value, weight;
 public:
  object_description() : name(),   description(),  symbol(0),   color(0),
                          type(0),  hitpoints(),    damage(),    dodge(),
                          defense(),speed(),        attribute(), value(),
                          weight()
  {
  }
  void set(const std::string &name,
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
           const dice &weight);
  std::string getName();
  std::string getDescription();
  char getSymbol();
  uint32_t getColor();
  uint32_t getType();
  uint32_t getHitpoints();
  uint32_t getDamage();
  uint32_t getDodge();
  uint32_t getDefense();
  uint32_t getSpeed();
  uint32_t getAttribute();
  uint32_t getValue();
  uint32_t getWeight();
  std::ostream &print(std::ostream &o);
};

# endif

#endif