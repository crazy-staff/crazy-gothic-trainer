#!/usr/bin/env python3
"""Turns Quackback's help center into the Gothic 1 Crazy Trainer docs.
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
    # docs frame (sidebar) around every help-center page
    "routes/_portal/hc.tsx": [
        ("import { setPublicDocumentCacheHeaders } from '@/lib/server/functions/public-cache'\n",
         "import { setPublicDocumentCacheHeaders } from '@/lib/server/functions/public-cache'\n"
         "import { getGothicDocsNavFn } from '@/gothic-docs/docs-fns'\n"
         "import { GothicDocsShell } from '@/gothic-docs/docs-shell'\n"),
        ("    return { helpCenterConfig, messages }\n",
         "    const docsNav = await getGothicDocsNavFn({ data: {} }).catch(() => [])\n"
         "    return { helpCenterConfig, messages, docsNav }\n"),
        ("  const { helpCenterConfig, messages } = Route.useLoaderData()\n",
         "  const { helpCenterConfig, messages, docsNav } = Route.useLoaderData()\n"),
        ("          <Outlet />\n",
         "          <GothicDocsShell nav={docsNav} locale={locale}>\n            <Outlet />\n          </GothicDocsShell>\n"),
    ],
    # article page: docs body (callouts, trainer cards, media), no author box, wider text
    "routes/_portal/hc/$locale/articles/$idSlug/index.tsx": [
        ("import { RichTextContent, isRichTextContent } from '@/components/ui/rich-text-content'\n",
         "import { isRichTextContent } from '@/components/ui/rich-text-content'\n"
         "import { GothicDocsContent } from '@/gothic-docs/docs-content'\n"),
        ('      <div className="mx-auto max-w-6xl px-4 sm:px-6">\n', '      <div className="w-full">\n'),
        ('          <article className="min-w-0 max-w-2xl flex-1 py-10">', '          <article className="min-w-0 max-w-3xl flex-1 py-10">'),
        ("            {(article.author || article.updatedAt) && (", "            {false && (article.author || article.updatedAt) && ("),
        ("                  <RichTextContent content={article.contentJson as JSONContent} />",
         "                  <GothicDocsContent content={article.contentJson as JSONContent} />"),
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
         "              linkedPostIds: parseTypeIdArray<PostId>(\n                parsed.data.linkedPostIds,\n                'post',\n                'linked post IDs'\n              ),\n              categoryIds: await (await import('@/gothic-docs/changelog-categories')).changelogCategoryIdsByName(parsed.data.categoryNames),\n            },"),
    ],
    # names: Help Center -> Docs
    "lib/server/domains/settings/settings.types.ts": [
        ("    label: 'Help Center',\n    description: 'Publish searchable help articles so customers can find answers themselves.',",
         "    label: 'Docs',\n    description: 'Documentation of the trainer: guides, every option, FAQ - with a sidebar, search and translations.',"),
    ],
    "lib/shared/settings-pages.ts": [
        ("  '/admin/settings/help-center': { label: 'Help Center' },", "  '/admin/settings/help-center': { label: 'Docs' },"),
    ],
}
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
print(f"portal transform ok: {sum(len(v) for v in EDITS.values())} edits, {sum(len(v) for v in LABELS.values())} labels")
