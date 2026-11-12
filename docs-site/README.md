# Docs site

The documentation of Gothic 1 Crazy Trainer, served at `/docs` on the same domain as the
Quackback portal (a reverse proxy sends `/docs/*` here and everything else to Quackback),
styled like the portal so it feels like one site.

- Built with [Fumadocs](https://fumadocs.dev) (Next.js static export, `basePath: '/docs'`).
- `content/docs/**` — hand-written pages (MDX).
- `content/docs/features/**` — generated, do not edit:
  `python3 scripts/extract-options.py` reads every option from `../src` into `data/options.json`,
  `python3 scripts/gen-features.py` writes the pages using English names from `data/options.en.json`
  (a tab is published once all its options are translated there).
- `components/portal-header.tsx` — copy of the portal header; `app/global.css` — portal colours.

```
npm install
npm run build      # -> out/ (serve it at /docs)
```
