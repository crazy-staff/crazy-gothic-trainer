/**
 * The docs shell: Fumadocs layouts (from @fumadocs/base-ui, the package fumadocs.dev uses)
 * inside Quackback's portal. The page tree is built from the published categories and
 * articles, wrapped in two levels of root folders that become the switchers at the top of the
 * sidebar: the game (Gothic 1, Gothic 2 later) and the renderer.
 * Ctrl+K / the sidebar search opens the search dialog over /api/docs-search.
 */
import type { ComponentProps, ReactNode } from 'react'
import { useMemo } from 'react'
import { Link as RouterLink } from '@tanstack/react-router'
import { TanstackProvider } from 'fumadocs-core/framework/tanstack'
import type * as PageTree from 'fumadocs-core/page-tree'
import { SearchProvider } from '@fumadocs/base-ui/contexts/search'
import { DocsLayout as SpaciousLayout } from '@fumadocs/base-ui/layouts/spacious'
import { HeaderActions } from '@fumadocs/base-ui/layouts/spacious/slots/actions'
import { DocsLayout as ClassicLayout } from '@fumadocs/base-ui/layouts/docs'
import { BookOpenIcon, CpuChipIcon, HomeIcon, ShieldCheckIcon } from '@heroicons/react/20/solid'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { usePortalNavItems } from '@/components/public/use-portal-nav-items'
import { useUserRole, useWorkspaceSettings } from '@/lib/client/hooks/use-root-context'
import { hcArticlePath, localizedHcPath } from '@/lib/shared/help-center-url'
import { createDocsSearchDialog } from './search-dialog'
import { DocsVariantProvider, VariantPicker, useDocsVariant } from './variant'
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

function NavTitle({ withOrg }: { withOrg: boolean }) {
  const settings = useWorkspaceSettings() as { name?: string } | undefined
  if (withOrg && settings?.name)
    return (
      <span className="inline-flex min-w-0 items-center gap-2">
        <span className="flex size-6 shrink-0 items-center justify-center rounded-md bg-fd-primary text-xs font-bold text-fd-primary-foreground">
          {settings.name.slice(0, 1).toUpperCase()}
        </span>
        <span className="truncate">{settings.name}</span>
      </span>
    )
  return (
    <span className="inline-flex items-center gap-2">
      <BookOpenIcon className="size-4 text-fd-primary" />
      Documentation
    </span>
  )
}

/** Variant b: the portal's own links in the page bar, next to the Fumadocs actions. */
function PortalBarActions({ className, ...props }: ComponentProps<'div'>) {
  const items = usePortalNavItems()
  const role = useUserRole() as string | null | undefined
  const team = role === 'admin' || role === 'member' || role === 'owner'
  return (
    <div className={'flex min-w-0 items-center gap-1 ' + (className ?? '')} {...props}>
      <nav className="flex min-w-0 items-center gap-0.5 overflow-x-auto rounded-lg bg-fd-background max-lg:hidden">
        {items.map((item) => {
          const href = item.kind === 'builtin' ? item.to : item.href
          const label = item.kind === 'builtin' ? (item.label ?? item.defaultMessage) : item.label
          const docs = item.kind === 'builtin' && item.type === 'help'
          return (
            <RouterLink
              key={item.id}
              to={href as '/'}
              className={
                'rounded-lg px-2.5 py-1 text-sm whitespace-nowrap transition-colors ' +
                (docs
                  ? 'bg-fd-primary/10 font-medium text-fd-primary'
                  : 'text-fd-muted-foreground hover:bg-fd-accent hover:text-fd-accent-foreground')
              }
            >
              {label}
            </RouterLink>
          )
        })}
      </nav>
      {team && (
        <a
          href="/admin"
          className="inline-flex items-center gap-1.5 rounded-lg border px-2.5 py-1 text-sm text-fd-muted-foreground transition-colors hover:bg-fd-accent hover:text-fd-accent-foreground"
        >
          <ShieldCheckIcon className="size-3.5" />
          Admin
        </a>
      )}
      <HeaderActions />
    </div>
  )
}

function Shell({ nav, locale, children }: { nav: DocsNavCategory[]; locale: string; children: ReactNode }) {
  const { variant } = useDocsVariant()
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
  const home = localizedHcPath(locale, '/hc')
  return (
    <SearchProvider SearchDialog={SearchDialog}>
      <div className="custom-docs" data-variant={variant}>
        {variant === 'c' ? (
          <ClassicLayout tree={tree} themeSwitch={{ enabled: false }} nav={{ title: <NavTitle withOrg={false} />, url: home }}>
            {children}
          </ClassicLayout>
        ) : (
          <SpaciousLayout
            tree={tree}
            themeSwitch={{ enabled: false }}
            nav={{ title: <NavTitle withOrg={variant === 'b'} />, url: home }}
            slots={variant === 'b' ? { actions: PortalBarActions } : undefined}
          >
            {children}
          </SpaciousLayout>
        )}
      </div>
      <VariantPicker />
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
      <DocsVariantProvider>
        <Shell nav={nav} locale={locale}>
          {children}
        </Shell>
      </DocsVariantProvider>
    </TanstackProvider>
  )
}
