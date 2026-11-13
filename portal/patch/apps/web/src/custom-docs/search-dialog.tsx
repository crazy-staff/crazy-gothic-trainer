/**
 * The search dialog of fumadocs.dev (apps/docs/components/layouts/search.tsx, MIT), adapted:
 * a "Filter" menu in the footer (one entry per docs category, with its description) and a
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
}

export interface DocsSearchPage {
  name: string
  url: string
}

export function createDocsSearchDialog(filters: DocsSearchFilter[], pages: DocsSearchPage[]) {
  const items: DocsSearchFilter[] = [{ name: 'All', value: undefined }, ...filters]
  return function DocsSearchDialog(props: SharedProps) {
    const [open, setOpen] = useState(false)
    const [tag, setTag] = useState<string | undefined>()
    const search = useFetchSearch({ api: '/api/docs-search', tag })
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
          <SearchDialogFooter className="flex flex-row flex-wrap items-center gap-2">
            <Popover open={open} onOpenChange={setOpen}>
              <PopoverTrigger className={buttonVariants({ size: 'sm', variant: 'ghost', className: '-m-1.5 me-auto' })}>
                <span className="me-2 text-fd-muted-foreground/80">Filter</span>
                {items.find((item) => item.value === tag)?.name}
                <ChevronDown className="size-3.5 text-fd-muted-foreground" />
              </PopoverTrigger>
              <PopoverContent className="flex max-h-80 flex-col gap-1 overflow-y-auto p-1" align="start">
                {items.map((item, i) => {
                  const selected = item.value === tag
                  return (
                    <button
                      key={i}
                      type="button"
                      onClick={() => {
                        setTag(item.value)
                        setOpen(false)
                      }}
                      className={
                        'rounded-lg px-2 py-1.5 text-start ' +
                        (selected
                          ? 'bg-fd-primary/10 text-fd-primary'
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
          </SearchDialogFooter>
        </SearchDialogContent>
      </SearchDialog>
    )
  }
}
