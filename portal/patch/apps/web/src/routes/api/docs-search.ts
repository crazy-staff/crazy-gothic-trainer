import { createFileRoute } from '@tanstack/react-router'

/** GET /api/docs-search?query=...&tag=... : fumadocs search over the published docs. */
export const Route = createFileRoute('/api/docs-search')({
  server: {
    handlers: {
      GET: async ({ request }) => {
        const { docsSearchApi } = await import('@/custom-docs/search-index')
        const api = await docsSearchApi()
        return api.GET(request)
      },
    },
  },
})
