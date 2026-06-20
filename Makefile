CC=gcc
CFLAGS=-g -Wall -pedantic

ALLEGRO_VERSION=5.2.9.1
MINGW_VERSION=14.1.0
FOLDER=C:
FOLDER_NAME=\allegro-$(ALLEGRO_VERSION)-mingw-$(MINGW_VERSION)
PATH_ALLEGRO=$(FOLDER)$(FOLDER_NAME)
LIB_ALLEGRO=\lib
INCLUDE_ALLEGRO=\include

FLAGS=-I include -I $(PATH_ALLEGRO)$(INCLUDE_ALLEGRO) $(CFLAGS)
LIBS=-L $(PATH_ALLEGRO)$(LIB_ALLEGRO) -lallegro_monolith -lallegro_main -lallegro_primitives -lallegro_font -lallegro_ttf -lallegro_image -lallegro_audio -lallegro_acodec

OBJS=src/main.o src/game.o src/board.o src/piece.o
TARGET=tetris.exe

all: $(TARGET)

src/main.o: src/main.c include/game.h include/config.h
	$(CC) $(FLAGS) -c src/main.c -o src/main.o

src/game.o: src/game.c include/game.h include/config.h include/board.h include/piece.h
	$(CC) $(FLAGS) -c src/game.c -o src/game.o

src/board.o: src/board.c include/board.h include/config.h
	$(CC) $(FLAGS) -c src/board.c -o src/board.o

src/piece.o: src/piece.c include/piece.h include/config.h
	$(CC) $(FLAGS) -c src/piece.c -o src/piece.o

$(TARGET): $(OBJS)
	@echo ----------------------------------------------------------
	@echo Makefile Tetris
	@echo ----------------------------------------------------------
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

clean:
	-del /q src\main.o src\game.o src\board.o src\piece.o tetris.exe 2>nul
