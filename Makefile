# Makefile for Mighty Bomb Jack - Native Source Port (Linux & GCC)

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -Iinclude $(shell pkg-config --cflags sdl2 2>/dev/null || echo -I/usr/include/SDL2)
LIBS ?= $(shell pkg-config --libs sdl2 2>/dev/null || echo -lSDL2) -lm

SRCS = src/main.c \
       src/platform/mjk_platform_sdl.c \
       src/core/mjk_game.c \
       src/core/mjk_state_machine.c \
       src/core/mjk_player.c \
       src/core/mjk_room_decoder.c \
       src/core/mjk_room_data.c \
       src/core/mjk_ppu.c \
       src/core/mjk_enemy.c \
       src/core/mjk_hud.c \
       src/core/mjk_apu.c \
       src/core/mjk_assets.c

TARGET = bin/mjk

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)
	@if [ -f assets/assets.dat ]; then cp -f assets/assets.dat bin/assets.dat; fi
	@echo "[OK]: Compilacion exitosa para Linux en $(TARGET)"

clean:
	rm -rf bin/mjk bin/*.o
