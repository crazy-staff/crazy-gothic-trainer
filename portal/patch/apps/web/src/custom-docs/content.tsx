/**
 * Article body for the docs: renders the article's TipTap JSON with Fumadocs components.
 *
 * Plain markdown (headings, lists, tables, quotes, code, images, task lists) maps to the
 * usual elements. Extra blocks are written in markdown and become components here:
 *   > **Note:** / **Tip:** / **Warning:** / **Danger:** / **Success:**  -> callout
 *   ```trainer   {json}   -> replica of a trainer menu card (trainer-mock.tsx)
 *   ```media     screenshot: caption / video: caption / image: url | caption
 *   ```cards     [{"title","description","href","icon"}]
 *   ```steps     [{"title","body"}]            (body is markdown)
 *   ```tabs      [{"title","body"}]
 *   ```accordion [{"title","body"}]
 *   ```files     indented tree, folders end with "/"
 *   ```options   {"Name": {"type","default","description","status"}}
 *   ```keys      [{"keys":"Ctrl+K","action":"Open search"}]
 * Inline code `key:F1` draws a key, `status:OK` a status badge.
 * See docs/pages/playground/component-showcase.md in the trainer repo for every element.
 */
import type { JSONContent } from '@tiptap/core'
import type { ReactNode } from 'react'
import { Fragment, useMemo } from 'react'
import Link from 'fumadocs-core/link'
import { Heading } from 'fumadocs-ui/components/heading'
import { Callout } from 'fumadocs-ui/components/callout'
import { Card, Cards } from 'fumadocs-ui/components/card'
import { Step, Steps } from 'fumadocs-ui/components/steps'
import { Tab, Tabs } from 'fumadocs-ui/components/tabs'
import { Accordion, Accordions } from 'fumadocs-ui/components/accordion'
import { File, Files, Folder } from 'fumadocs-ui/components/files'
import { TypeTable } from 'fumadocs-ui/components/type-table'
import { DynamicCodeBlock } from 'fumadocs-ui/components/dynamic-codeblock'
import { TranslationProvider } from '@fuma-translate/react'
import ReactMarkdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import { PhotoIcon, VideoCameraIcon } from '@heroicons/react/20/solid'
import { CategoryIcon } from '@/components/help-center/category-icon'
import { RichTextContent } from '@/components/ui/rich-text-content'
import { TrainerMock, type TrainerMockSpec } from './trainer-mock'
import { BLOCK_LANGS as BLOCKS, headingIdMap, textOf } from './structure'

// ---------------------------------------------------------------- small pieces

const STATUS: Record<string, string> = {
  ok: 'bg-emerald-500/15 text-emerald-400 border-emerald-500/30',
  bug: 'bg-amber-500/15 text-amber-400 border-amber-500/30',
  crash: 'bg-red-500/15 text-red-400 border-red-500/30',
  test: 'bg-sky-500/15 text-sky-400 border-sky-500/30',
  new: 'bg-fd-primary/15 text-fd-primary border-fd-primary/30',
}

export function StatusBadge({ value }: { value: string }) {
  const cls = STATUS[value.toLowerCase()] ?? 'bg-fd-muted text-fd-muted-foreground border-fd-border'
  return (
    <span className={`inline-flex items-center rounded-md border px-1.5 py-px align-middle font-mono text-[0.7rem] font-semibold uppercase tracking-wide ${cls}`}>
      {value}
    </span>
  )
}

export function Keys({ value }: { value: string }) {
  const parts = value.split('+').map((k) => k.trim()).filter(Boolean)
  return (
    <span className="inline-flex items-center gap-1 align-middle">
      {parts.map((k, i) => (
        <Fragment key={i}>
          {i > 0 && <span className="text-xs text-fd-muted-foreground">+</span>}
          <kbd className="inline-flex min-w-6 items-center justify-center rounded-md border border-b-2 bg-fd-muted px-1.5 font-mono text-[0.75rem] font-medium leading-5 text-fd-foreground">
            {k}
          </kbd>
        </Fragment>
      ))}
    </span>
  )
}

function InlineCode({ text }: { text: string }) {
  const key = text.match(/^key:(.+)$/i)
  if (key) return <Keys value={key[1]} />
  const status = text.match(/^status:(.+)$/i)
  if (status) return <StatusBadge value={status[1]} />
  return <code className="docs-code">{text}</code>
}

function DocLink({ href, children }: { href: string; children: ReactNode }) {
  const external = /^https?:\/\//.test(href)
  return (
    <Link href={href} external={external} className="docs-link">
      {children}
    </Link>
  )
}

/** Markdown inside a block (steps, tabs, accordions, option descriptions). */
export function Md({ children }: { children: string }) {
  return (
    <div className="docs-md">
      <ReactMarkdown
        remarkPlugins={[remarkGfm]}
        components={{
          a: ({ href, children }) => <DocLink href={href ?? '#'}>{children}</DocLink>,
          code: ({ className, children }) => {
            const raw = String(children ?? '')
            if (className?.startsWith('language-'))
              return <DynamicCodeBlock lang={className.slice(9)} code={raw.replace(/\n$/, '')} />
            return <InlineCode text={raw} />
          },
          pre: ({ children }) => <>{children}</>,
        }}
      >
        {children}
      </ReactMarkdown>
    </div>
  )
}

// ---------------------------------------------------------------- custom blocks

function parse<T>(text: string): T | null {
  try {
    return JSON.parse(text) as T
  } catch {
    return null
  }
}

function BlockError({ lang }: { lang: string }) {
  return <Callout type="error" title={`Broken \`${lang}\` block`}>The JSON in this block could not be read.</Callout>
}

function Media({ text }: { text: string }) {
  return (
    <div className="not-prose my-2 space-y-4">
      {text.split('\n').filter((l) => l.trim()).map((line, i) => {
        const m = line.match(/^(screenshot|image|video|gif)\s*:\s*(.*)$/i)
        const kind = (m?.[1] ?? 'screenshot').toLowerCase()
        const rest = m?.[2] ?? line
        const [maybeUrl, caption] = rest.includes('|') ? rest.split('|').map((s) => s.trim()) : [null, rest]
        const url = maybeUrl && /^(https?:)?\//.test(maybeUrl) ? maybeUrl : null
        const Icon = kind === 'video' || kind === 'gif' ? VideoCameraIcon : PhotoIcon
        return (
          <figure key={i}>
            {url && kind === 'video' ? (
              <video src={url} controls className="w-full rounded-xl border" />
            ) : url ? (
              <img src={url} alt={caption ?? ''} className="w-full rounded-xl border" />
            ) : (
              <div className="docs-media-box">
                <Icon className="size-7" />
                <span className="text-xs font-medium uppercase tracking-wider">
                  {kind === 'gif' ? 'GIF' : kind} coming soon
                </span>
              </div>
            )}
            {caption && <figcaption className="mt-2 text-center text-sm text-fd-muted-foreground">{caption}</figcaption>}
          </figure>
        )
      })}
    </div>
  )
}

interface FileNode {
  name: string
  folder: boolean
  children: FileNode[]
}

function parseFiles(text: string): FileNode[] {
  const root: FileNode = { name: '', folder: true, children: [] }
  const stack: { indent: number; node: FileNode }[] = [{ indent: -1, node: root }]
  for (const line of text.split('\n')) {
    if (!line.trim()) continue
    const indent = line.length - line.trimStart().length
    const name = line.trim()
    const node: FileNode = { name: name.replace(/\/$/, ''), folder: name.endsWith('/'), children: [] }
    while (stack.length > 1 && stack[stack.length - 1].indent >= indent) stack.pop()
    stack[stack.length - 1].node.children.push(node)
    if (node.folder) stack.push({ indent, node })
  }
  return root.children
}

function FileTree({ nodes }: { nodes: FileNode[] }) {
  return (
    <>
      {nodes.map((n, i) =>
        n.folder ? (
          <Folder key={i} name={n.name} defaultOpen>
            <FileTree nodes={n.children} />
          </Folder>
        ) : (
          <File key={i} name={n.name} />
        )
      )}
    </>
  )
}

type Item = { title: string; body?: string; description?: string; href?: string; icon?: string }

function Block({ lang, raw }: { lang: string; raw: string }) {
  switch (lang) {
    case 'trainer': {
      const spec = parse<TrainerMockSpec>(raw)
      return spec ? <TrainerMock spec={spec} /> : <BlockError lang={lang} />
    }
    case 'media':
      return <Media text={raw} />
    case 'cards': {
      const items = parse<Item[]>(raw)
      if (!items) return <BlockError lang={lang} />
      return (
        <Cards>
          {items.map((c, i) => (
            <Card
              key={i}
              title={c.title}
              description={c.description}
              href={c.href}
              icon={c.icon ? <CategoryIcon icon={c.icon} className="size-4" /> : undefined}
            />
          ))}
        </Cards>
      )
    }
    case 'steps': {
      const items = parse<Item[]>(raw)
      if (!items) return <BlockError lang={lang} />
      return (
        <Steps>
          {items.map((s, i) => (
            <Step key={i}>
              <p className="docs-step-title">{s.title}</p>
              {s.body && <Md>{s.body}</Md>}
            </Step>
          ))}
        </Steps>
      )
    }
    case 'tabs': {
      const items = parse<Item[]>(raw)
      if (!items) return <BlockError lang={lang} />
      return (
        <Tabs items={items.map((t) => t.title)}>
          {items.map((t, i) => (
            <Tab key={i} value={t.title}>
              <Md>{t.body ?? ''}</Md>
            </Tab>
          ))}
        </Tabs>
      )
    }
    case 'accordion': {
      const items = parse<Item[]>(raw)
      if (!items) return <BlockError lang={lang} />
      return (
        <Accordions type="multiple">
          {items.map((a, i) => (
            <Accordion key={i} title={a.title}>
              <Md>{a.body ?? ''}</Md>
            </Accordion>
          ))}
        </Accordions>
      )
    }
    case 'files':
      return (
        <Files>
          <FileTree nodes={parseFiles(raw)} />
        </Files>
      )
    case 'options': {
      const spec = parse<Record<string, { type?: string; default?: string; description?: string; status?: string; required?: boolean }>>(raw)
      if (!spec) return <BlockError lang={lang} />
      const type = Object.fromEntries(
        Object.entries(spec).map(([name, o]) => [
          name,
          {
            type: o.type ?? '',
            default: o.default,
            required: o.required ?? true,
            description: (
              <span className="inline-flex flex-wrap items-center gap-2">
                {o.status && <StatusBadge value={o.status} />}
                {o.description && <Md>{o.description}</Md>}
              </span>
            ),
          },
        ])
      )
      return (
        <TranslationProvider translations={{ 'Prop(type table)': 'Option', 'Type(type table)': 'Kind' }}>
          <TypeTable type={type} />
        </TranslationProvider>
      )
    }
    case 'keys': {
      const rows = parse<{ keys: string; action: string }[]>(raw)
      if (!rows) return <BlockError lang={lang} />
      return (
        <div className="docs-table-wrap">
          <table className="docs-table">
            <tbody>
              {rows.map((r, i) => (
                <tr key={i}>
                  <td className="w-0 whitespace-nowrap">
                    {r.keys.split(/\s*\/\s*/).map((k, j) => (
                      <Fragment key={j}>
                        {j > 0 && <span className="mx-1.5 text-fd-muted-foreground">or</span>}
                        <Keys value={k} />
                      </Fragment>
                    ))}
                  </td>
                  <td>
                    <Md>{r.action}</Md>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )
    }
    default:
      return null
  }
}

// ---------------------------------------------------------------- TipTap renderer

const CALLOUT: Record<string, { type: 'info' | 'success' | 'warn' | 'error' | 'idea'; title: string }> = {
  note: { type: 'info', title: 'Note' },
  info: { type: 'info', title: 'Note' },
  tip: { type: 'idea', title: 'Tip' },
  warning: { type: 'warn', title: 'Warning' },
  danger: { type: 'error', title: 'Danger' },
  success: { type: 'success', title: 'Done' },
}

function calloutOf(node: JSONContent): { kind: (typeof CALLOUT)[string]; title: string | null; body: JSONContent[] } | null {
  if (node.type !== 'blockquote') return null
  const firstPara = node.content?.[0]
  const first = firstPara?.content?.[0]
  const m = first?.text?.match(/^(Note|Info|Tip|Warning|Danger|Success)(?:\s*[-–:]\s*(.*?))?:?\s*$/i)
  if (!m || !first?.marks?.some((k) => k.type === 'bold')) return null
  const kind = CALLOUT[m[1].toLowerCase()]
  const rest = (firstPara!.content ?? []).slice(1)
  if (rest[0]?.text) rest[0] = { ...rest[0], text: rest[0].text.replace(/^\s+/, '') }
  const body = [...(rest.length ? [{ ...firstPara!, content: rest }] : []), ...(node.content ?? []).slice(1)]
  return { kind, title: m[2]?.trim() || null, body }
}

function Marks({ node }: { node: JSONContent }) {
  let el: ReactNode = node.text ?? ''
  const marks = node.marks ?? []
  if (marks.some((m) => m.type === 'code')) return <InlineCode text={node.text ?? ''} />
  for (const m of marks) {
    if (m.type === 'bold') el = <strong>{el}</strong>
    else if (m.type === 'italic') el = <em>{el}</em>
    else if (m.type === 'strike') el = <s>{el}</s>
    else if (m.type === 'underline') el = <u>{el}</u>
  }
  const link = marks.find((m) => m.type === 'link')
  if (link) el = <DocLink href={String(link.attrs?.href ?? '#')}>{el}</DocLink>
  return <>{el}</>
}

function Nodes({ nodes, ids }: { nodes?: JSONContent[]; ids: Map<JSONContent, string> }) {
  return <>{(nodes ?? []).map((n, i) => <NodeView key={i} node={n} ids={ids} />)}</>
}

function NodeView({ node, ids }: { node: JSONContent; ids: Map<JSONContent, string> }) {
  const kids = <Nodes nodes={node.content} ids={ids} />
  switch (node.type) {
    case 'text':
      return <Marks node={node} />
    case 'hardBreak':
      return <br />
    case 'paragraph':
      return <p>{kids}</p>
    case 'heading': {
      const level = Math.min(Math.max(Number(node.attrs?.level) || 2, 1), 6)
      const as = `h${level === 1 ? 2 : level}` as 'h2'
      return (
        <Heading as={as} id={ids.get(node)}>
          {kids}
        </Heading>
      )
    }
    case 'bulletList':
      return <ul>{kids}</ul>
    case 'orderedList':
      return <ol start={Number(node.attrs?.start) || undefined}>{kids}</ol>
    case 'listItem':
      return <li>{kids}</li>
    case 'taskList':
      return <ul className="docs-tasks">{kids}</ul>
    case 'taskItem':
      return (
        <li data-checked={node.attrs?.checked ? 'true' : 'false'}>
          <span className="docs-task-box" aria-hidden>
            {node.attrs?.checked ? '✓' : ''}
          </span>
          <div>{kids}</div>
        </li>
      )
    case 'blockquote': {
      const c = calloutOf(node)
      if (c)
        return (
          <Callout type={c.kind.type} title={c.title ?? c.kind.title}>
            <Nodes nodes={c.body} ids={ids} />
          </Callout>
        )
      return <blockquote>{kids}</blockquote>
    }
    case 'codeBlock': {
      const lang = String(node.attrs?.language ?? '')
      const raw = textOf(node)
      if (BLOCKS.has(lang)) return <Block lang={lang} raw={raw} />
      return <DynamicCodeBlock lang={lang || 'text'} code={raw} />
    }
    case 'horizontalRule':
      return <hr />
    case 'image':
    case 'resizableImage':
      return (
        <figure>
          <img src={String(node.attrs?.src ?? '')} alt={String(node.attrs?.alt ?? '')} className="rounded-xl border" />
          {node.attrs?.title && <figcaption>{String(node.attrs.title)}</figcaption>}
        </figure>
      )
    case 'table':
      return (
        <div className="docs-table-wrap">
          <table className="docs-table">
            <tbody>{kids}</tbody>
          </table>
        </div>
      )
    case 'tableRow':
      return <tr>{kids}</tr>
    case 'tableHeader':
      return <th>{kids}</th>
    case 'tableCell':
      return <td>{kids}</td>
    default:
      // Anything Quackback knows and we don't (embeds, video): its own renderer.
      return <RichTextContent content={{ type: 'doc', content: [node] }} />
  }
}

export function DocsContent({ content }: { content: JSONContent }) {
  const ids = useMemo(() => headingIdMap(content), [content])
  return (
    <div className="docs-prose">
      <Nodes nodes={content.content} ids={ids} />
    </div>
  )
}
