CC = gcc
CXX = g++
ECHO = echo
RM = rm -f

FLAGS = -Wall -Werror -ggdb
LDFLAGS = -lncurses

BIN = HW10
OBJS = binheap.o character.o descriptions.o dice.o npc.o pc.o object.o dungeon.o

all: $(BIN)

$(BIN): $(OBJS)
		@$(ECHO) Linking $@
		@$(CXX) $^ -o $@ $(LDFLAGS)
		
-include $(OBJS:.o=.d)

%.o: %.c
		@$(ECHO) Compiling $<
		@$(CC) $(FLAGS) -MMD -MF $*.d -c $<
		
%.o: %.cpp
		@$(ECHO) Compiling $<
		@$(CXX) $(FLAGS) -MMD -MF $*.d -c $<
		
clean:
		@$(ECHO) Removing all generated files
		@$(RM) *.o $(BIN) *.d