/**
 * The frame of every portal page, docs included: Fumadocs' "spacious" layout with our sidebar
 * (sidebar.tsx) and a top bar that only holds notifications and the account.
 * Mounted once (routes/_portal.tsx), so moving between sections never rebuilds the sidebar;
 * on docs pages the docs tree slides in under the sections.
 */
import type { ReactNode } from 'react'
import { useMemo } from 'react'
import { useMatches, useRouterState } from '@tanstack/react-router'
import { useQuery } from '@tanstack/react-query'
import { TanstackProvider } from 'fumadocs-core/framework/tanstack'
import type * as PageTree from 'fumadocs-core/page-tree'
import { SearchProvider } from '@fumadocs/base-ui/contexts/search'
import { DocsLayout as SpaciousLayout } from '@fumadocs/base-ui/layouts/spacious'
import { DocsPage } from '@fumadocs/base-ui/layouts/spacious/page'
import { useWorkspaceSettings } from '@/lib/client/hooks/use-root-context'
import { hcArticlePath } from '@/lib/shared/help-center-url'
import { getDocsNavFn } from './docs-fns'
import { docsTree } from './docs-layout'
import { createSearchDialog } from './search-dialog'
import { BarActions, NavTitle, useSectionIndex } from './shell-parts'
import { PageTransition } from './page-transition'
import { Sidebar, SidebarDrawer, SidebarProvider } from './sidebar'
import type { DocsNavCategory } from './types'

const EMPTY_TREE: PageTree.Root = { $id: 'none', name: 'Portal', children: [] }
const SIDEBAR_SLOTS = { provider: SidebarProvider, main: Sidebar, drawer: SidebarDrawer }

export function isDocsPath(pathname: string) {
  return pathname === '/hc' || pathname.startsWith('/hc/')
}

/** The docs navigation: from the docs route's loader on docs pages, fetched once elsewhere. */
export function useDocsNav(): DocsNavCategory[] {
  const settings = useWorkspaceSettings()
  const fromLoader = useMatches({
    select: (matches) =>
      (matches.find((m) => m.routeId === '/_portal/hc')?.loaderData as { docsNav?: DocsNavCategory[] } | undefined)?.docsNav,
  })
  const { data } = useQuery({
    queryKey: ['docs-nav'],
    queryFn: () => getDocsNavFn({ data: {} }),
    initialData: fromLoader,
    staleTime: 5 * 60_000,
    enabled: !!settings?.featureFlags?.helpCenter,
  })
  return fromLoader ?? data ?? []
}

export function AppShell({ children }: { children: ReactNode }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const onDocs = isDocsPath(pathname)
  const nav = useDocsNav()
  const locale = 'en'
  const tree = useMemo(() => (onDocs ? docsTree(nav, locale) : EMPTY_TREE), [onDocs, nav])
  const SearchDialog = useMemo(
    () =>
      createSearchDialog(
        nav.map((c) => ({ name: c.name, description: c.description ?? undefined, value: c.slug })),
        nav.flatMap((c) =>
          c.articles.map((a) => ({ name: a.title, url: hcArticlePath({ locale, urlId: a.urlId, slug: a.slug }) }))
        )
      ),
    [nav]
  )
  const section = useSectionIndex(pathname)
  const depth = pathname.split('/').filter(Boolean).length
  return (
    <TanstackProvider>
      <SearchProvider SearchDialog={SearchDialog}>
        <div className="custom-docs">
          <SpaciousLayout
            tree={tree}
            tabs={false}
            themeSwitch={{ enabled: false }}
            nav={{ title: <NavTitle />, url: '/' }}
            slots={{ actions: BarActions, sidebar: SIDEBAR_SLOTS }}
          >
            {onDocs ? (
              children
            ) : (
              <DocsPage
                full
                tableOfContent={{ enabled: false }}
                tableOfContentPopover={{ enabled: false }}
                breadcrumb={{ enabled: false }}
                footer={{ enabled: false }}
                className="app-page"
              >
                <PageTransition id={pathname.startsWith('/settings') ? '/settings' : pathname} position={section * 1000 + depth} className="app-page-content">
                  {children}
                </PageTransition>
              </DocsPage>
            )}
          </SpaciousLayout>
        </div>
      </SearchProvider>
    </TanstackProvider>
  )
}
