import { createFileRoute, redirect } from '@tanstack/react-router'
import { isTeamMember } from '@/lib/shared/roles'
import { DesignSystemPage } from '@/custom-docs/design-system'

/** /design-system: the app's tokens and components, for the team. */
export const Route = createFileRoute('/_portal/design-system')({
  beforeLoad: ({ context }) => {
    if (!context.session?.user || !isTeamMember((context as { userRole?: string | null }).userRole as never)) {
      throw redirect({ to: '/' })
    }
  },
  head: () => ({ meta: [{ title: 'Design system' }, { name: 'robots', content: 'noindex' }] }),
  component: DesignSystemPage,
})
