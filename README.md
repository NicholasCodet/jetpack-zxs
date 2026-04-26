# Jetpack GBA

## Pitch

A Game Boy Advance homebrew prototype inspired by Jetpac on ZX Spectrum.

The goal is not to create a pixel-perfect clone, but a faithful GBA reinterpretation focused on responsive jetpack movement, fixed-screen arcade action, item collection, enemies, and level progression.

## Project status

Current milestone: Milestone 1 — Player movement (in progress)

Current focus:

- Keep the base GBA build setup stable.
- Tune and iterate on the first playable movement prototype.
- Current ROM behavior: placeholder player with acceleration-based horizontal movement, gravity, and jetpack thrust.

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

    mgba roms/jetpack_gba.gba

## Controls

- D-pad left/right: move
- A: jetpack thrust

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
