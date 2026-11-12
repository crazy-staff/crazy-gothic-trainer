/**
 * Gothic 1 Crazy Trainer: docs navigation (every public category with its articles),
 * for the left sidebar of the documentation (Quackback help center restyled as docs).
 */
import { createServerFn } from '@tanstack/react-start'
import { z } from 'zod'
import type { DocsNavCategory } from './docs-types'

export const getGothicDocsNavFn = createServerFn({ method: 'GET' })
  .validator(z.object({ locale: z.string().optional() }))
  .handler(async ({ data }): Promise<DocsNavCategory[]> => {
    const { listPublicCategoriesForLocale, listPublicArticlesForCategoryLocale } =
      await import('@/lib/server/domains/help-center/help-center-locale.query')
    const { resolveHelpPublicViewer } = await import('@/lib/server/functions/help-center')
    const { DEFAULT_LOCALE } = await import('@/lib/shared/i18n')
    const locale = data.locale ?? DEFAULT_LOCALE
    const viewer = await resolveHelpPublicViewer()
    const categories = (await listPublicCategoriesForLocale(locale, viewer))
      .filter((c) => c.recursivePublishedArticleCount > 0)
      .sort((a, b) => a.position - b.position)
    const withArticles = await Promise.all(
      categories.map(async (c) => {
        const articles = (await listPublicArticlesForCategoryLocale(c.id, locale, viewer))
          .slice()
          .sort((a, b) => (a.position ?? 1e9) - (b.position ?? 1e9) || a.title.localeCompare(b.title))
        return {
          id: String(c.id),
          name: c.name,
          slug: c.slug,
          urlId: c.urlId,
          parentId: c.parentId ? String(c.parentId) : null,
          articles: articles.map((a) => ({ title: a.title, slug: a.slug, urlId: a.urlId })),
        }
      })
    )
    return withArticles
  })
