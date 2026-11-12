import { createFileRoute } from '@tanstack/react-router'
import { getGothicDocsNavFn } from '@/gothic-docs/docs.server'
import { GothicDocsLayout } from '@/gothic-docs/docs-layout'

// Gothic 1 Crazy Trainer docs inside the portal layout (added by gothic1-crazy-trainer/portal).
export const Route = createFileRoute('/_portal/docs')({
  loader: async () => ({ nav: await getGothicDocsNavFn() }),
  head: () => ({ meta: [{ title: 'Docs - Gothic 1 Crazy Trainer' }] }),
  component: DocsLayoutRoute,
})

function DocsLayoutRoute() {
  const { nav } = Route.useLoaderData()
  return <GothicDocsLayout nav={nav} />
}
