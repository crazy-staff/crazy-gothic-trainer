/**
 * Server only. Fumadocs search (Orama, "advanced" mode: pages and their sections) over the
 * published docs. Rebuilt at most once a minute, so new articles show up without a restart.
 */
import { createSearchAPI } from 'fumadocs-core/search/server'
import type { JSONContent } from '@tiptap/core'
import { structuredDataOf } from './structure'

type Api = ReturnType<typeof createSearchAPI>
let cached: { api: Api; at: number } | null = null
const TTL = 60_000

async function buildIndexes() {
  const { loadDocsNav } = await import('./nav-loader')
  const { db, helpCenterArticles, inArray } = await import('@/lib/server/db')
  const { hcArticlePath } = await import('@/lib/shared/help-center-url')
  const { DEFAULT_LOCALE } = await import('@/lib/shared/i18n')
  const nav = await loadDocsNav()
  const ids = nav.flatMap((c) => c.articles.map((a) => a.id))
  if (ids.length === 0) return []
  const rows = await db
    .select({ id: helpCenterArticles.id, contentJson: helpCenterArticles.contentJson })
    .from(helpCenterArticles)
    .where(inArray(helpCenterArticles.id, ids as never[]))
  const content = new Map(rows.map((r) => [String(r.id), r.contentJson as JSONContent | null]))
  return nav.flatMap((c) =>
    c.articles.map((a) => ({
      id: a.id,
      title: a.title,
      description: a.description ?? undefined,
      breadcrumbs: [c.name],
      tag: c.slug,
      url: hcArticlePath({ locale: DEFAULT_LOCALE, urlId: a.urlId, slug: a.slug }),
      structuredData: structuredDataOf(content.get(a.id)),
    }))
  )
}

export async function docsSearchApi(): Promise<Api> {
  if (cached && Date.now() - cached.at < TTL) return cached.api
  const indexes = await buildIndexes()
  const api = createSearchAPI('advanced', { indexes })
  cached = { api, at: Date.now() }
  return api
}
