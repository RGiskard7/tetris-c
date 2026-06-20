# Resumen de Implementacion - Tetris

## Codigo

| Archivo | Lineas | Que hace |
|---------|--------|----------|
| `src/main.c` | ~220 | Entrada, inicializa Allegro, bucle principal |
| `src/game.c` | ~1270 | Logica central: estados, input, gravedad, scoring, high scores, musica, render |
| `src/board.c` | ~310 | Tablero 10x22: colisiones, bloqueo, limpieza de lineas, render |
| `src/piece.c` | ~430 | Tetriminos: 7 piezas x 4 rotaciones, offsets, render, ghost |
| `include/config.h` | ~108 | Constantes, geometria, colores, velocidades, scoring y reglas |

## Arquitectura

- Structs opacos con getters/setters (mismo patron que Breakout y Space Invaders)
- Configuracion centralizada en `config.h`
- Maquina de 5 estados: titulo, jugando, pausa, game over, entrada de iniciales (high score)
- Array `bool filled[22][10]` para celdas ocupadas (evita `al_unmap_rgba`)
- Recursos creados y destruidos explicitamente, en orden correcto
- Musica de fondo con `ALLEGRO_PLAYMODE_LOOP`, se para en pausa y game over

## Fidelidad al NES original

- Tablero 10x20 visible + 2 filas ocultas superiores (22 filas totales)
- 7 tetriminos (I, J, L, O, S, T, Z) con offsets de rotacion por estado (0, 90, 180, 270)
- Rotacion sin wall kicks: si colisiona, la rotacion falla
- 30 niveles (0-29) con tabla de velocidades identica a NES (48 frames en nivel 0, 1 frame en nivel 29)
- Cada 10 lineas limpias = subida de nivel
- Scoring NES: 40/100/300/1200 puntos, multiplicado por (nivel + 1)
- Reroll randomizer: si la pieza generada es igual a la anterior, tira otra vez
- Lock delay de 15 frames que se reinicia al mover o rotar la pieza
- Block Out como condicion de derrota (la pieza no puede spawnear)
- Soft drop: +1 punto por fila bajada manualmente
- Hard drop: +2 puntos por fila, caida y bloqueo instantaneos
- Ghost piece que muestra la posicion final de caida
- Preview de la siguiente pieza en el lateral derecho
- DAS con delay de 16 frames y repeticion cada 6 frames

## Sistema de records

- Top 5 persistente en `highscores.dat` (formato: `AAA 1500`)
- La pantalla de titulo alterna cada 90 frames entre el titulo y la tabla de records
- Al terminar la partida, si la puntuacion entra en el top 5, se piden 3 iniciales
- Entrada de iniciales: UP/DOWN para cambiar letra (0-9, A-Z), LEFT/RIGHT para mover cursor
- ENTER guarda, ESC salta. Delay de 45 frames anti-rebote del ENTER del Game Over

## Sistema de compilacion

- `Makefile` (Windows/MinGW)
- `scripts/build.bat` / `scripts/build.sh`
- `scripts/dist.bat` / `scripts/dist.sh` (carpeta portable: Windows lleva .exe + DLL + recursos; macOS/Linux solo binario + recursos)
- `scripts/install-deps.bat` / `scripts/install-deps.sh`

## Constantes principales (config.h)

| Constante | Valor | Descripcion |
|-----------|-------|-------------|
| `DISPLAY_WIDTH/HEIGHT` | 640 x 700 | Tamano de ventana |
| `BOARD_COLS / BOARD_ROWS` | 10 x 22 | Dimensiones del tablero (20 visible + 2 ocultas) |
| `CELL_SIZE` | 28 | Tamano de cada celda en pixeles |
| `PIECE_TYPES` | 7 | Numero de tetriminos |
| `LINES_PER_LEVEL` | 10 | Lineas necesarias para subir de nivel |
| `MAX_LEVEL` | 29 | Nivel maximo (30 niveles: 0-29) |
| `DAS_DELAY / DAS_REPEAT` | 16 / 6 | Frames del Delayed Auto Shift |
| `LOCK_DELAY` | 15 | Frames de retraso antes del bloqueo |
| `PTS_SINGLE` | 40 | Puntos base por 1 linea |
| `PTS_DOUBLE` | 100 | Puntos base por 2 lineas |
| `PTS_TRIPLE` | 300 | Puntos base por 3 lineas |
| `PTS_TETRIS` | 1200 | Puntos base por 4 lineas (Tetris) |
| `MAX_TOP_SCORES` | 5 | Records en la tabla |

## Sobre la DLL de Allegro

El ejecutable solo necesita `allegro_monolith-5.2.dll`; el resto de dependencias
(`KERNEL32`, `api-ms-win-crt-*`) ya vienen con Windows 10/11. No se puede generar
un .exe sin esa DLL porque esta distribucion de Allegro no incluye los `.a`
estaticos de sus dependencias (FLAC, Vorbis, FreeType, libpng...): solo existen
dentro del propio monolito. Por eso `scripts/dist.bat` la empaqueta junto al juego.
