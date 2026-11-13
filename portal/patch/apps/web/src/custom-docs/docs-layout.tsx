/** The docs' page tree for the app sidebar, built from the published categories and articles. */
import type * as PageTree from 'fumadocs-core/page-tree'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { hcArticlePath } from '@/lib/shared/help-center-url'
import type { DocsNavCategory } from './types'

export function docsTree(nav: DocsNavCategory[], locale: string): PageTree.Root {
  const children: PageTree.Node[] = []
  for (const c of nav) {
    children.push({
      type: 'separator',
      $id: `sep-${c.id}`,
      name: c.name,
      icon: c.icon ? <CategoryIcon icon={c.icon} /> : undefined,
    })
    for (const a of c.articles)
      children.push({
        type: 'page',
        $id: `page-${a.id}`,
        name: a.title,
        url: hcArticlePath({ locale, urlId: a.urlId, slug: a.slug }),
      })
  }
  return { $id: 'docs', name: 'Docs', children }
}
