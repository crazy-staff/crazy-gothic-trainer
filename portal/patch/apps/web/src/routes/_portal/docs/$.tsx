import { createFileRoute, notFound } from '@tanstack/react-router'
import { getGothicDocsPageFn } from '@/gothic-docs/docs-fns'
import { GothicDocsPage } from '@/gothic-docs/docs-page'

export const Route = createFileRoute('/_portal/docs/$')({
  loader: async ({ params }) => {
    const page = await getGothicDocsPageFn({ data: { slug: params._splat ?? '' } })
    if (!page) throw notFound()
    return { page }
  },
  head: ({ loaderData }) => ({
    meta: loaderData
      ? [{ title: `${loaderData.page.title} - Docs` }, ...(loaderData.page.description ? [{ name: 'description', content: loaderData.page.description }] : [])]
      : [],
  }),
  component: () => <GothicDocsPage page={Route.useLoaderData().page} />,
})
