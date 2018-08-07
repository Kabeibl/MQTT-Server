IDIR=./include
LDIR=./lib
OBJ_DIR=./obj
SRC_DIR=./src

LINK_TARGET=mqtt_server.exe

CC=gcc
CFLAGS+=-I$(IDIR) -g -Wall 
LDFLAGS+=-lpaho-mqtt3c -lncurses -lmenu -lform -pthread -lrt

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# TESTS #
test: $(LINK_TARGET)
	-./$(LINK_TARGET)
	make clean > /dev/null

all: $(LINK_TARGET)

# Link #
$(LINK_TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# Compile #
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean #
clean:
	rm $(OBJS)