/** Server only (imported lazily from server functions and API routes). */
import type { DocsNavCategory } from './types'

export async function loadDocsNav(locale?: string): Promise<DocsNavCategory[]> {
  const { listPublicCategoriesForLocale, listPublicArticlesForCategoryLocale } = await import(
    '@/lib/server/domains/help-center/help-center-locale.query'
  )
  const { resolveHelpPublicViewer } = await import('@/lib/server/functions/help-center')
  const { DEFAULT_LOCALE } = await import('@/lib/shared/i18n')
  const loc = locale ?? DEFAULT_LOCALE
  const viewer = await resolveHelpPublicViewer().catch(() => undefined)
  const categories = (await listPublicCategoriesForLocale(loc, viewer))
    .filter((c) => c.recursivePublishedArticleCount > 0)
    .sort((a, b) => a.position - b.position)
  return Promise.all(
    categories.map(async (c) => {
      const articles = (await listPublicArticlesForCategoryLocale(c.id, loc, viewer))
        .slice()
        .sort((a, b) => (a.position ?? 1e9) - (b.position ?? 1e9) || a.title.localeCompare(b.title))
      return {
        id: String(c.id),
        name: c.name,
        slug: c.slug,
        urlId: c.urlId,
        icon: (c as { icon?: string | null }).icon ?? null,
        description: (c as { description?: string | null }).description ?? null,
        parentId: c.parentId ? String(c.parentId) : null,
        articles: articles.map((a) => ({
          id: String(a.id),
          title: a.title,
          slug: a.slug,
          urlId: a.urlId,
          description: (a as { description?: string | null }).description ?? null,
        })),
      }
    })
  )
}
