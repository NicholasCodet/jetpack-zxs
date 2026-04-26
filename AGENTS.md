# AGENTS.md

## Role

You are helping build a Game Boy Advance homebrew game in C inspired by Jetpac on ZX Spectrum.

The goal is to support a vibecoding workflow while keeping the codebase stable, simple, and easy to reason about.

## Working principles

- Always inspect the current code before modifying it.
- Make the smallest useful change.
- Keep the project compiling at every step.
- Do not add features that were not requested.
- Do not refactor unrelated files.
- Prefer boring, readable C over clever abstractions.
- Avoid floating point math in gameplay code.
- Prefer fixed-point integers for movement and physics.
- Keep constants easy to tune.
- Preserve the existing project setup unless explicitly asked to change it.

## GBA constraints

- Target platform: Game Boy Advance.
- Target resolution: 240x160.
- Language: C.
- Prefer simple sprite and tile usage.
- Keep memory usage conservative.
- Avoid dynamic allocation unless explicitly needed.
- Update graphics during VBlank where appropriate.
- Avoid expensive runtime operations in the main loop.
- Keep gameplay logic deterministic where possible.

## Project constraints

- This is a GBA reinterpretation inspired by Jetpac, not a pixel-perfect clone.
- The screen is fixed; do not add scrolling unless requested.
- Prioritize player feel before content volume.
- Do not implement menus, multiple levels, audio, or advanced effects unless requested.
- Keep the architecture simple enough for a small arcade game.

## Expected response after each coding task

After making changes, report:

1. Summary of changes.
2. Files modified.
3. How to build.
4. How to test in emulator.
5. Documentation updated, if any.
6. Tuning constants, if any.
7. Known limitations.
8. Suggested Git commit message.

Do not create the commit unless explicitly asked.

## Coding style

- Use clear names.
- Keep functions short when practical.
- Prefer explicit state over hidden behavior.
- Use comments only when they clarify intent.
- Do not over-engineer.
- Do not introduce large abstractions prematurely.
- Keep gameplay constants grouped and easy to edit.

## Documentation maintenance

When completing a meaningful change, update the relevant documentation files if needed.
Keep documentation aligned with the current project state.

In particular:
- Update `README.md` when build instructions, run instructions, controls, project status, dependencies, or setup details change.
- Update `ROADMAP.md` when a milestone task is completed, started, blocked, or changed.
- Update `AGENTS.md` only when project workflow rules or coding-agent instructions change.

Do not update documentation just for the sake of updating it.
Prefer small, accurate documentation updates over broad rewrites.

## Git commit suggestion

After each meaningful change, suggest one Git commit message.
Do not create the commit.
Use Conventional Commits when possible.

Examples:
- `chore: initialize GBA project documentation`
- `build: validate GBA makefile setup`
- `feat: add player jetpack movement prototype`
- `fix: clamp player inside screen bounds`
- `docs: update roadmap after movement milestone`

The suggested commit message should be short, specific, and describe the actual change.

## Safety rule

If a requested change risks breaking the build or requires a larger refactor, explain the risk first and propose the smallest safe alternative.