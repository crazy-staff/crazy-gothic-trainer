/**
 * The frame of every portal page outside the docs: the same Fumadocs "spacious" layout as the
 * docs (sidebar + panel with the top bar), so the whole site reads as one app.
 *   sidebar: workspace, search (docs), the portal sections, then the page's own panel
 *            (filters, a settings menu, ...) that pages send with <SidebarSlot>
 *   panel:   the page, sliding in from the side of its link (page-transition.tsx)
 * The docs (/hc) bring their own sidebar tree and are passed through untouched.
 */
import type { ReactNode } from 'react'
import { useMemo } from 'react'
import { useRouterState } from '@tanstack/react-router'
import { useIntl } from 'react-intl'
import { TanstackProvider } from 'fumadocs-core/framework/tanstack'
import type * as PageTree from 'fumadocs-core/page-tree'
import { SearchProvider } from '@fumadocs/base-ui/contexts/search'
import { DocsLayout as SpaciousLayout } from '@fumadocs/base-ui/layouts/spacious'
import { DocsPage } from '@fumadocs/base-ui/layouts/spacious/page'
import {
  BookOpenIcon,
  ChatBubbleLeftRightIcon,
  LifebuoyIcon,
  LinkIcon,
  MapIcon,
  MegaphoneIcon,
  SignalIcon,
} from '@heroicons/react/20/solid'
import { usePortalNavItems } from '@/components/public/use-portal-nav-items'
import { createDocsSearchDialog } from './search-dialog'
import { NavTitle, PortalBarActions, navItemLabel, useSectionIndex } from './shell-parts'
import { PageTransition } from './page-transition'
import { SIDEBAR_SLOT_ID } from './sidebar-slot'

const ICONS: Record<string, ReactNode> = {
  feedback: <ChatBubbleLeftRightIcon />,
  roadmap: <MapIcon />,
  changelog: <MegaphoneIcon />,
  help: <BookOpenIcon />,
  support: <LifebuoyIcon />,
  status: <SignalIcon />,
}

export function isDocsPath(pathname: string) {
  return pathname === '/hc' || pathname.startsWith('/hc/') || pathname === '/docs'
}

/** The docs' search, usable from every page (without the docs' category list). */
const PortalSearchDialog = createDocsSearchDialog([], [])

function Frame({ children }: { children: ReactNode }) {
  const intl = useIntl()
  const items = usePortalNavItems()
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const section = useSectionIndex(pathname)
  const tree = useMemo<PageTree.Root>(
    () => ({
      $id: 'portal',
      name: 'Portal',
      children: [
        ...items.map(
          (item): PageTree.Node => ({
            type: 'page',
            $id: `nav-${item.id}`,
            name: navItemLabel(item, intl.formatMessage),
            url: item.kind === 'builtin' ? item.to : item.href,
            external: item.kind === 'link',
            icon: item.kind === 'builtin' ? ICONS[item.type] : <LinkIcon />,
          })
        ),
        // the page's own panel lands here (SidebarSlot)
        { type: 'separator', $id: 'page-panel', name: <span id={SIDEBAR_SLOT_ID} className="app-sidebar-slot" /> },
      ],
    }),
    [items, intl]
  )
  const depth = pathname.split('/').filter(Boolean).length
  return (
    <SearchProvider SearchDialog={PortalSearchDialog}>
      <div className="custom-docs app-shell">
        <SpaciousLayout
          tree={tree}
          tabs={false}
          themeSwitch={{ enabled: false }}
          nav={{ title: <NavTitle />, url: '/' }}
          slots={{ actions: PortalBarActions }}
        >
          <DocsPage
            full
            tableOfContent={{ enabled: false }}
            tableOfContentPopover={{ enabled: false }}
            breadcrumb={{ enabled: false }}
            footer={{ enabled: false }}
            className="app-page"
          >
            <PageTransition id={pathname} position={[section, depth]} className="app-page-content">
              {children}
            </PageTransition>
          </DocsPage>
        </SpaciousLayout>
      </div>
    </SearchProvider>
  )
}

export function AppShell({ children }: { children: ReactNode }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  if (isDocsPath(pathname)) return <>{children}</>
  return (
    <TanstackProvider>
      <Frame>{children}</Frame>
    </TanstackProvider>
  )
}
