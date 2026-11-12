/**
 * The docs shell: Fumadocs' docs layout (sidebar, search, page tree) inside Quackback's
 * portal, under its real header. The page tree is built from the published categories and
 * articles; Ctrl+K / the sidebar search opens Fumadocs' search dialog over /api/docs-search.
 */
import type { ReactNode } from 'react'
import { useMemo } from 'react'
import { TanstackProvider } from 'fumadocs-core/framework/tanstack'
import { SearchProvider } from 'fumadocs-ui/contexts/search'
import { DocsLayout } from 'fumadocs-ui/layouts/docs'
import { createDocsSearchDialog } from './search-dialog'
import type * as PageTree from 'fumadocs-core/page-tree'
import { BookOpenIcon } from '@heroicons/react/20/solid'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { hcArticlePath, localizedHcPath } from '@/lib/shared/help-center-url'
import type { DocsNavCategory } from './types'

export function docsTree(nav: DocsNavCategory[], locale: string): PageTree.Root {
  const children: PageTree.Node[] = []
  for (const c of nav) {
    children.push({
      type: 'separator',
      $id: `sep-${c.id}`,
      name: c.name,
      icon: c.icon ? <CategoryIcon icon={c.icon} className="size-4" /> : undefined,
    })
    for (const a of c.articles) {
      children.push({
        type: 'page',
        $id: `page-${a.id}`,
        name: a.title,
        url: hcArticlePath({ locale, urlId: a.urlId, slug: a.slug }),
      })
    }
  }
  return { $id: 'docs', name: 'Docs', children }
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
  const tree = useMemo(() => docsTree(nav, locale), [nav, locale])
  const SearchDialog = useMemo(
    () => createDocsSearchDialog(nav.map((c) => ({ name: c.name, value: c.slug }))),
    [nav]
  )
  const home = localizedHcPath(locale, '/hc')
  return (
    <TanstackProvider>
      <SearchProvider SearchDialog={SearchDialog}>
        <div className="custom-docs">
          <DocsLayout
            tree={tree}
            nav={{
              title: (
                <span className="inline-flex items-center gap-2">
                  <BookOpenIcon className="size-4 text-fd-primary" />
                  Documentation
                </span>
              ),
              url: home,
            }}
            themeSwitch={{ enabled: false }}
            sidebar={{ collapsible: true }}
            containerProps={{ className: 'custom-docs-layout' }}
          >
            {children}
          </DocsLayout>
        </div>
      </SearchProvider>
    </TanstackProvider>
  )
}
