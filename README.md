# Jetpack GBA

## Pitch

A Game Boy Advance homebrew prototype inspired by Jetpac on ZX Spectrum.

The goal is not to create a pixel-perfect clone, but a faithful GBA reinterpretation focused on responsive jetpack movement, fixed-screen arcade action, item collection, enemies, and level progression.

## Project status

Current milestone: Milestone 3 — Rocket parts (in progress)

Current focus:

- Keep the base GBA build setup stable.
- Refine fixed-screen composition with HUD and delivery-zone layout.
- Current ROM behavior: dirty-rectangle Mode 3 redraw with readable placeholder HUD text (SCORE/LIVES/HI), Jetpac-like fixed-screen composition, horizontal player wrap, and a three-section ship where the first section starts placed and the remaining two parts are released over the ship delivery zone, drop into their slots automatically, and must be delivered in order (body then nose). After full ship assembly, a six-fuel loop starts: one fuel at a time spawns at the top, falls to a platform/floor, becomes pickable, and auto-delivers to the ship until 6 fuels are delivered. Reaching this objective marks the ship as ready, and overlapping the launch zone triggers a stage-clear placeholder state. A minimal B-button one-shot-at-a-time firing prototype is active, rendered as a fast horizontal beam-like segment.

## Requirements

- devkitPro / devkitARM
- make
- A Game Boy Advance emulator, such as mGBA

## Build

Run:

    make

## Run

Open the generated `.gba` file in a Game Boy Advance emulator.

Example:

    mgba roms/jetpack-zxs.gba

## Controls

- D-pad left/right: move
- A: jetpack thrust
- B: fire projectile

## Original gameplay reference

The game is inspired by the structure of Jetpac on ZX Spectrum.

Observed reference structure:

- The game uses fixed-screen stages with three main platforms.
- The player can leave the screen from one horizontal side and reappear on the opposite side.
- Each stage features one active enemy type.
- There are 8 enemy types, each with its own behavior pattern.
- A full progression loop can be represented as 4 ships with 4 stages each, for a total of 16 stages.
- Across those 16 stages, the 8 enemy types appear twice.

Ship and stage structure:

- Stage 1 of each ship requires assembling the ship from 3 parts, then collecting 6 fuel pods.
- Stages 2, 3, and 4 of each ship require collecting 6 fuel pods only.
- After 4 stages, the game moves to the next ship.
- After 4 ships, the ship cycle loops.

Additional pickups:

- Fuel is required for stage progression.
- Bonus collectibles can appear and award score.
- Bonus collectibles are optional and are not required to complete a stage.

## Gameplay overview

The player controls an astronaut using a jetpack to assemble a rocket, collect items, avoid or shoot enemies, and progress through increasingly difficult levels.

Core loop:

1. Move around the screen with the jetpack.
2. Pick up rocket parts or fuel.
3. Bring items to the rocket.
4. Avoid or shoot enemies.
5. Complete the rocket objective.
6. Advance to the next level.

## Technical overview

Target platform: Game Boy Advance  
Language: C  
Resolution: 240x160  
Camera: fixed screen

Coordinate convention:

- X increases to the right.
- Y increases downward.
- Top-left is 0,0.

Gameplay code should avoid floating point math where possible. Prefer fixed-point arithmetic for player movement and physics.

Suggested fixed-point format:

    #define FIX_SHIFT 8
    #define TO_FIX(x) ((x) << FIX_SHIFT)
    #define FROM_FIX(x) ((x) >> FIX_SHIFT)

## Development principles

- Keep the game compiling.
- Make small, testable changes.
- Prioritize player feel before adding content.
- Avoid unnecessary abstractions.
- Keep the project readable for both humans and coding agents.
