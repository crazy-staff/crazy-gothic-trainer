/**
 * Layout variants of the docs, switchable from a picker in the corner while we choose one:
 *   a  Spacious (fumadocs.dev), the portal header above it in the same colours
 *   b  Spacious (fumadocs.dev), the portal header hidden, its links in the page bar
 *   c  Classic Fumadocs docs layout under the portal header
 * The choice is kept per browser (localStorage) and can be forced with ?layout=a|b|c.
 */
import { createContext, use, useEffect, useState, type ReactNode } from 'react'

export type DocsVariant = 'a' | 'b' | 'c'
export const VARIANTS: { id: DocsVariant; name: string; description: string }[] = [
  { id: 'a', name: 'Spacious', description: 'fumadocs.dev layout, portal header on top' },
  { id: 'b', name: 'Spacious, one bar', description: 'portal links inside the page bar' },
  { id: 'c', name: 'Classic', description: 'Fumadocs docs layout under the header' },
]
const KEY = 'custom-docs-layout'

const Ctx = createContext<{ variant: DocsVariant; setVariant: (v: DocsVariant) => void }>({
  variant: 'a',
  setVariant: () => {},
})

export function DocsVariantProvider({ children }: { children: ReactNode }) {
  const [variant, setState] = useState<DocsVariant>('a')
  useEffect(() => {
    try {
      const q = new URLSearchParams(window.location.search).get('layout')
      const v = (q ?? localStorage.getItem(KEY)) as DocsVariant | null
      if (v && VARIANTS.some((x) => x.id === v)) setState(v)
    } catch {
      // storage blocked: stay on the default
    }
  }, [])
  const setVariant = (v: DocsVariant) => {
    setState(v)
    try {
      localStorage.setItem(KEY, v)
    } catch {
      // ignore
    }
  }
  return <Ctx value={{ variant, setVariant }}>{children}</Ctx>
}

export const useDocsVariant = () => use(Ctx)

export function VariantPicker() {
  const { variant, setVariant } = useDocsVariant()
  const [open, setOpen] = useState(false)
  const current = VARIANTS.find((v) => v.id === variant)!
  return (
    <div className="fixed bottom-4 right-4 z-[60] text-sm">
      {open && (
        <div className="mb-2 flex w-64 flex-col gap-1 rounded-2xl border bg-fd-popover p-1 text-fd-popover-foreground shadow-lg">
          {VARIANTS.map((v) => (
            <button
              key={v.id}
              type="button"
              onClick={() => {
                setVariant(v.id)
                setOpen(false)
              }}
              className={
                'rounded-lg px-2.5 py-1.5 text-start transition-colors ' +
                (v.id === variant ? 'bg-fd-primary/10 text-fd-primary' : 'hover:bg-fd-accent hover:text-fd-accent-foreground')
              }
            >
              <p className="font-medium">
                {v.id.toUpperCase()} · {v.name}
              </p>
              <p className="text-xs opacity-70">{v.description}</p>
            </button>
          ))}
        </div>
      )}
      <button
        type="button"
        onClick={() => setOpen((o) => !o)}
        className="ms-auto flex items-center gap-2 rounded-full border bg-fd-popover px-3 py-1.5 text-fd-popover-foreground shadow-lg transition-colors hover:bg-fd-accent"
      >
        <span className="text-fd-muted-foreground">Layout</span>
        <span className="font-medium">
          {current.id.toUpperCase()} · {current.name}
        </span>
      </button>
    </div>
  )
}
