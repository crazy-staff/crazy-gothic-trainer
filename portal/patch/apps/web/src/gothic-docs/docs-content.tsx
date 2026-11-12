import type { JSONContent } from '@tiptap/core'
import type { MouseEvent, ReactNode } from 'react'
import { useNavigate } from '@tanstack/react-router'
import { RichTextContent } from '@/components/ui/rich-text-content'
import { InformationCircleIcon, ExclamationTriangleIcon, LightBulbIcon, PhotoIcon, VideoCameraIcon, ArrowRightIcon } from '@heroicons/react/20/solid'
import { TrainerMock, type TrainerMockSpec } from './trainer-mock'

/**
 * Article body for the docs. Plain content renders exactly like a help-center article; a few
 * extra blocks are written in markdown and turned into components here:
 *   > **Note:** / **Warning:** / **Tip:** ...      -> callout
 *   ```trainer  {json}                             -> replica of a trainer menu card (see trainer-mock.tsx)
 *   ```media    screenshot: caption / video: caption -> placeholder until the real media is added
 *   ```cards    [{"title","description","href"}]    -> link cards
 */
const textOf = (n: JSONContent): string => (n.text ?? '') + (n.content ?? []).map(textOf).join('')

const CALLOUT = {
  note: { icon: InformationCircleIcon, cls: 'gd-callout--note', label: 'Note' },
  tip: { icon: LightBulbIcon, cls: 'gd-callout--tip', label: 'Tip' },
  warning: { icon: ExclamationTriangleIcon, cls: 'gd-callout--warn', label: 'Warning' },
} as const

function calloutKind(node: JSONContent): keyof typeof CALLOUT | null {
  if (node.type !== 'blockquote') return null
  const first = node.content?.[0]?.content?.[0]
  const m = first?.text?.match(/^(Note|Tip|Warning):?\s*$/i)
  if (!m || !first?.marks?.some((k) => k.type === 'bold')) return null
  return m[1].toLowerCase() as keyof typeof CALLOUT
}

function stripCalloutLabel(node: JSONContent): JSONContent {
  const content = (node.content ?? []).map((p, i) => (i === 0 ? { ...p, content: (p.content ?? []).slice(1) } : p))
  if (content[0]?.content?.[0]?.text) content[0].content[0] = { ...content[0].content[0], text: content[0].content[0].text!.replace(/^\s+/, '') }
  return { type: 'doc', content }
}

function Media({ text }: { text: string }) {
  return (
    <div className="not-prose my-6 space-y-3">
      {text.split('\n').filter(Boolean).map((line, i) => {
        const m = line.match(/^(screenshot|image|video|gif)\s*:\s*(.*)$/i)
        const kind = (m?.[1] ?? 'screenshot').toLowerCase()
        const caption = m?.[2] ?? line
        const Icon = kind === 'video' || kind === 'gif' ? VideoCameraIcon : PhotoIcon
        return (
          <figure key={i} className="gd-media">
            <div className="gd-media-box">
              <Icon className="h-7 w-7" />
              <span className="text-xs font-medium uppercase tracking-wider">{kind === 'gif' ? 'GIF' : kind} coming soon</span>
            </div>
            <figcaption className="mt-2 text-center text-sm text-muted-foreground">{caption}</figcaption>
          </figure>
        )
      })}
    </div>
  )
}

function Cards({ items }: { items: { title: string; description?: string; href: string }[] }) {
  return (
    <div className="not-prose my-6 grid gap-3 sm:grid-cols-2">
      {items.map((c) => (
        <a key={c.href + c.title} href={c.href} className="gd-card group">
          <span className="flex items-center justify-between gap-2 font-semibold text-foreground">
            {c.title}
            <ArrowRightIcon className="h-4 w-4 text-muted-foreground transition-transform group-hover:translate-x-0.5 group-hover:text-primary" />
          </span>
          {c.description && <span className="mt-1 block text-sm text-muted-foreground">{c.description}</span>}
        </a>
      ))}
    </div>
  )
}

function parse<T>(text: string): T | null {
  try {
    return JSON.parse(text) as T
  } catch {
    return null
  }
}

export function GothicDocsContent({ content }: { content: JSONContent }) {
  const navigate = useNavigate()
  const out: ReactNode[] = []
  let run: JSONContent[] = []
  const flush = () => {
    if (run.length) out.push(<RichTextContent key={`r${out.length}`} content={{ type: 'doc', content: run }} />)
    run = []
  }
  for (const node of content.content ?? []) {
    const lang = node.type === 'codeBlock' ? String(node.attrs?.language ?? '') : ''
    const kind = calloutKind(node)
    if (lang === 'trainer') {
      const spec = parse<TrainerMockSpec>(textOf(node))
      flush()
      out.push(spec ? <TrainerMock key={`t${out.length}`} spec={spec} /> : null)
    } else if (lang === 'media') {
      flush()
      out.push(<Media key={`m${out.length}`} text={textOf(node)} />)
    } else if (lang === 'cards') {
      const items = parse<{ title: string; description?: string; href: string }[]>(textOf(node))
      flush()
      out.push(items ? <Cards key={`c${out.length}`} items={items} /> : null)
    } else if (kind) {
      flush()
      const c = CALLOUT[kind]
      out.push(
        <div key={`q${out.length}`} className={`gd-callout ${c.cls} not-prose`}>
          <c.icon className="gd-callout-icon h-5 w-5 shrink-0" />
          <div className="min-w-0">
            <div className="gd-callout-title">{c.label}</div>
            <RichTextContent content={stripCalloutLabel(node)} className="gd-callout-body" />
          </div>
        </div>
      )
    } else run.push(node)
  }
  flush()

  // Rendered links open in a new tab; links to this site stay in the tab (docs pages client-side).
  const onClick = (e: MouseEvent<HTMLDivElement>) => {
    const a = (e.target as HTMLElement).closest('a')
    const href = a?.getAttribute('href')
    if (!a || !href || !href.startsWith('/') || e.metaKey || e.ctrlKey || e.shiftKey || e.button !== 0) return
    e.preventDefault()
    if (href.startsWith('/hc')) navigate({ to: href as '/hc' })
    else window.location.assign(href)
  }
  return (
    <div className="gd-content" onClick={onClick}>
      {out}
    </div>
  )
}
