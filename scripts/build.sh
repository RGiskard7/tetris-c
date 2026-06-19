#!/bin/bash
# Script de compilacion para Tetris (Unix/macOS)
# Autor: RGiskard7

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

echo "==================================================="
echo "        Compilando Tetris"
echo "==================================================="
echo ""

ALLEGRO_DIR="${ALLEGRO_DIR:-/usr/local}"
ALLEGRO_INCLUDE="${ALLEGRO_DIR}/include"
ALLEGRO_LIB="${ALLEGRO_DIR}/lib"

CFLAGS="-g -Wall -pedantic -I include -I ${ALLEGRO_INCLUDE}"
LIBS="-L ${ALLEGRO_LIB} -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf"

# Compile objects
gcc ${CFLAGS} -c src/main.c -o src/main.o
gcc ${CFLAGS} -c src/game.c -o src/game.o
gcc ${CFLAGS} -c src/board.c -o src/board.o
gcc ${CFLAGS} -c src/piece.c -o src/piece.o

# Link
gcc ${CFLAGS} -o tetris src/main.o src/game.o src/board.o src/piece.o ${LIBS}

echo ""
echo "[OK] Compilacion exitosa."
echo "Ejecuta: ./tetris"
