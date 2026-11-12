/**
 * Docs pages (home, category, article) on Fumadocs' page layout. They replace the help
 * center's own page components; the routes and their loaders stay Quackback's.
 */
import type { JSONContent } from '@tiptap/core'
import { getRouteApi } from '@tanstack/react-router'
import { DocsDescription, DocsPage, DocsTitle, PageLastUpdate } from 'fumadocs-ui/layouts/docs/page'
import { Card, Cards } from 'fumadocs-ui/components/card'
import { ArrowRightIcon } from '@heroicons/react/20/solid'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { HelpCenterArticleFeedback } from '@/components/help-center/help-center-article-feedback'
import { EmbedHydration } from '@/components/shared/embed-hydration'
import { isRichTextContent } from '@/components/ui/rich-text-content'
import { hcArticlePath, hcCollectionPath } from '@/lib/shared/help-center-url'
import { DEFAULT_HELP_CENTER_CONFIG, type HelpCenterConfig } from '@/lib/shared/types/settings'
import { useWorkspaceSettings } from '@/lib/client/hooks/use-root-context'
import { isPortalSupportSurfaceEnabled } from '@/lib/shared/support-surfaces'
import { DocsContent } from './content'
import { tocOf } from './structure'
import type { DocsNavCategory } from './types'

const layoutApi = getRouteApi('/_portal/hc')
const articleApi = getRouteApi('/_portal/hc/$locale/articles/$idSlug/')
const collectionApi = getRouteApi('/_portal/hc/$locale/collections/$idSlug')

function useDocsNav(): { nav: DocsNavCategory[]; config: HelpCenterConfig | null } {
  const data = layoutApi.useLoaderData() as { docsNav?: DocsNavCategory[]; helpCenterConfig: HelpCenterConfig | null }
  return { nav: data.docsNav ?? [], config: data.helpCenterConfig }
}

function CategoryCards({ nav, locale }: { nav: DocsNavCategory[]; locale: string }) {
  return (
    <Cards className="sm:grid-cols-2 lg:grid-cols-3">
      {nav.map((c) => {
        const first = c.articles[0]
        return (
          <Card
            key={c.id}
            icon={c.icon ? <CategoryIcon icon={c.icon} className="size-4" /> : undefined}
            title={c.name}
            href={first ? hcArticlePath({ locale, urlId: first.urlId, slug: first.slug }) : hcCollectionPath({ locale, urlId: c.urlId, slug: c.slug })}
            description={
              <>
                {c.description && <span className="block">{c.description}</span>}
                <span className="mt-2 block text-xs text-fd-muted-foreground/80">
                  {c.articles.length} {c.articles.length === 1 ? 'page' : 'pages'}
                </span>
              </>
            }
          />
        )
      })}
    </Cards>
  )
}

export function DocsHomePage() {
  const { nav, config } = useDocsNav()
  const locale = config?.locales?.default ?? 'en'
  const title =
    config?.homepageTitle && config.homepageTitle !== DEFAULT_HELP_CENTER_CONFIG.homepageTitle
      ? config.homepageTitle
      : 'Documentation'
  const description =
    config?.homepageDescription && config.homepageDescription !== DEFAULT_HELP_CENTER_CONFIG.homepageDescription
      ? config.homepageDescription
      : 'Everything about the project, page by page.'
  const start = nav[0]?.articles[0]
  return (
    <DocsPage full tableOfContent={{ enabled: false }} breadcrumb={{ enabled: false }} footer={{ enabled: false }}>
      <div className="docs-home-hero not-prose">
        <p className="text-sm font-medium text-fd-primary">Docs</p>
        <h1 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">{title}</h1>
        <p className="mt-3 max-w-2xl text-lg text-fd-muted-foreground">{description}</p>
        {start && (
          <a
            href={hcArticlePath({ locale, urlId: start.urlId, slug: start.slug })}
            className="mt-6 inline-flex items-center gap-2 rounded-lg bg-fd-primary px-4 py-2 text-sm font-medium text-fd-primary-foreground transition-opacity hover:opacity-90"
          >
            Start with {start.title}
            <ArrowRightIcon className="size-4" />
          </a>
        )}
      </div>
      <h2 className="mt-10 mb-4 text-lg font-semibold">Browse by topic</h2>
      <CategoryCards nav={nav} locale={locale} />
    </DocsPage>
  )
}

export function DocsCategoryPage() {
  const { nav } = useDocsNav()
  const { locale, idSlug } = collectionApi.useParams()
  const urlId = Number(String(idSlug).split('-')[0])
  const category = nav.find((c) => c.urlId === urlId)
  if (!category) return <DocsHomePage />
  return (
    <DocsPage tableOfContent={{ enabled: false }} footer={{ enabled: false }}>
      <DocsTitle>{category.name}</DocsTitle>
      {category.description && <DocsDescription>{category.description}</DocsDescription>}
      <Cards>
        {category.articles.map((a) => (
          <Card
            key={a.id}
            title={a.title}
            description={a.description ?? undefined}
            href={hcArticlePath({ locale, urlId: a.urlId, slug: a.slug })}
          />
        ))}
      </Cards>
    </DocsPage>
  )
}

export function DocsArticlePage() {
  const { article } = articleApi.useLoaderData() as {
    article: {
      id: string
      title: string
      description?: string | null
      content?: string | null
      contentJson?: unknown
      updatedAt?: string | Date | null
    }
  }
  const settings = useWorkspaceSettings()
  const supportEnabled = isPortalSupportSurfaceEnabled(settings?.featureFlags, settings?.portalConfig)
  const json = article.contentJson && isRichTextContent(article.contentJson) ? (article.contentJson as JSONContent) : null
  const toc = tocOf(json)
  return (
    <DocsPage toc={toc} tableOfContent={{ style: 'clerk' }} tableOfContentPopover={{ style: 'clerk' }}>
      <DocsTitle>{article.title}</DocsTitle>
      {article.description && <DocsDescription>{article.description}</DocsDescription>}
      <div className="docs-body flex-1">
        {json ? (
          <EmbedHydration>
            <DocsContent content={json} />
          </EmbedHydration>
        ) : (
          <p className="whitespace-pre-wrap">{article.content}</p>
        )}
      </div>
      <div className="docs-feedback not-prose">
        <HelpCenterArticleFeedback articleId={article.id as never} supportHref={supportEnabled ? '/support/new' : null} />
      </div>
      {article.updatedAt && <PageLastUpdate date={new Date(article.updatedAt)} />}
    </DocsPage>
  )
}
