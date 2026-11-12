/**
 * Pure helpers over an article's TipTap JSON, shared by the page (anchors, table of
 * contents) and the search index (structured data). Heading ids come from Quackback's own
 * heading-ids module, so links made anywhere else in Quackback keep working.
 */
import type { JSONContent } from '@tiptap/core'
import { collectHeadings, createHeadingIds, plainText } from '@/lib/shared/heading-ids'

/** Code block languages that are components, not code. */
export const BLOCK_LANGS = new Set([
  'trainer', 'media', 'cards', 'steps', 'tabs', 'accordion', 'files', 'options', 'keys',
])

export const textOf = (n: JSONContent | undefined): string =>
  !n ? '' : (n.text ?? '') + (n.content ?? []).map(textOf).join('')

export function headingIdMap(doc: JSONContent): Map<JSONContent, string> {
  return createHeadingIds(collectHeadings(doc as never)) as unknown as Map<JSONContent, string>
}

export interface TocEntry {
  title: string
  url: string
  depth: number
}

export function tocOf(doc: JSONContent | null | undefined): TocEntry[] {
  if (!doc) return []
  const ids = headingIdMap(doc)
  const out: TocEntry[] = []
  for (const node of collectHeadings(doc as never) as JSONContent[]) {
    const level = Number(node.attrs?.level)
    const id = ids.get(node)
    if (id && level >= 2 && level <= 4) out.push({ title: plainText(node as never).trim(), url: `#${id}`, depth: level })
  }
  return out
}

/** Text a custom block contributes to search (JSON blocks: every string value). */
function blockText(lang: string, raw: string): string {
  if (lang === 'media') return raw
  try {
    const strings: string[] = []
    const walk = (v: unknown) => {
      if (typeof v === 'string') strings.push(v)
      else if (Array.isArray(v)) v.forEach(walk)
      else if (v && typeof v === 'object') Object.values(v).forEach(walk)
    }
    walk(JSON.parse(raw))
    return strings.join(' ').replace(/[`*_#>\[\]()]/g, ' ').replace(/\s+/g, ' ').trim()
  } catch {
    return raw
  }
}

export interface StructuredData {
  headings: { id: string; content: string }[]
  contents: { heading: string | undefined; content: string }[]
}

export function structuredDataOf(doc: JSONContent | null | undefined): StructuredData {
  const data: StructuredData = { headings: [], contents: [] }
  if (!doc) return data
  const ids = headingIdMap(doc)
  let current: string | undefined
  const push = (content: string) => {
    const c = content.replace(/\s+/g, ' ').trim()
    if (c) data.contents.push({ heading: current, content: c })
  }
  const visit = (node: JSONContent) => {
    switch (node.type) {
      case 'heading': {
        const id = ids.get(node)
        if (id) {
          data.headings.push({ id, content: textOf(node).trim() })
          current = id
        }
        return
      }
      case 'codeBlock': {
        const lang = String(node.attrs?.language ?? '')
        push(BLOCK_LANGS.has(lang) ? blockText(lang, textOf(node)) : textOf(node))
        return
      }
      case 'paragraph':
      case 'tableRow':
        push(node.type === 'tableRow' ? (node.content ?? []).map(textOf).join(' | ') : textOf(node))
        return
      default:
        for (const child of node.content ?? []) visit(child)
    }
  }
  visit(doc)
  return data
}
