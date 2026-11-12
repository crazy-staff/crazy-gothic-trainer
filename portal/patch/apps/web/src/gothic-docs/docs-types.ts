export interface DocsNavArticle {
  title: string
  slug: string
  urlId: number
}

export interface DocsNavCategory {
  id: string
  name: string
  slug: string
  urlId: number
  parentId: string | null
  articles: DocsNavArticle[]
}
