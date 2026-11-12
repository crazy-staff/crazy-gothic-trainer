# Portal (Quackback + docs)

Our public site is [Quackback](https://github.com/QuackbackIO/quackback) (feedback, roadmap, changelog,
support, status) with one addition: a **Docs** section rendered inside the portal layout, so the header,
session, notifications and theme are the real ones.

- `QUACKBACK_VERSION` — the upstream release we build on.
- `patch/` — files copied over the upstream checkout. **New files only** (`routes/_portal/docs*`,
  `src/gothic-docs/*`), nothing upstream is edited, so updating Quackback is just bumping the version.
- `build-manifest.mjs` — bundles `docs/pages/**.md` + `sections.json` into the image.
- `.github/workflows/portal.yml` builds `ghcr.io/dawidseipold/gothic1-portal` on every change.

The "Docs" tab is a custom link in Admin → Settings → Portal → Navigation (`https://<domain>/docs`).
The admin UI saves links as "open in new tab"; run `docs-link.sql` once to make it open in the same tab.

Docs pages: `docs/pages` (markdown). Feature pages are generated from the trainer source:
`python3 tools/docs/extract-options.py && python3 tools/docs/gen-features.py` (English names in `docs/options.en.json`).
