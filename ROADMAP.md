# Roadmap

## Current status

Current milestone: Milestone 3 — Rocket parts

Status: In progress

Current focus:

- Tune HUD/playfield composition and ship delivery-zone behavior.
- Validate two-part collection flow with one section pre-placed and drop-to-slot delivery behavior.
- Expand the fuel loop to 6 deliveries with one active fuel at a time after ship assembly.
- Add ship-ready and stage-clear placeholder triggers after the fuel objective.
- Restore Jetpac-style horizontal wrap for player movement.
- Start Milestone 5 with a minimal one-projectile B-button shooting prototype.
- Start Milestone 4 with one moving enemy plus first shot collision and score increment.
- Add first player damage/lives prototype with respawn, invulnerability, and game-over placeholder on enemy contact.
- Refine hit/respawn flow so carried objects are released back into play, and add beam-shot horizontal wrap with finite lifetime.
- Add short death-to-respawn delay and tune beam shot lifetime to roughly half-screen travel.
- Synchronize player/enemy hide and respawn timing during death delay.

Next milestone: Milestone 4 — Enemies

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
Progress update: horizontal acceleration and friction refinement added on 2026-04-26; horizontal wrap behavior added on 2026-05-24.

Tasks:

- Add a placeholder player.
- Implement horizontal movement.
- Implement gravity.
- Implement jetpack thrust.
- Clamp the player vertically and wrap horizontally.
- Add simple floor collision.
- Expose movement constants for tuning.

Success criteria:

- The player can move left and right.
- The player can fly using the A button.
- The player falls when A is released.
- The player lands on the floor.
- The player wraps across left/right screen edges and remains inside vertical playfield bounds.

## Milestone 2 — Platforms

Goal: add the first level structure.

Status: Completed on 2026-04-26
Progress update: static platform rendering plus robust one-way landing collisions (with fixed-point crossing checks) completed on 2026-04-26.

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

Status: In progress
Progress update: dirty-rectangle redraw retained, readable placeholder HUD text added, and Jetpac-like platform/ship composition refined with one pre-placed section plus two collectable parts delivered in enforced order (body then nose) using release-and-drop-to-slot behavior on 2026-05-24. Nose pickup is now locked until body delivery to prevent assembly softlock. Fuel loop now requires 6 deliveries with deterministic one-at-a-time sky spawns after assembly; each fuel falls, lands, becomes pickable, and auto-delivers on 2026-05-24. After 6 fuels, ship-ready and stage-clear placeholder states are now triggered in place.

Tasks:

- Add a rocket base.
- Add collectible rocket parts.
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

Status: In progress
Progress update: one placeholder enemy now moves horizontally with wrap/respawn behavior; beam-shot collision despawns the shot, respawns enemy position, and increments score on 2026-05-25. Scoring rules aligned to current reference target: enemy = 25, ship-part pickup = 100, fuel pickup = 100, deliveries = 0.
Progress update: player/enemy contact now decrements lives, respawns the player at spawn with a short invulnerability window, updates HUD lives dynamically, and triggers a GAME OVER placeholder at 0 lives on 2026-05-25.
Progress update: death handling now resets enemy position, releases carried ship part/fuel at the hit location, and lets released objects fall to platform/floor before becoming pickable again; beam shots now wrap horizontally and expire by lifetime on 2026-05-25.
Progress update: enemy contact now triggers a brief respawn delay before player re-entry (with invulnerability after respawn), and beam lifetime is tuned down to reduce travel range to about half a screen on 2026-05-25.
Progress update: on enemy contact, player and enemy now disappear together during the delay, active shots are cleared, and player/enemy reappear together after respawn; enemies remain inactive on game over on 2026-05-25.

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

Status: In progress
Progress update: minimal single-projectile B-button prototype added on 2026-05-24. Projectile feel refined toward a fast horizontal beam-style shot on 2026-05-24.

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
