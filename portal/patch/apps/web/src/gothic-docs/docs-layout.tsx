import { useMemo, useState } from 'react'
import { Link, Outlet, useRouterState } from '@tanstack/react-router'
import { MagnifyingGlassIcon, Bars3Icon } from '@heroicons/react/20/solid'
import { cn } from '@/lib/shared/utils'
import type { DocsNavSection } from './docs.types'

export const docsHref = (slug: string) => (slug === 'index' ? '/docs' : `/docs/${slug}`)

/** Marks the custom "Docs" tab in the portal header as active while a docs page is open. */
const ACTIVE_TAB_CSS = `.portal-nav a[href$="/docs"],.portal-nav a[href$="/docs/"]{background:var(--nav-active-background);color:var(--nav-active-foreground)}
.gothic-docs .prose :where(code)::before,.gothic-docs .prose :where(code)::after{content:none}
.gothic-docs .prose :where(code):not(:where(pre *)){background:var(--muted);border-radius:calc(var(--radius)*0.5);padding:.15em .4em;font-weight:500;font-size:.875em}`

function Sidebar({ nav, current, onNavigate }: { nav: DocsNavSection[]; current: string; onNavigate?: () => void }) {
  const [q, setQ] = useState('')
  const filtered = useMemo(() => {
    const s = q.trim().toLowerCase()
    if (!s) return nav
    return nav
      .map((sec) => ({ ...sec, items: sec.items.filter((i) => i.title.toLowerCase().includes(s) || sec.title.toLowerCase().includes(s)) }))
      .filter((sec) => sec.items.length > 0)
  }, [nav, q])

  return (
    <div className="flex flex-col gap-5">
      <label className="relative block">
        <MagnifyingGlassIcon className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-muted-foreground" />
        <input
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="Search docs..."
          className="h-9 w-full rounded-[calc(var(--radius)*0.8)] border border-border bg-card pl-9 pr-3 text-sm outline-none placeholder:text-muted-foreground focus:border-primary/60"
        />
      </label>
      {filtered.map((sec) => (
        <div key={sec.title}>
          <p className="mb-1.5 px-3 text-xs font-semibold uppercase tracking-wider text-muted-foreground">{sec.title}</p>
          <ul className="space-y-0.5">
            {sec.items.map((item) => {
              const active = item.slug === current
              return (
                <li key={item.slug}>
                  <Link
                    to={docsHref(item.slug) as '/'}
                    onClick={onNavigate}
                    className={cn(
                      'block rounded-[calc(var(--radius)*0.8)] px-3 py-1.5 text-sm transition-colors',
                      active
                        ? 'bg-[var(--nav-active-background)] font-medium text-primary'
                        : 'text-muted-foreground hover:bg-[var(--nav-active-background)]/50 hover:text-foreground'
                    )}
                  >
                    {item.title}
                  </Link>
                </li>
              )
            })}
          </ul>
        </div>
      ))}
      {filtered.length === 0 && <p className="px-3 text-sm text-muted-foreground">Nothing found.</p>}
    </div>
  )
}

export function GothicDocsLayout({ nav }: { nav: DocsNavSection[] }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const current = pathname.replace(/^\/docs\/?/, '').replace(/\/$/, '') || 'index'
  const [open, setOpen] = useState(false)
  const currentTitle = nav.flatMap((s) => s.items).find((i) => i.slug === current)?.title ?? 'Docs'

  return (
    <div className="gothic-docs relative">
      <style>{ACTIVE_TAB_CSS}</style>
      {/* grid + brand glow, as on the help-center pages */}
      <div className="pointer-events-none absolute inset-x-0 top-0 h-[420px] overflow-hidden" aria-hidden="true">
        <div
          className="absolute inset-0 opacity-[0.09] dark:opacity-[0.13]"
          style={{
            backgroundImage:
              'linear-gradient(to right, rgb(128 128 128) 1px, transparent 1px),linear-gradient(to bottom, rgb(128 128 128) 1px, transparent 1px)',
            backgroundSize: '52px 52px',
            maskImage: 'radial-gradient(ellipse 80% 62% at 50% 0%, #000 30%, transparent 78%)',
            WebkitMaskImage: 'radial-gradient(ellipse 80% 62% at 50% 0%, #000 30%, transparent 78%)',
          }}
        />
        <div
          className="absolute -top-32 left-1/2 h-80 w-[760px] max-w-full -translate-x-1/2 blur-2xl"
          style={{ background: 'radial-gradient(ellipse at center, color-mix(in oklch, var(--primary) 20%, transparent), transparent 70%)' }}
        />
      </div>

      <div className="relative z-10 mx-auto max-w-7xl px-4 sm:px-6">
        {/* mobile: collapsible menu */}
        <div className="lg:hidden pt-4">
          <button
            type="button"
            onClick={() => setOpen((v) => !v)}
            className="flex w-full items-center gap-2 rounded-[calc(var(--radius)*0.8)] border border-border bg-card px-3 py-2 text-sm"
          >
            <Bars3Icon className="h-4 w-4 text-muted-foreground" />
            <span className="font-medium">{currentTitle}</span>
          </button>
          {open && (
            <div className="mt-2 rounded-[var(--radius)] border border-border bg-card p-3">
              <Sidebar nav={nav} current={current} onNavigate={() => setOpen(false)} />
            </div>
          )}
        </div>

        <div className="flex gap-8 xl:gap-10">
          <aside className="sticky top-[var(--portal-header-height,3.5rem)] hidden h-[calc(100vh-var(--portal-header-height,3.5rem))] w-60 shrink-0 overflow-y-auto py-8 pr-2 lg:block">
            <Sidebar nav={nav} current={current} />
          </aside>
          <div className="min-w-0 flex-1">
            <Outlet />
          </div>
        </div>
      </div>
    </div>
  )
}
