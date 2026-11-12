# Portal (Quackback + docs)

Our public site is [Quackback](https://github.com/QuackbackIO/quackback) (feedback, roadmap, changelog,
support, status) with its **help center turned into the trainer docs**.

- `QUACKBACK_VERSION` — the upstream release we build on.
- `dependencies.txt` — extra npm packages (Fumadocs UI/Core, Shiki), added with `bun add` before the image build.
- `transform.py` — run on an upstream checkout: copies `patch/` (new files) and applies exact edits.
  Every edit must match exactly once; if Quackback changed, the build stops with a clear message.
  - help center → **Docs** everywhere in English (module, nav tab, admin, settings);
  - docs pages on **Fumadocs UI** (`patch/apps/web/src/custom-docs`): sidebar from the published
    categories and articles, search dialog (`Ctrl+K`, Orama over `/api/docs-search`), page outline
    ("clerk" style), prev/next, and article components (callouts, cards, steps, tabs, accordions,
    file trees, option tables, keys, status badges, trainer menu replicas, media placeholders);
  - portal header: one full-width row, navigation centred;
  - API additions: article `position`, changelog `categoryNames` (e.g. `["Docs"]`).
- `.github/workflows/portal.yml` builds `ghcr.io/dawidseipold/gothic1-portal` on every change here.

## Docs content

Pages are markdown in `docs/pages/<category>/*.md` (categories in `docs/pages/categories.json`).
Front matter: `title`, `description`, `order`, `menu` (the in-game menu path).
Every element a page can use is shown (with its source) in `docs/pages/playground/component-showcase.md`.
Link pages with `doc:<slug>` or `doc:<slug>#<heading>`.

Sync to a portal: `QB_URL=https://… QB_KEY=qb_… node tools/docs/sync.mjs` — creates/updates categories and
pages, publishes them and adds a changelog entry in the **Docs** category for new or changed pages.
