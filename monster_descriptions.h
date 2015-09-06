#ifndef MONSTER_DESCRIPTIONS_H
# define MONSTER_DESCRIPTIONS_H

# include <stdint.h>

typedef struct game _game;

# ifdef __cplusplus

# include <string>
# include "dice.h"

extern "C" {
# endif

uint32_t parse_monster_descriptions(_game *g, char* filepath);
uint32_t print_monster_descriptions(_game *g);
uint32_t destroy_monster_descriptions(_game *g);
void generate_monsters(_game *g, uint32_t nummon);

# ifdef __cplusplus
} /* extern "C" */

class monster_description {
 private:
  std::string name, description;
  char symbol;
  uint32_t color, abilities;
  dice speed, hitpoints, damage;
 public:
  monster_description() : name(),       description(), symbol(0),   color(0),
                          abilities(0), speed(),       hitpoints(), damage()
  {
  }
  void set(const std::string &name,
           const std::string &description,
           const char symbol,
           const uint32_t color,
           const dice &speed,
           const uint32_t abilities,
           const dice &hitpoints,
           const dice &damage);
  std::string getName();
  std::string getDescription();
  char getSymbol();
  uint32_t getColor();
  uint32_t getAbilities();
  dice getSpeed();
  dice getHitpoints();
  dice getDamage();
  std::ostream &print(std::ostream &o);
};

class monster {
 private:
  std::string name, description;
  char symbol;
  uint32_t color, abilities, speed, hitpoints, damage;
 public:
  monster() : name(), description(), symbol(0), color(0), abilities(0), speed(0), hitpoints(0), damage(0)
  {
  }
  void set(const std::string &name,
           const std::string &description,
           const char symbol,
           const uint32_t color,
           const uint32_t speed,
           const uint32_t abilities,
           const uint32_t hitpoints,
           const uint32_t damage);
  std::string getName();
  std::string getDescription();
  char getSymbol();
  uint32_t getColor();
  uint32_t getAbilities();
  uint32_t getSpeed();
  uint32_t getHitpoints();
  uint32_t getDamage();
  std::ostream &print(std::ostream &o);
};

# endif

#endif
