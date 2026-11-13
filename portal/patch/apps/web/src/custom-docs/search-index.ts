/**
 * Server only. Fumadocs search (Orama, "advanced" mode: pages and their sections) over the
 * published docs, the public ideas and the changelog. Rebuilt at most once a minute, so new articles show up without a restart.
 */
import { createSearchAPI } from 'fumadocs-core/search/server'
import type { JSONContent } from '@tiptap/core'
import { structuredDataOf } from './structure'
import { GAMES, RENDERERS, variantTag, type Variant } from './variants'

const available = (v: Variant) => v.status === 'available'

type Api = ReturnType<typeof createSearchAPI>
let cached: { api: Api; at: number } | null = null
const TTL = 60_000

async function buildIndexes() {
  const { loadDocsNav } = await import('./nav-loader')
  const { db, helpCenterArticles, inArray } = await import('@/lib/server/db')
  const { hcArticlePath } = await import('@/lib/shared/help-center-url')
  const { DEFAULT_LOCALE } = await import('@/lib/shared/i18n')
  const nav = await loadDocsNav().catch(() => [])
  const ids = nav.flatMap((c) => c.articles.map((a) => a.id))
  const rows = ids.length === 0 ? [] : await db
    .select({ id: helpCenterArticles.id, contentJson: helpCenterArticles.contentJson })
    .from(helpCenterArticles)
    .where(inArray(helpCenterArticles.id, ids as never[]))
  const content = new Map(rows.map((r) => [String(r.id), r.contentJson as JSONContent | null]))
  const plain = (content: string) => ({ headings: [], contents: content.trim() ? [{ heading: undefined, content: content.replace(/\s+/g, ' ').slice(0, 2000) }] : [] })
  // ideas (public boards only: listed as an anonymous visitor would see them)
  const posts: Index[] = []
  const seen = new Set<string>()
  try {
    const { listPublicPosts } = await import('@/lib/server/domains/posts/post.public')
    for (let page = 1; page <= 10; page++) {
      const res = await listPublicPosts({ sort: 'top', page, limit: 100 } as never)
      let added = 0
      for (const p of res.items as unknown as { id: string; title: string; content?: string | null; board: { name: string; slug: string } }[]) {
        if (seen.has(p.id)) continue
        seen.add(p.id)
        added++
        posts.push({
          id: `post-${p.id}`,
          title: p.title,
          breadcrumbs: ['Feedback', p.board.name],
          tag: ['section:feedback'],
          url: `/b/${p.board.slug}/posts/${p.id}`,
          structuredData: plain(p.content ?? ''),
        })
      }
      if (!res.hasMore || added === 0) break
    }
  } catch {
    // feedback is off or unreadable: search the rest
  }
  const changelog: Index[] = []
  try {
    const { listPublicChangelogs } = await import('@/lib/server/domains/changelog/changelog.public')
    const res = await listPublicChangelogs({ limit: 100 })
    for (const e of res.items as unknown as { id: string; title: string; content?: string | null }[])
      changelog.push({
        id: `changelog-${e.id}`,
        title: e.title,
        breadcrumbs: ['Changelog'],
        tag: ['section:changelog'],
        url: `/changelog/${e.id}`,
        structuredData: plain((e.content ?? '').replace(/[#*_>`\[\]()]/g, ' ')),
      })
  } catch {
    // changelog is off
  }
  const docs: Index[] = nav.flatMap((c) =>
    c.articles.map((a) => ({
      id: a.id,
      title: a.title,
      description: a.description ?? undefined,
      breadcrumbs: [c.name],
      // one tag per filter of the search dialog: category, game, renderer
      tag: ['section:docs', c.slug, ...GAMES.filter(available).map((g) => variantTag('game', g)), ...RENDERERS.filter(available).map((r) => variantTag('renderer', r))],
      url: hcArticlePath({ locale: DEFAULT_LOCALE, urlId: a.urlId, slug: a.slug }),
      structuredData: structuredDataOf(content.get(a.id)),
    }))
  )
  return [...new Map([...docs, ...posts, ...changelog].map((i) => [i.id, i])).values()]
}

interface Index {
  id: string
  title: string
  description?: string
  breadcrumbs?: string[]
  tag: string[]
  url: string
  structuredData: { headings: { id: string; content: string }[]; contents: { heading: string | undefined; content: string }[] }
}

export async function docsSearchApi(): Promise<Api> {
  if (cached && Date.now() - cached.at < TTL) return cached.api
  const indexes = await buildIndexes()
  const api = createSearchAPI('advanced', { indexes })
  cached = { api, at: Date.now() }
  return api
}
