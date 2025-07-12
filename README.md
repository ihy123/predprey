# Predator-prey simulation

A simple 2D simulation written in C using raylib.

## Rules

Preys feed from the ground and reproduce.
Predators eat preys to keep living and can reproduce when satiated.
Movement is life: every entity tries to move at every tick of the timer
and dies if it can't.

More explicitly, at every tick:
- prey feeds (increase health)
- prey moves to a neighbouring cell
- prey reproduces on enough health: leave their offspring in
original cell and loose some health for giving birth
- pred hunts when not too satiated: kill prey in one of neighbouring cells
and move there. Increase satiation by prey's health multiplied by a
conversion rate
- pred moves if can't hunt: move to a neighbouring cell
- pred reproduces on enough satiety: leave their offspring in
original cell and loose some satiety for giving birth

## Plans

- separate tickrate from FPS
- stats
- graphs?
- UI:
  - controls:
    - pause (space, p)
    - clean (backspace, del)
    - exit (esc)
    - add mode (a):
      - add pred (lmb)
      - add prey (rmb)
    - del mode (d):
      - del (lmb, rmb)
  - settings:
    - field size
    - tick rate (per second)
    - fps limit
    - stats averaging interval
    - graphing interval
    - balance:
      - preset:
        - 30x30
        - 60x60+
      - max health/satiety
      - wrapping field
      - satiety min for birth
      - satiety max for hunting
      - satiety decrement
      - satiety penalty for birth
      - satiety to health ratio
      - health min for birth
      - health increment
      - health penalty for birth
