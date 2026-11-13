import { createFileRoute } from '@tanstack/react-router'

/**
 * GET /api/docs-search?query=...&tag=a,b&kind=all|pages|headings
 * Fumadocs search over the published docs. `tag`: every tag must match (category, game,
 * renderer); `kind`: pages only, pages and headings, or everything (default).
 */
export const Route = createFileRoute('/api/docs-search')({
  server: {
    handlers: {
      GET: async ({ request }) => {
        const { docsSearchApi } = await import('@/custom-docs/search-index')
        const api = await docsSearchApi()
        const url = new URL(request.url)
        const query = url.searchParams.get('query') ?? ''
        const tag = (url.searchParams.get('tag') ?? '').split(',').filter(Boolean)
        const kind = url.searchParams.get('kind') ?? 'all'
        if (!query.trim()) return Response.json([])
        let results = await api.search(query, { tag } as never)
        if (kind === 'pages') results = results.filter((r) => r.type === 'page')
        else if (kind === 'headings') results = results.filter((r) => r.type !== 'text')
        return Response.json(results)
      },
    },
  },
})
