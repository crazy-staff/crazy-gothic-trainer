import { createFileRoute } from '@tanstack/react-router'
import { DocsSoonPage } from '@/custom-docs/pages'

/** /hc/soon/<what>: placeholder pages for the docs switchers (Gothic 2, original renderer). */
export const Route = createFileRoute('/_portal/hc/soon/$what')({
  component: function SoonRoute() {
    const { what } = Route.useParams()
    return <DocsSoonPage what={what} />
  },
})
