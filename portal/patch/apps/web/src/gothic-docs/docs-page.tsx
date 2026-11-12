import { Link, useNavigate } from '@tanstack/react-router'
import type { MouseEvent } from 'react'
import { RichTextContent } from '@/components/ui/rich-text-content'
import { HelpCenterToc } from '@/components/help-center/help-center-toc'
import { extractHeadings } from '@/components/help-center/help-center-article-utils'
import { ChevronRightIcon } from '@heroicons/react/16/solid'
import { docsHref } from './docs-layout'
import type { DocsPageData } from './docs.types'

export function GothicDocsPage({ page }: { page: DocsPageData }) {
  const headings = extractHeadings(page.contentJson as Parameters<typeof extractHeadings>[0])
  const navigate = useNavigate()
  // Rendered links always open in a new tab; links inside this site stay in the tab (docs: client-side).
  const onClick = (e: MouseEvent<HTMLDivElement>) => {
    const a = (e.target as HTMLElement).closest('a')
    const href = a?.getAttribute('href')
    if (!a || !href || !href.startsWith('/') || e.metaKey || e.ctrlKey || e.shiftKey) return
    e.preventDefault()
    if (href === '/docs' || href.startsWith('/docs/')) navigate({ to: href as '/' })
    else window.location.assign(href)
  }
  return (
    <div className="flex gap-8 xl:gap-12">
      <article className="min-w-0 max-w-3xl flex-1 py-10">
        <nav className="flex items-center gap-1.5 text-sm text-muted-foreground">
          <Link to={'/docs' as '/'} className="hover:text-foreground transition-colors">Docs</Link>
          {page.section && page.slug !== 'index' && (
            <>
              <ChevronRightIcon className="h-3.5 w-3.5 shrink-0" />
              <span className="text-foreground font-medium">{page.section}</span>
            </>
          )}
        </nav>
        <h1 className="mt-6 text-3xl sm:text-4xl font-bold leading-tight tracking-tight">{page.title}</h1>
        {page.description && <p className="mt-4 text-lg text-muted-foreground leading-relaxed">{page.description}</p>}
        <div className="mt-8" onClick={onClick}>
          <RichTextContent content={page.contentJson} />
        </div>
        {(page.prev || page.next) && (
          <div className="mt-10 pt-8 border-t border-border/40 flex items-start justify-between gap-4">
            {page.prev ? (
              <Link to={docsHref(page.prev.slug) as '/'} className="group text-left">
                <span className="text-xs text-muted-foreground group-hover:text-foreground transition-colors">&larr; Previous</span>
                <p className="text-sm font-medium text-foreground group-hover:text-primary transition-colors mt-0.5">{page.prev.title}</p>
              </Link>
            ) : (
              <div />
            )}
            {page.next ? (
              <Link to={docsHref(page.next.slug) as '/'} className="group text-right">
                <span className="text-xs text-muted-foreground group-hover:text-foreground transition-colors">Next &rarr;</span>
                <p className="text-sm font-medium text-foreground group-hover:text-primary transition-colors mt-0.5">{page.next.title}</p>
              </Link>
            ) : (
              <div />
            )}
          </div>
        )}
        {page.editUrl && (
          <a href={page.editUrl} target="_blank" rel="noopener noreferrer" className="mt-6 inline-block text-xs text-muted-foreground hover:text-foreground">
            Edit this page on GitHub
          </a>
        )}
      </article>
      <HelpCenterToc headings={headings} />
    </div>
  )
}
