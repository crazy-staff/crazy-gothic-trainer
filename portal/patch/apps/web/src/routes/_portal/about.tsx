import { createFileRoute } from '@tanstack/react-router'
import { AboutPage } from '@/custom-docs/about'

/** /about: the technologies behind the site and links to its source. */
export const Route = createFileRoute('/_portal/about')({
  head: () => ({ meta: [{ title: 'About' }] }),
  component: AboutPage,
})
