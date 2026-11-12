import { useMemo, useState, type ReactNode } from 'react'
import { Link, useRouterState } from '@tanstack/react-router'
import { MagnifyingGlassIcon, Bars3Icon, ChevronDownIcon } from '@heroicons/react/20/solid'
import { cn } from '@/lib/shared/utils'
import { hcArticlePath, hcCollectionPath } from '@/lib/shared/help-center-url'
import type { DocsNavCategory } from './docs-types'
import { DOCS_CSS } from './docs-style'

function Sidebar({ nav, locale, onNavigate }: { nav: DocsNavCategory[]; locale: string; onNavigate?: () => void }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const [q, setQ] = useState('')
  const filtered = useMemo(() => {
    const s = q.trim().toLowerCase()
    if (!s) return nav
    return nav
      .map((c) => ({
        ...c,
        articles: c.name.toLowerCase().includes(s) ? c.articles : c.articles.filter((a) => a.title.toLowerCase().includes(s)),
      }))
      .filter((c) => c.articles.length > 0)
  }, [nav, q])

  return (
    <div className="flex flex-col gap-6">
      <label className="relative block">
        <MagnifyingGlassIcon className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-muted-foreground" />
        <input
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="Filter docs..."
          className="h-9 w-full rounded-[calc(var(--radius)*0.8)] border border-border bg-card/60 pl-9 pr-3 text-sm outline-none placeholder:text-muted-foreground focus:border-primary/60"
        />
      </label>
      {filtered.map((cat) => {
        const catHref = hcCollectionPath({ locale, urlId: cat.urlId, slug: cat.slug })
        return (
          <div key={cat.id}>
            <Link
              to={catHref as '/hc'}
              onClick={onNavigate}
              className="mb-1.5 block px-3 text-[13px] font-semibold text-foreground hover:text-primary"
            >
              {cat.name}
            </Link>
            <ul className="space-y-px border-l border-border/70 ml-3">
              {cat.articles.map((a) => {
                const href = hcArticlePath({ locale, urlId: a.urlId, slug: a.slug })
                const active = pathname === href || pathname.startsWith(href + '/')
                return (
                  <li key={a.urlId}>
                    <Link
                      to={href as '/hc'}
                      onClick={onNavigate}
                      className={cn(
                        '-ml-px block border-l py-1.5 pl-3 pr-2 text-sm transition-colors',
                        active
                          ? 'border-primary font-medium text-primary'
                          : 'border-transparent text-muted-foreground hover:border-border hover:text-foreground'
                      )}
                    >
                      {a.title}
                    </Link>
                  </li>
                )
              })}
            </ul>
          </div>
        )
      })}
      {filtered.length === 0 && <p className="px-3 text-sm text-muted-foreground">Nothing found.</p>}
    </div>
  )
}

/** Docs frame around every help-center page: sidebar on the left, the page on the right. */
export function GothicDocsShell({ nav, locale, children }: { nav: DocsNavCategory[]; locale: string; children: ReactNode }) {
  const [open, setOpen] = useState(false)
  return (
    <div className="gothic-docs relative">
      <style>{DOCS_CSS}</style>
      <div className="mx-auto flex max-w-[88rem] gap-6 px-4 sm:px-6 xl:gap-8">
        <aside className="gothic-docs-sidebar sticky top-[var(--portal-header-height,3.5rem)] hidden h-[calc(100vh-var(--portal-header-height,3.5rem))] w-64 shrink-0 overflow-y-auto py-8 pr-3 lg:block">
          <Sidebar nav={nav} locale={locale} />
        </aside>
        <div className="min-w-0 flex-1">
          <div className="pt-4 lg:hidden">
            <button
              type="button"
              onClick={() => setOpen((v) => !v)}
              className="flex w-full items-center gap-2 rounded-[calc(var(--radius)*0.8)] border border-border bg-card px-3 py-2 text-sm"
            >
              <Bars3Icon className="h-4 w-4 text-muted-foreground" />
              <span className="font-medium">Menu</span>
              <ChevronDownIcon className={cn('ml-auto h-4 w-4 text-muted-foreground transition-transform', open && 'rotate-180')} />
            </button>
            {open && (
              <div className="mt-2 rounded-[var(--radius)] border border-border bg-card p-3">
                <Sidebar nav={nav} locale={locale} onNavigate={() => setOpen(false)} />
              </div>
            )}
          </div>
          {children}
        </div>
      </div>
    </div>
  )
}
