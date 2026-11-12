export interface DocsNavArticle {
  id: string
  title: string
  slug: string
  urlId: number
  description: string | null
}

export interface DocsNavCategory {
  id: string
  name: string
  slug: string
  urlId: number
  icon: string | null
  description: string | null
  parentId: string | null
  articles: DocsNavArticle[]
}
