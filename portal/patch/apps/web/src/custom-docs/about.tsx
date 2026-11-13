/** /about: what this site is built with and where its source lives (it is open source). */
import { Card, Cards } from '@fumadocs/base-ui/components/card'
import { Stagger } from './page-transition'

const SOURCE = [
  {
    title: 'This portal',
    description: 'Quackback with docs on Fumadocs and this design. AGPL-3.0.',
    href: 'https://github.com/dawidseipold/quackback-fumadocs-custom',
  },
  {
    title: 'Gothic 1 Crazy Trainer',
    description: 'The trainer itself, its docs pages and releases.',
    href: 'https://github.com/dawidseipold/gothic-crazy-trainer',
  },
]
const TECH = [
  ['Quackback', 'Feedback, roadmap, changelog, support and status. AGPL-3.0.', 'https://github.com/QuackbackIO/quackback'],
  ['Fumadocs', 'Docs layout, sidebar, search dialog and article components. MIT.', 'https://fumadocs.dev'],
  ['TanStack Start', 'The React framework the app runs on.', 'https://tanstack.com/start'],
  ['React', 'User interface.', 'https://react.dev'],
  ['Tailwind CSS', 'Styling.', 'https://tailwindcss.com'],
  ['Base UI', 'Menus, dialogs and other unstyled components.', 'https://base-ui.com'],
  ['Motion', 'Page transitions and animations.', 'https://motion.dev'],
  ['Orama', 'Full-text search over docs, ideas and the changelog.', 'https://orama.com'],
  ['Shiki', 'Code highlighting.', 'https://shiki.style'],
  ['PostgreSQL', 'Database.', 'https://www.postgresql.org'],
  ['Bun', 'Runtime and package manager.', 'https://bun.sh'],
] as const

export function AboutPage() {
  return (
    <div className="mx-auto w-full max-w-4xl px-4 py-8 sm:px-6">
      <p className="text-sm font-medium text-fd-primary">Open source</p>
      <h1 className="mt-1 text-3xl font-semibold tracking-tight">About this site</h1>
      <p className="mt-2 max-w-2xl text-fd-muted-foreground">
        Everything here is open source: the trainer, its documentation and this portal. Below is the source and the
        projects it stands on.
      </p>
      <h2 className="mt-10 mb-4 text-lg font-semibold">Source code</h2>
      <Cards>
        {SOURCE.map((s) => (
          <Card key={s.href} title={s.title} description={s.description} href={s.href} external />
        ))}
      </Cards>
      <h2 className="mt-10 mb-4 text-lg font-semibold">Built with</h2>
      <Stagger className="grid gap-3 sm:grid-cols-2">
        {TECH.map(([name, what, href]) => (
          <a
            key={name}
            href={href}
            target="_blank"
            rel="noopener noreferrer"
            className="block h-full rounded-xl border border-fd-border bg-fd-card p-4 transition-colors hover:bg-fd-accent/80"
          >
            <span className="block text-sm font-medium">{name}</span>
            <span className="mt-1 block text-sm text-fd-muted-foreground">{what}</span>
          </a>
        ))}
      </Stagger>
    </div>
  )
}
