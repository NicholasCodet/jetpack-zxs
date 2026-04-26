# Roadmap

## Current status

Current milestone: Milestone 1 — Player movement

Status: In progress

Current focus:

- Tune the first playable movement prototype.
- Keep movement constants easy to adjust.
- Prepare Milestone 2 platform implementation.

Next milestone: Milestone 2 — Platforms

## Milestone 0 — Project setup

Goal: validate the project foundation.

Status: Completed on 2026-04-26

Tasks:

- Validate the Makefile.
- Generate a bootable `.gba` file.
- Confirm the game launches in an emulator.
- Add initial documentation files.
- Confirm the base project architecture.

Success criteria:

- `make` completes successfully.
- A `.gba` file is generated.
- The ROM boots in a GBA emulator.

## Milestone 1 — Player movement

Goal: create the smallest playable movement prototype.

Status: Prototype implemented on 2026-04-26
Progress update: horizontal acceleration and friction refinement added on 2026-04-26.

Tasks:

- Add a placeholder player.
- Implement horizontal movement.
- Implement gravity.
- Implement jetpack thrust.
- Clamp the player to the screen.
- Add simple floor collision.
- Expose movement constants for tuning.

Success criteria:

- The player can move left and right.
- The player can fly using the A button.
- The player falls when A is released.
- The player lands on the floor.
- The player cannot leave the screen.

## Milestone 2 — Platforms

Goal: add the first level structure.

Tasks:

- Add static platforms.
- Implement platform collision.
- Tune landing behavior.
- Keep the screen readable at GBA resolution.

Success criteria:

- The player can land on platforms.
- The player can take off from platforms.
- The player does not pass through platforms.
- The level remains playable on a 240x160 screen.

## Milestone 3 — Rocket parts

Goal: implement the main objective loop.

Tasks:

- Add a rocket base.
- Add one collectible rocket part.
- Allow the player to pick up the part.
- Allow the player to carry the part.
- Allow the player to deposit the part on the rocket.

Success criteria:

- A rocket part appears on the screen.
- The player can collect it.
- The part follows the player while carried.
- The part is added to the rocket when delivered.

## Milestone 4 — Enemies

Goal: add basic danger.

Tasks:

- Add one enemy type.
- Implement enemy movement.
- Respawn enemies when they leave the screen.
- Add collision between player and enemies.
- Add player damage or life loss.

Success criteria:

- Enemies move across the screen.
- Enemies can hit the player.
- The player loses a life or restarts after collision.

## Milestone 5 — Shooting

Goal: let the player defend themselves.

Tasks:

- Add player projectiles.
- Trigger shooting with the B button.
- Add projectile movement.
- Add collision between projectiles and enemies.
- Add score increase when enemies are destroyed.

Success criteria:

- The player can shoot.
- Projectiles move correctly.
- Projectiles can destroy enemies.
- Destroying enemies increases the score.

## Milestone 6 — Level loop

Goal: complete the first full gameplay loop.

Tasks:

- Complete the rocket assembly objective.
- Add fuel or final launch requirement.
- Trigger level completion.
- Reset or advance the level.
- Increase difficulty slightly.

Success criteria:

- The player can complete a level.
- The game transitions to the next level or restarts with increased difficulty.
- The core loop feels understandable and replayable.
