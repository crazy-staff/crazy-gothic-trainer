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
         "import { getDocsNavFn } from '@/custom-docs/docs-fns'\n"),
        # no wrappers: the docs page must be a direct child of the Fumadocs layout grid
        ("""      <div className="flex flex-1 min-h-0 flex-col" dir={isRtlLocale(locale) ? 'rtl' : 'ltr'}>
        {additionalLocales.length > 0 && (
          <div className="flex justify-end px-4 py-2 sm:px-6">
            <HelpCenterLocaleSwitcher
              currentLocale={locale}
              defaultLocale={defaultLocale}
              additionalLocales={additionalLocales}
              canonicalPath={canonicalPath}
            />
          </div>
        )}
        <div className="flex-1 min-w-0">
          <Outlet />
        </div>
      </div>""", """      <Outlet />"""),
        ("import { HelpCenterLocaleSwitcher } from '@/components/help-center/help-center-locale-switcher'\n", ""),
        ("import { DEFAULT_LOCALE, isRtlLocale, seedAreaMessages } from '@/lib/shared/i18n'", "import { DEFAULT_LOCALE, seedAreaMessages } from '@/lib/shared/i18n'"),
        ("  const { locale, canonicalPath } = parseHcLocalePath(pathname, [", "  void parseHcLocalePath(pathname, ["),
        ("    return { helpCenterConfig, messages }\n",
         "    const docsNav = await getDocsNavFn({ data: {} }).catch(() => [])\n"
         "    return { helpCenterConfig, messages, docsNav }\n"),
    ],
    # every portal page in the Fumadocs frame (app-shell.tsx); pages send panels to its sidebar
    "routes/_portal.tsx": [
        ("import { PortalHeader } from '@/components/public/portal-header'\n",
         "import { PortalHeader } from '@/components/public/portal-header'\nimport { AppShell } from '@/custom-docs/app-shell'\n"),
        ("                <FileViewerProvider onJumpToMessage={scrollToMessage}>\n                  <Outlet />\n                </FileViewerProvider>",
         "                <FileViewerProvider onJumpToMessage={scrollToMessage}>\n                  <AppShell>\n                    <Outlet />\n                  </AppShell>\n                </FileViewerProvider>"),
    ],
    "components/public/feedback/feedback-container.tsx": [
        ("import { FeedbackSidebar } from '@/components/public/feedback/feedback-sidebar'\n",
         "import { FeedbackSidebar } from '@/components/public/feedback/feedback-sidebar'\nimport { SidebarSlot } from '@/custom-docs/sidebar-slot'\n"),
        ("  const poweredBy = showPoweredBy ? <PortalPoweredBy workspaceSlug={workspaceSlug} /> : null\n",
         "  const poweredBy = null\n  void showPoweredBy\n  void PortalPoweredBy\n"),
        ("        <FeedbackSidebar footer={poweredBy}>{filterPanel('sidebar')}</FeedbackSidebar>\n",
         "        <SidebarSlot fallback={<FeedbackSidebar>{filterPanel('sidebar')}</FeedbackSidebar>}>\n          {filterPanel('sidebar')}\n        </SidebarSlot>\n"),
    ],
    "routes/_portal/settings.tsx": [
        ("import { SettingsNav } from '@/components/settings/settings-nav'\n",
         "import { SettingsNav } from '@/components/settings/settings-nav'\nimport { SidebarSlot } from '@/custom-docs/sidebar-slot'\n"),
        ("import { SettingsNav } from '@/components/settings/settings-nav'\nimport { SidebarSlot } from '@/custom-docs/sidebar-slot'\n",
         "import { SettingsNav } from '@/components/settings/settings-nav'\nimport { SidebarSlot } from '@/custom-docs/sidebar-slot'\nimport { SettingsOutlet, SettingsRows } from '@/custom-docs/settings-rows'\n"),
        ("        <SettingsNav showApiKeys={isTeamMember(userRole)} />\n",
         "        <SidebarSlot fallback={<SettingsNav showApiKeys={isTeamMember(userRole)} />}>\n          <SettingsRows showApiKeys={isTeamMember(userRole)} />\n        </SidebarSlot>\n"),
        ("          <Outlet />\n", "          <SettingsOutlet>\n            <Outlet />\n          </SettingsOutlet>\n"),
        (" animate-in fade-in duration-200\">", "\">"),
    ],
    # votes: a downvote is replaced by an upvote; ideas and votes need an account
    "lib/server/functions/public-posts.ts": [
        ("  const result = await voteOnPost(data.postId as PostId, ctx.principal.id)\n  log.debug(",
         "  if (ctx.principal.type !== 'anonymous')\n    await (await import('@/custom-docs/downvote-fns')).clearDownvote(data.postId, ctx.principal.id)\n  const result = await voteOnPost(data.postId as PostId, ctx.principal.id)\n  log.debug("),
    ],
    "lib/server/domains/settings/settings.service.ts": [
        ("        allowAnonymous: portalConfig.features.allowAnonymous,", "        allowAnonymous: false,"),
    ],
    "components/public/post-card.tsx": [
        ("import { cn } from '@/lib/shared/utils'\n", "import { cn } from '@/lib/shared/utils'\nimport { DownvoteButton } from '@/custom-docs/downvote-button'\n"),
        ("      {/* Vote column */}\n      {voteButton}",
         "      {/* Vote column */}\n      <div className=\"flex shrink-0 flex-col items-center gap-1\">\n        {voteButton}\n        <DownvoteButton postId={id} />\n      </div>"),
        ("'group/vote flex flex-col items-center justify-center shrink-0 rounded-md border", "'group/vote flex flex-col items-center justify-center shrink-0 rounded-lg border"),
    ],
    "components/public/feedback/feedback-toolbar.tsx": [
        ("'flex items-center gap-1.5 px-2 sm:px-3 py-1.5 rounded-full text-sm transition-colors cursor-pointer',\n                  isActive\n                    ? 'bg-muted text-foreground font-medium'\n                    : 'text-muted-foreground hover:text-foreground hover:bg-muted/50'",
         "'flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg text-sm transition-colors duration-150 cursor-pointer',\n                  isActive\n                    ? 'bg-primary/10 text-primary font-medium'\n                    : 'text-muted-foreground hover:text-accent-foreground hover:bg-accent/60'"),
        ("rounded-full border border-border bg-transparent ps-3 pe-1 text-sm focus-within:ring-2 focus-within:ring-ring/50",
         "rounded-lg border border-border bg-secondary/50 ps-3 pe-1 text-sm transition-colors hover:bg-accent/60 focus-within:ring-2 focus-within:ring-ring/50"),
    ],
    # tabs: Fumadocs-style segmented control
    "components/ui/tabs.tsx": [
        ("      pill: 'h-9 w-fit justify-center rounded-item border border-border/50 p-[3px]',",
         "      pill: 'h-9 w-fit justify-center gap-0.5 rounded-lg border border-border bg-secondary/50 p-[3px]',"),
        ("          'h-[calc(100%-1px)] flex-1 rounded-item border border-transparent px-2 py-1',\n          'text-foreground dark:text-muted-foreground',\n          'data-active:bg-background',\n          'dark:data-active:text-foreground dark:data-active:border-input dark:data-active:bg-input/30',",
         "          'h-full flex-1 rounded-md border border-transparent px-2.5 py-1 transition-colors duration-150',\n          'text-muted-foreground hover:text-accent-foreground',\n          'data-active:bg-primary/10 data-active:text-primary',"),
    ],
    "components/onboarding/home-launch-plan.tsx": [
        ("      ? openLaterSteps(path).length\n      : 0", "      ? 0 * openLaterSteps(path).length\n      : 0"),
    ],
    "components/admin/admin-sidebar.tsx": [
        ("<SearchTrigger tour className={railControlClass()} />",
         "<SearchTrigger tour className=\"mb-2 flex h-10 w-full items-center gap-3 rounded-xl border border-border bg-secondary/50 ps-2.5 pe-2 text-sm text-muted-foreground transition-colors hover:bg-accent hover:text-accent-foreground [&>svg]:size-4 [&>kbd]:rounded-md [&>kbd]:border [&>kbd]:border-border [&>kbd]:bg-background [&>kbd]:px-1.5\" />"),
    ],
    # launch plan: off (a one-person project needs no onboarding checklist)
    "lib/shared/launch-checklist.ts": [
        ("  return status?.inLaunchWindow === true && isLaunchPlanActive(status)\n}",
         "  void status\n  void isLaunchPlanActive\n  return false\n}"),
    ],
    "components/onboarding/launch-plan-dock.tsx": [
        ("export function LaunchPlanDock() {\n", "export function LaunchPlanDock() {\n  if (Date.now() > 0) return null\n"),
        ("export function LaunchPlanInHelp({ children }: { children: ReactNode }) {\n", "export function LaunchPlanInHelp({ children }: { children: ReactNode }) {\n  if (Date.now() > 0) return null\n"),
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
    # no help-center hero/search above articles: the docs layout must be the direct parent
    "routes/_portal/hc/$locale/articles/$idSlug.tsx": [
        ("""  return (
    <>
      <HelpCenterHero variant="compact">
        <HelpCenterHeroSearch locale={locale} askAiEnabled={askAiEnabled} />
      </HelpCenterHero>
      <div className="mx-auto max-w-7xl">
        <Outlet />
      </div>
    </>
  )""", """  void locale
  void askAiEnabled
  return <Outlet />"""),
    ],
    "routes/_portal/hc/$locale/articles/$idSlug/index.tsx": [
        ("import { getPublicArticlePageFn } from '@/lib/server/functions/help-center'\n",
         "import { getPublicArticlePageFn } from '@/lib/server/functions/help-center'\nimport { DocsArticlePage } from '@/custom-docs/pages'\n"),
        ("  component: ArticleDetailPage,\n", "  component: DocsArticlePage,\n"),
    ],
    # one copy of fumadocs-core: bun can install two (different peer sets), and two copies mean
    # two React contexts ("wrap your application inside FrameworkProvider")
    "../vite.config.ts": [
        ("    resolve: {\n      tsconfigPaths: true,\n    },\n",
         "    resolve: {\n      tsconfigPaths: true,\n      dedupe: ['fumadocs-core', '@fumadocs/base-ui', '@base-ui/react', '@fuma-translate/react'],\n    },\n"),
    ],
    # styles: Fumadocs theme mapped onto the portal theme
    "globals.css": [
        ('@import "tw-animate-css";\n', '@import "tw-animate-css";\n@import "./custom-docs/fumadocs.css";\n@import "./custom-docs/theme.css";\n'),
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
        ("  return (\n    (parsed as { features?: { allowAnonymous?: boolean } } | null | undefined)?.features\n      ?.allowAnonymous === true\n  )",
         "  // Custom: anonymous posting and voting are off for good (review bombing).\n  void parsed\n  return false"),
        ("    label: 'Help Center',\n    description: 'Publish searchable help articles so customers can find answers themselves.',",
         "    label: 'Docs',\n    description: 'Documentation with a sidebar, search (Ctrl+K), page outline and translations.',"),
    ],
}

# Regions replaced whole: (file, start marker, end marker, new text). Both markers must be unique.
HEADER = """  // Custom: the portal header is the top bar of the page panel, like the docs' bar:
  // logo on the left, navigation and account on the right. On wide screens the panel has a
  // fixed height and scrolls inside (theme.css), so a new page starts at its top.
  const panelPath = useRouterState({ select: (s) => s.location.pathname })
  useEffect(() => {
    document.querySelector('[data-portal]')?.scrollTo({ top: 0 })
  }, [panelPath])
  const headerNav = (
    <Navigation
      navItems={navItems}
      helpHeaderLinks={settings?.helpCenterConfig?.headerLinks}
      supportUnreadTotal={supportUnreadTotal}
    />
  )
  return (
    // A banner landmark, so the logo and account controls sit in one.
    <header ref={headerRef} className="portal-header sticky top-0 z-40 w-full">
      <div className="flex h-14 w-full items-center gap-3 ps-4 pe-3 sm:ps-6">
        <Link to="/" className="portal-header__logo flex min-w-0 items-center gap-2 me-auto">
          {orgLogo ? (
            <img
              src={orgLogo}
              alt={orgName}
              className="h-7 w-7 [border-radius:calc(var(--radius)*0.6)]"
            />
          ) : (
            <div className="h-7 w-7 [border-radius:calc(var(--radius)*0.6)] bg-primary flex items-center justify-center text-sm text-primary-foreground font-semibold">
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
        <div className="shrink-0">{authButtons}</div>
      </div>
      <div className="flex h-11 items-center overflow-x-auto px-4 scrollbar-none sm:px-6 lg:hidden">
        {headerNav}
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
