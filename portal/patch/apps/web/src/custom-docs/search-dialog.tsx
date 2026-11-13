/**
 * The search dialog of fumadocs.dev (apps/docs/components/layouts/search.tsx, MIT), adapted:
 * several filter menus in the footer (category, results, game, renderer) and a
 * "Jump to <page>" action when the query starts a page title.
 */
import { useMemo, useState } from 'react'
import { ArrowRight, ChevronDown } from 'lucide-react'
import { useRouter } from '@tanstack/react-router'
import {
  SearchDialog,
  SearchDialogClose,
  SearchDialogContent,
  SearchDialogFooter,
  SearchDialogHeader,
  SearchDialogIcon,
  SearchDialogInput,
  SearchDialogList,
  SearchDialogOverlay,
  type SearchItemType,
  type SharedProps,
} from '@fumadocs/base-ui/components/dialog/search'
import { Popover, PopoverContent, PopoverTrigger } from '@fumadocs/base-ui/components/ui/popover'
import { buttonVariants } from '@fumadocs/base-ui/components/ui/button'
import { useFetchSearch } from 'fumadocs-core/search/client'

export interface DocsSearchFilter {
  name: string
  description?: string
  value: string | undefined
  disabled?: boolean
}

export interface DocsSearchPage {
  name: string
  url: string
}

type GroupId = 'category' | 'kind' | 'game' | 'renderer'
interface FilterGroup {
  id: GroupId
  label: string
  options: DocsSearchFilter[]
}

const KIND: DocsSearchFilter[] = [
  { name: 'Everything', description: 'Pages, headings and text', value: undefined },
  { name: 'Pages', description: 'Only page titles', value: 'pages' },
  { name: 'Headings', description: 'Page titles and section headings', value: 'headings' },
]
const GAME: DocsSearchFilter[] = [
  { name: 'Any', value: undefined },
  { name: 'Gothic 1', description: 'Gothic 1 Crazy Trainer', value: 'game:gothic-1' },
  { name: 'Gothic 2', description: 'Coming later', value: 'game:gothic-2', disabled: true },
]
const RENDERER: DocsSearchFilter[] = [
  { name: 'Any', value: undefined },
  { name: 'GD3D11', description: 'DirectX 11 renderer', value: 'renderer:gd3d11' },
  { name: 'Original', description: 'Without GD3D11 · coming later', value: 'renderer:original', disabled: true },
]
const NONE: Record<GroupId, string | undefined> = { category: undefined, kind: undefined, game: undefined, renderer: undefined }

function FilterMenu({
  group,
  value,
  onChange,
}: {
  group: FilterGroup
  value: string | undefined
  onChange: (v: string | undefined) => void
}) {
  const [open, setOpen] = useState(false)
  const current = group.options.find((o) => o.value === value) ?? group.options[0]
  return (
    <Popover open={open} onOpenChange={setOpen}>
      <PopoverTrigger className={buttonVariants({ size: 'sm', variant: 'ghost', className: '-my-1.5' })}>
        <span className="me-2 text-fd-muted-foreground/80">{group.label}</span>
        {current.name}
        <ChevronDown className="size-3.5 text-fd-muted-foreground" />
      </PopoverTrigger>
      <PopoverContent className="flex max-h-80 flex-col gap-1 overflow-y-auto p-1" align="start">
        {group.options.map((item, i) => {
          const selected = item.value === value
          return (
            <button
              key={i}
              type="button"
              disabled={item.disabled}
              onClick={() => {
                onChange(item.value)
                setOpen(false)
              }}
              className={
                'rounded-lg px-2 py-1.5 text-start ' +
                (selected
                  ? 'bg-fd-primary/10 text-fd-primary'
                  : item.disabled
                    ? 'cursor-not-allowed opacity-50'
                    : 'hover:bg-fd-accent hover:text-fd-accent-foreground')
              }
            >
              <p className="mb-0.5 font-medium">{item.name}</p>
              {item.description && <p className="text-xs opacity-70">{item.description}</p>}
            </button>
          )
        })}
      </PopoverContent>
    </Popover>
  )
}

export function createDocsSearchDialog(categories: DocsSearchFilter[], pages: DocsSearchPage[]) {
  const groups: FilterGroup[] = [
    { id: 'category', label: 'Filter', options: [{ name: 'All', value: undefined }, ...categories] },
    { id: 'kind', label: 'Results', options: KIND },
    { id: 'game', label: 'Game', options: GAME },
    { id: 'renderer', label: 'Renderer', options: RENDERER },
  ]
  return function DocsSearchDialog(props: SharedProps) {
    const [filters, setFilters] = useState(NONE)
    const tag = [filters.category, filters.game, filters.renderer].filter(Boolean).join(',') || undefined
    const api = '/api/docs-search' + (filters.kind ? `?kind=${filters.kind}` : '')
    const search = useFetchSearch({ api, tag })
    const router = useRouter()

    const jump = useMemo<SearchItemType | undefined>(() => {
      const q = search.search.trim().toLowerCase()
      if (!q) return
      const page = pages.find((p) => p.name.toLowerCase().startsWith(q))
      if (!page) return
      return {
        id: 'quick-action',
        type: 'action',
        node: (
          <div className="inline-flex items-center gap-2 text-fd-muted-foreground">
            <ArrowRight className="size-4" />
            <p>
              Jump to <span className="font-medium text-fd-foreground">{page.name}</span>
            </p>
          </div>
        ),
        onSelect: () => void router.navigate({ href: page.url }),
      }
    }, [search.search, router])

    return (
      <SearchDialog {...search} {...props}>
        <SearchDialogOverlay />
        <SearchDialogContent>
          <SearchDialogHeader>
            <SearchDialogIcon />
            <SearchDialogInput />
            <SearchDialogClose />
          </SearchDialogHeader>
          <SearchDialogList items={jump ? [jump, ...(search.data?.items ?? [])] : search.data?.items} />
          <SearchDialogFooter className="flex flex-row flex-wrap items-center gap-x-1 gap-y-2">
            {groups.map((g) => (
              <FilterMenu
                key={g.id}
                group={g}
                value={filters[g.id]}
                onChange={(v) => setFilters((f) => ({ ...f, [g.id]: v }))}
              />
            ))}
            {Object.values(filters).some(Boolean) && (
              <button
                type="button"
                onClick={() => setFilters(NONE)}
                className="ms-auto text-xs text-fd-muted-foreground hover:text-fd-accent-foreground"
              >
                Clear filters
              </button>
            )}
          </SearchDialogFooter>
        </SearchDialogContent>
      </SearchDialog>
    )
  }
}
