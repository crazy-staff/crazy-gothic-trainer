import { createFileRoute, notFound } from '@tanstack/react-router'
import { getGothicDocsPageFn } from '@/gothic-docs/docs.server'
import { GothicDocsPage } from '@/gothic-docs/docs-page'

export const Route = createFileRoute('/_portal/docs/')({
  loader: async () => {
    const page = await getGothicDocsPageFn({ data: { slug: 'index' } })
    if (!page) throw notFound()
    return { page }
  },
  head: ({ loaderData }) => ({ meta: loaderData ? [{ title: `${loaderData.page.title} - Docs` }] : [] }),
  component: () => <GothicDocsPage page={Route.useLoaderData().page} />,
})
