# Portal (Quackback + docs)

Our public site is [Quackback](https://github.com/QuackbackIO/quackback) (feedback, roadmap, changelog,
support, status) with its **help center turned into the trainer docs**.

- `QUACKBACK_VERSION` — the upstream release we build on.
- `transform.py` — run on an upstream checkout: copies `patch/` (new files) and applies exact edits.
  Every edit must match exactly once; if Quackback changed, the build stops with a clear message.
  - help center → **Docs** (module, nav tab, admin, settings), docs frame with a left sidebar,
    article components (callouts, cards, media placeholders, trainer menu replicas);
  - API additions: article `position`, changelog `categoryNames` (e.g. `["Docs"]`).
- `.github/workflows/portal.yml` builds `ghcr.io/dawidseipold/gothic1-portal` on every change here.

## Docs content

Pages are markdown in `docs/pages/<category>/*.md` (categories in `docs/pages/categories.json`).
Front matter: `title`, `description`, `order`, `menu` (the in-game menu path).
Extra blocks: `> **Note:** / **Tip:** / **Warning:**` callouts, fenced ` ```trainer ` (menu card replica, JSON),
` ```media ` (`screenshot: …` / `video: …` placeholder), ` ```cards ` (JSON link cards). Link pages with `doc:<slug>`.

Sync to a portal: `QB_URL=https://… QB_KEY=qb_… node tools/docs/sync.mjs` — creates/updates categories and
pages, publishes them and adds a changelog entry in the **Docs** category for new or changed pages.
