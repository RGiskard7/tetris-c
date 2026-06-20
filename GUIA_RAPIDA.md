# Guia Rapida - Tetris

Arranca el juego en menos de 5 minutos.

---

## El camino mas corto

### Windows

```cmd
scripts\install-deps.bat
scripts\build.bat run
```

### macOS / Linux

```bash
scripts/install-deps.sh
scripts/build.sh run
```

---

## Controles

| Tecla        | Accion               |
|--------------|----------------------|
| Left / Right | Mover pieza          |
| Down         | Caida suave (soft drop) |
| Up           | Rotar pieza (90 grados) |
| Space        | Caida instantanea (hard drop) |
| P            | Pausa                |
| Enter        | Iniciar / Reiniciar  |
| Escape       | Salir (titulo) / Pausa (juego) |

---

## Reglas del original (NES)

- 7 tetriminos: I, J, L, O, S, T, Z
- 30 niveles (0-29). Cada 10 lineas limpias = subida de nivel
- Velocidad desde 48 frames/fila (nivel 0) hasta 1 frame/fila (nivel 29, el "kill screen")
- Puntuacion: Single=40, Double=100, Triple=300, Tetris=1200 (todo x nivel+1)
- Sin wall kicks: si una rotacion choca, falla
- Lock delay breve que se reinicia al mover o rotar la pieza
- La partida termina por Block Out (la pieza siguiente no puede aparecer)

---

## Si algo falla

**"undefined reference" al compilar**
Faltan las librerias de Allegro. Revisa que `allegro_font-5`, `allegro_ttf-5` y `allegro_primitives-5` esten instaladas.

**No se ve nada o no carga la fuente**
Ejecuta siempre desde la raiz del proyecto (`tetris-c/`). La fuente `space_invaders.ttf` se copia automaticamente desde los otros proyectos si esta disponible.

**Falta `allegro_monolith-5.2.dll` al ejecutar**
`scripts\build.bat run` la copia sola. Para repartir el juego usa `scripts\dist.bat`.

**La ventana se abre en negro y se cierra**
Asegurate de que `resources/fonts/space_invaders.ttf` existe. El juego funciona sin ella pero no mostrara texto en los overlays. Puedes copiarla desde `breakout-c/resources/fonts/`.

---

## Verificar que todo esta bien

```bash
gcc --version
pkg-config --modversion allegro-5
```
