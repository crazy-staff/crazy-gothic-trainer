import { createFileRoute, redirect } from '@tanstack/react-router'

// Short link: /docs -> the docs (Quackback's help center, restyled as documentation).
export const Route = createFileRoute('/_portal/docs')({
  beforeLoad: () => {
    throw redirect({ href: '/hc', statusCode: 302 })
  },
})
