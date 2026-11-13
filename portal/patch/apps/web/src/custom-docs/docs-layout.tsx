/**
 * The docs shell: Fumadocs layouts (from @fumadocs/base-ui, the package fumadocs.dev uses)
 * inside Quackback's portal. The page tree is built from the published categories and
 * articles, wrapped in two levels of root folders that become the switchers at the top of the
 * sidebar: the game (Gothic 1, Gothic 2 later) and the renderer.
 * Ctrl+K / the sidebar search opens the search dialog over /api/docs-search.
 */
import type { ReactNode } from 'react'
import { useMemo } from 'react'
import { TanstackProvider } from 'fumadocs-core/framework/tanstack'
import type * as PageTree from 'fumadocs-core/page-tree'
import { SearchProvider } from '@fumadocs/base-ui/contexts/search'
import { DocsLayout as SpaciousLayout } from '@fumadocs/base-ui/layouts/spacious'
import { CpuChipIcon, HomeIcon } from '@heroicons/react/20/solid'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { hcArticlePath, localizedHcPath } from '@/lib/shared/help-center-url'
import { createDocsSearchDialog } from './search-dialog'
import { NavTitle, PortalBarActions } from './shell-parts'
import type { DocsNavCategory } from './types'

/** Placeholder pages for switcher options that have no docs yet (route: hc/soon/$what). */
export const SOON: Record<string, { title: string; description: string }> = {
  'gothic-2': { title: 'Gothic 2', description: 'The trainer for Gothic 2 is planned. Its docs will live here.' },
  'original-renderer': {
    title: 'Original renderer',
    description: 'Notes for playing without GD3D11 (the game’s own DirectX 7 renderer) will live here.',
  },
}
export const soonPath = (what: string) => `/hc/soon/${what}`

function GameIcon({ n }: { n: string }) {
  return (
    <span className="flex size-full items-center justify-center rounded-md bg-fd-primary/15 text-[0.65rem] font-bold text-fd-primary">
      G{n}
    </span>
  )
}

export function docsTree(nav: DocsNavCategory[], locale: string): PageTree.Root {
  const pages: PageTree.Node[] = [
    { type: 'page', $id: 'overview', name: 'Overview', url: localizedHcPath(locale, '/hc'), icon: <HomeIcon /> },
  ]
  for (const c of nav) {
    pages.push({
      type: 'separator',
      $id: `sep-${c.id}`,
      name: c.name,
      icon: c.icon ? <CategoryIcon icon={c.icon} /> : undefined,
    })
    for (const a of c.articles) {
      pages.push({
        type: 'page',
        $id: `page-${a.id}`,
        name: a.title,
        url: hcArticlePath({ locale, urlId: a.urlId, slug: a.slug }),
      })
    }
  }
  const soonFolder = (id: string, root: string, icon: ReactNode, description: string): PageTree.Folder => ({
    type: 'folder',
    $id: id,
    root,
    name: SOON[id].title,
    description,
    icon,
    children: [{ type: 'page', $id: `${id}-soon`, name: 'Coming soon', url: soonPath(id) }],
  })
  return {
    $id: 'docs',
    name: 'Docs',
    children: [
      {
        type: 'folder',
        $id: 'gothic-1',
        root: 'game',
        name: 'Gothic 1',
        description: 'Gothic 1 Crazy Trainer',
        icon: <GameIcon n="1" />,
        children: [
          {
            type: 'folder',
            $id: 'gd3d11',
            root: 'renderer',
            name: 'GD3D11',
            description: 'DirectX 11 renderer',
            icon: <CpuChipIcon />,
            children: pages,
          },
          soonFolder('original-renderer', 'renderer', <CpuChipIcon />, 'Without GD3D11 · coming later'),
        ],
      },
      soonFolder('gothic-2', 'game', <GameIcon n="2" />, 'Coming later'),
    ],
  }
}

function Shell({ nav, locale, children }: { nav: DocsNavCategory[]; locale: string; children: ReactNode }) {
  const tree = useMemo(() => docsTree(nav, locale), [nav, locale])
  const SearchDialog = useMemo(
    () =>
      createDocsSearchDialog(
        nav.map((c) => ({ name: c.name, description: c.description ?? undefined, value: c.slug })),
        nav.flatMap((c) =>
          c.articles.map((a) => ({ name: a.title, url: hcArticlePath({ locale, urlId: a.urlId, slug: a.slug }) }))
        )
      ),
    [nav, locale]
  )
  return (
    <SearchProvider SearchDialog={SearchDialog}>
      <div className="custom-docs">
        <SpaciousLayout
          tree={tree}
          themeSwitch={{ enabled: false }}
          nav={{ title: <NavTitle />, url: '/' }}
          slots={{ actions: PortalBarActions }}
        >
          {children}
        </SpaciousLayout>
      </div>
    </SearchProvider>
  )
}

export function CustomDocsLayout({
  nav,
  locale,
  children,
}: {
  nav: DocsNavCategory[]
  locale: string
  children: ReactNode
}) {
  return (
    <TanstackProvider>
      <Shell nav={nav} locale={locale}>
        {children}
      </Shell>
    </TanstackProvider>
  )
}
