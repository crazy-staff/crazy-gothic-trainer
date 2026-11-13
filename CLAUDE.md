# crazy-gothic-trainer — instructions for Claude

The in-game trainer of the Crazy ecosystem (org `crazy-staff`): a Union DLL for Gothic 1 (Steam) with a
Dear ImGui menu drawn through GD3D11, split into per-game modules (`src/games/<game>`,
`src/renderers/<renderer>`, `src/shared`). Gothic 2 and story mods (Chronicles of Myrtana) come later.

## State (2026-11-13)

- **Paused.** Order of work in the ecosystem: crazy-ui → crazy-hub → back to the trainer.
- Planned later: rewrite in Rust (Tauri + React with crazy-ui for the desktop part; how to render React UI
  inside the game is a separate research task). The menu gets redesigned to match crazy-ui.
- The portal code in `portal/` and `tools/portal/` belongs to the old crazy-hub approach (frozen); it moves
  out when crazy-hub's rewrite starts. Docs content in `docs/pages` will be imported into the new docs
  module of crazy-hub once, after which the hub is the source of truth.

## People and process

- The owner (Dawid) is project lead; he tests builds in the game and sends feedback lists. Talk to him in
  **Polish**; everything on GitHub in **English** (README translated to PL/DE/RU). Labels `kind: value`.
- Features are labelled by test status (green verified, yellow works badly, red crashes, blue new); update
  the flags after each of his reports. Keep features modular so a change can't break verified ones.
- Work on his Windows PC in `F:\Documents\projects\crazy-gothic-trainer` (local Claude Code session), and
  copy builds into the game folder.
- Every build gets a GitHub release with install notes.
