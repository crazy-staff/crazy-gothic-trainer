import type { JSONContent } from '@tiptap/core'

export interface DocsNavSection {
  title: string
  items: { slug: string; title: string }[]
}

export interface DocsPageData {
  slug: string
  title: string
  description: string | null
  section: string | null
  contentJson: JSONContent
  prev: { slug: string; title: string } | null
  next: { slug: string; title: string } | null
  editUrl: string | null
}
