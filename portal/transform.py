#!/usr/bin/env python3
"""Turns Quackback's help center into documentation built on Fumadocs (custom-docs).
Usage: python3 portal/transform.py <quackback checkout>
1. copies portal/patch/** (new files) over the checkout
2. applies exact text replacements to upstream files. Every replacement must match exactly
   once - if Quackback changed the code, the build stops here with a clear message instead
   of producing a half-patched portal.
"""
import json, os, shutil, sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else "."
HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(ROOT, "apps", "web", "src")

# --- 1. new files --------------------------------------------------------------------------
shutil.copytree(os.path.join(HERE, "patch"), ROOT, dirs_exist_ok=True)

# --- 2. edits ------------------------------------------------------------------------------
EDITS = {
    # docs layout (Fumadocs) around every help-center page, fed with the docs navigation
    "routes/_portal/hc.tsx": [
        ("import { setPublicDocumentCacheHeaders } from '@/lib/server/functions/public-cache'\n",
         "import { setPublicDocumentCacheHeaders } from '@/lib/server/functions/public-cache'\n"
         "import { getDocsNavFn } from '@/custom-docs/docs-fns'\n"
         "import { CustomDocsLayout } from '@/custom-docs/docs-layout'\n"),
        ("    return { helpCenterConfig, messages }\n",
         "    const docsNav = await getDocsNavFn({ data: {} }).catch(() => [])\n"
         "    return { helpCenterConfig, messages, docsNav }\n"),
        ("  const { helpCenterConfig, messages } = Route.useLoaderData()\n",
         "  const { helpCenterConfig, messages, docsNav } = Route.useLoaderData()\n"),
        ("          <Outlet />\n",
         "          <CustomDocsLayout nav={docsNav} locale={locale}>\n            <Outlet />\n          </CustomDocsLayout>\n"),
    ],
    # pages: docs home, category and article pages instead of the help-center ones
    "routes/_portal/hc/index.tsx": [
        ("import { createFileRoute, redirect } from '@tanstack/react-router'\n",
         "import { createFileRoute, redirect } from '@tanstack/react-router'\nimport { DocsHomePage } from '@/custom-docs/pages'\n"),
        ("  component: HelpCenterLandingPage,\n", "  component: DocsHomePage,\n"),
    ],
    "routes/_portal/hc/$locale/index.tsx": [
        ("import { createFileRoute } from '@tanstack/react-router'\n",
         "import { createFileRoute } from '@tanstack/react-router'\nimport { DocsHomePage } from '@/custom-docs/pages'\n"),
        ("  component: LocaleHelpCenterLandingPage,\n", "  component: DocsHomePage,\n"),
    ],
    "routes/_portal/hc/$locale/collections/$idSlug.tsx": [
        ("import { getPublicCollectionPageFn } from '@/lib/server/functions/help-center'\n",
         "import { getPublicCollectionPageFn } from '@/lib/server/functions/help-center'\nimport { DocsCategoryPage } from '@/custom-docs/pages'\n"),
        ("  component: CollectionPage,\n", "  component: DocsCategoryPage,\n"),
    ],
    "routes/_portal/hc/$locale/articles/$idSlug/index.tsx": [
        ("import { getPublicArticlePageFn } from '@/lib/server/functions/help-center'\n",
         "import { getPublicArticlePageFn } from '@/lib/server/functions/help-center'\nimport { DocsArticlePage } from '@/custom-docs/pages'\n"),
        ("  component: ArticleDetailPage,\n", "  component: DocsArticlePage,\n"),
    ],
    # styles: Fumadocs theme mapped onto the portal theme
    "globals.css": [
        ('@import "tw-animate-css";\n', '@import "tw-animate-css";\n@import "./custom-docs/fumadocs.css";\n'),
    ],
    # API: article order (position) for the docs sidebar
    "routes/api/v1/help-center/articles/index.ts": [
        ("  description: z.string().max(300).optional(),\n  authorId: z.string().optional(),\n})",
         "  description: z.string().max(300).optional(),\n  authorId: z.string().optional(),\n  position: z.number().int().min(0).nullable().optional(),\n})"),
    ],
    "routes/api/v1/help-center/articles/$articleId.ts": [
        ("  publishedAt: z.string().datetime().nullable().optional(),\n  authorId: z.string().optional(),\n})",
         "  publishedAt: z.string().datetime().nullable().optional(),\n  authorId: z.string().optional(),\n  position: z.number().int().min(0).nullable().optional(),\n})"),
    ],
    # API: changelog categories by name ("Docs", "Trainer")
    "routes/api/v1/changelog/index.ts": [
        ("  linkedPostIds: z.array(z.string()).optional(),\n})",
         "  linkedPostIds: z.array(z.string()).optional(),\n  categoryNames: z.array(z.string().min(1).max(50)).max(10).optional(),\n})"),
        ("              linkedPostIds: parseTypeIdArray<PostId>(\n                parsed.data.linkedPostIds,\n                'post',\n                'linked post IDs'\n              ),\n            },",
         "              linkedPostIds: parseTypeIdArray<PostId>(\n                parsed.data.linkedPostIds,\n                'post',\n                'linked post IDs'\n              ),\n              categoryIds: await (await import('@/custom-docs/changelog-categories')).changelogCategoryIdsByName(parsed.data.categoryNames),\n            },"),
    ],
    # settings card text for the module
    "lib/server/domains/settings/settings.types.ts": [
        ("    label: 'Help Center',\n    description: 'Publish searchable help articles so customers can find answers themselves.',",
         "    label: 'Docs',\n    description: 'Documentation with a sidebar, search (Ctrl+K), page outline and translations.',"),
    ],
}

# Regions replaced whole: (file, start marker, end marker, new text). Both markers must be unique.
HEADER = """  // Custom: one full-width row on large screens (logo, centred navigation, account);
  // the navigation gets its own row below on smaller screens.
  const headerNav = (
    <Navigation
      navItems={navItems}
      helpHeaderLinks={settings?.helpCenterConfig?.headerLinks}
      supportUnreadTotal={supportUnreadTotal}
    />
  )
  return (
    // A banner landmark, so the logo and account controls sit in one.
    <header
      ref={headerRef}
      className="portal-header sticky top-0 z-40 w-full border-b border-[var(--border)] bg-[color-mix(in_oklab,var(--background)_82%,transparent)] backdrop-blur-md"
    >
      <div className="mx-auto w-full max-w-[97rem] px-4 sm:px-6">
        <div className="grid h-14 grid-cols-[1fr_auto] items-center gap-4 lg:grid-cols-[1fr_auto_1fr]">
          <Link to="/" className="portal-header__logo flex min-w-0 items-center gap-2 justify-self-start">
            {orgLogo ? (
              <img
                src={orgLogo}
                alt={orgName}
                className="h-8 w-8 [border-radius:calc(var(--radius)*0.6)]"
              />
            ) : (
              <div className="h-8 w-8 [border-radius:calc(var(--radius)*0.6)] bg-primary flex items-center justify-center text-primary-foreground font-semibold">
                {nameInitial(orgName)}
              </div>
            )}
            <span
              title={orgName}
              className="portal-header__name font-semibold hidden sm:block max-w-[18ch] truncate text-[var(--header-foreground)]"
            >
              {orgName}
            </span>
          </Link>
          <div className="hidden min-w-0 overflow-x-auto scrollbar-none lg:block">{headerNav}</div>
          <div className="justify-self-end">{authButtons}</div>
        </div>
        <div className="-mx-4 flex h-11 items-center overflow-x-auto px-4 scrollbar-none sm:-mx-6 sm:px-6 lg:hidden">
          {headerNav}
        </div>
      </div>
    </header>
  )
}

"""
REGIONS = [
    ("components/public/portal-header.tsx",
     "  // Two-row layout: Logo + Auth on top, Navigation below\n",
     "const navItemClass = ",
     HEADER),
]

# Every visible "Help center" in English becomes "Docs" (code defaults and en.json; tests skipped).
REBRAND = [("Help Center", "Docs"), ("Help center", "Docs"), ("help center", "docs")]

LABELS = {  # locale catalogs
    "en": {"portal.header.nav.help": "Docs", "portal.hc.breadcrumbs.root": "Docs", "admin.nav.helpCenter": "Docs"},
    "pl": {"portal.header.nav.help": "Docs", "portal.hc.breadcrumbs.root": "Docs", "admin.nav.helpCenter": "Docs"},
    "de": {"portal.header.nav.help": "Docs", "portal.hc.breadcrumbs.root": "Docs", "admin.nav.helpCenter": "Docs"},
    "ru": {"portal.header.nav.help": "Docs", "portal.hc.breadcrumbs.root": "Docs", "admin.nav.helpCenter": "Docs"},
}

errors = []
for rel, reps in EDITS.items():
    p = os.path.join(WEB, rel)
    s = open(p, encoding="utf-8").read()
    for old, new in reps:
        n = s.count(old)
        if n != 1:
            errors.append(f"{rel}: expected 1 match, found {n}: {old.strip()[:80]!r}")
            continue
        s = s.replace(old, new)
    open(p, "w", encoding="utf-8").write(s)
for rel, start, end, new in REGIONS:
    p = os.path.join(WEB, rel)
    s = open(p, encoding="utf-8").read()
    if s.count(start) != 1 or s.count(end) != 1 or s.index(start) > s.index(end):
        errors.append(f"{rel}: region markers not found once: {start.strip()[:60]!r} .. {end.strip()[:40]!r}")
        continue
    open(p, "w", encoding="utf-8").write(s[: s.index(start)] + new + s[s.index(end):])
rebranded = 0
for dirpath, dirs, files in os.walk(WEB):
    dirs[:] = [d for d in dirs if d not in ("__tests__", "custom-docs", "node_modules")]
    for f in files:
        if not f.endswith((".ts", ".tsx")) and f != "en.json":
            continue
        p = os.path.join(dirpath, f)
        s = open(p, encoding="utf-8").read()
        t = s
        for old, new in REBRAND:
            t = t.replace(old, new)
        if t != s:
            rebranded += 1
            open(p, "w", encoding="utf-8").write(t)
for loc, keys in LABELS.items():
    p = os.path.join(WEB, "locales", f"{loc}.json")
    d = json.load(open(p, encoding="utf-8"))
    for k, v in keys.items():
        if k not in d:
            errors.append(f"locales/{loc}.json: missing key {k}")
        d[k] = v
    json.dump(d, open(p, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    open(p, "a").write("\n")
if errors:
    print("PORTAL TRANSFORM FAILED - Quackback changed; update portal/transform.py:\n  " + "\n  ".join(errors))
    sys.exit(1)
print(f"portal transform ok: {sum(len(v) for v in EDITS.values())} edits, {len(REGIONS)} regions, "
      f"{rebranded} files renamed Help center -> Docs, {sum(len(v) for v in LABELS.values())} labels")
