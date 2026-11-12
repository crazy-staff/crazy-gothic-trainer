/**
 * Gothic 1 Crazy Trainer docs - server side.
 * Pages come from ./manifest.ts, generated at image build time from the trainer repo
 * (docs/pages/**.md + meta.json). Markdown is turned into TipTap JSON with the same
 * converter the help center uses, so pages render exactly like help-center articles.
 * Added on top of Quackback by gothic1-crazy-trainer/portal - new files only.
 */
import { createServerFn } from '@tanstack/react-start'
import { z } from 'zod'
import type { DocsNavSection, DocsPageData } from './docs.types'

async function load() {
  const { DOCS_PAGES, DOCS_SECTIONS } = await import('./manifest')
  return { pages: DOCS_PAGES, sections: DOCS_SECTIONS }
}

export const getGothicDocsNavFn = createServerFn({ method: 'GET' }).handler(
  async (): Promise<DocsNavSection[]> => {
    const { pages, sections } = await load()
    const bySlug = new Map(pages.map((p) => [p.slug, p]))
    return sections.map((s) => ({
      title: s.title,
      items: s.pages
        .map((slug) => bySlug.get(slug))
        .filter((p): p is NonNullable<typeof p> => !!p)
        .map((p) => ({ slug: p.slug, title: p.title })),
    }))
  }
)

export const getGothicDocsPageFn = createServerFn({ method: 'GET' })
  .validator(z.object({ slug: z.string().max(200) }))
  .handler(async ({ data }): Promise<DocsPageData | null> => {
    const { pages, sections } = await load()
    const order = sections.flatMap((s) => s.pages)
    const page = pages.find((p) => p.slug === (data.slug.replace(/^\/+|\/+$/g, '') || 'index'))
    if (!page) return null
    const { markdownToTiptapJson } = await import('@/lib/server/markdown-tiptap')
    const i = order.indexOf(page.slug)
    const link = (slug?: string) => {
      const p = slug ? pages.find((x) => x.slug === slug) : undefined
      return p ? { slug: p.slug, title: p.title } : null
    }
    return {
      slug: page.slug,
      title: page.title,
      description: page.description,
      section: sections.find((s) => s.pages.includes(page.slug))?.title ?? null,
      contentJson: markdownToTiptapJson(page.markdown) as DocsPageData['contentJson'],
      prev: i > 0 ? link(order[i - 1]) : null,
      next: i >= 0 ? link(order[i + 1]) : null,
      editUrl: page.source ? `https://github.com/dawidseipold/gothic1-crazy-trainer/blob/main/${page.source}` : null,
    }
  })
