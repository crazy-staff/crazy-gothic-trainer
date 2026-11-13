import { Bars3Icon, Squares2X2Icon } from '@heroicons/react/20/solid'
import { SearchInput } from '@/components/shared/search-input'
import { SortMenu } from '@/components/shared/sort-menu'
import { cn } from '@/lib/shared/utils'

interface SortOption {
  value: string
  label: string
}

export type ListView = 'list' | 'grid'

interface AdminListHeaderProps {
  searchValue: string
  onSearchChange: (value: string) => void
  searchPlaceholder?: string
  sortOptions?: SortOption[]
  activeSort?: string
  onSortChange?: (value: string) => void
  /** The sort trigger's text for the active option, for translated copy. */
  formatSortLabel?: (label: string) => React.ReactNode
  /** Filter controls placed after the sort menu */
  filters?: React.ReactNode
  /** Slot for the primary action (e.g., NewButton), on the right */
  action?: React.ReactNode
  /** List or grid, when the list offers both. */
  view?: { value: ListView; onChange: (view: ListView) => void }
  /** Additional rows below the search bar (e.g., active filters bar) */
  children?: React.ReactNode
}

const VIEWS = [
  { value: 'list', label: 'List', icon: Bars3Icon },
  { value: 'grid', label: 'Grid', icon: Squares2X2Icon },
] as const

/**
 * The toolbar above every admin list: one card holding search, sort, filters, the view and the
 * main action, with the active filters as a second row inside it - nothing floats on its own.
 */
export function AdminListHeader({
  searchValue,
  onSearchChange,
  searchPlaceholder = 'Search...',
  sortOptions,
  activeSort,
  onSortChange,
  formatSortLabel,
  filters,
  action,
  view,
  children,
}: AdminListHeaderProps) {
  return (
    <div data-admin-toolbar="" className="sticky top-0 z-10 bg-background/90 pt-6 pb-3 backdrop-blur-md">
      <div className="rounded-xl border bg-card">
        <div className="flex flex-wrap items-center gap-1.5 p-1.5">
          <div data-slot="admin-list-search" className="flex min-w-[180px] max-w-[420px] flex-1">
            <SearchInput value={searchValue} onChange={onSearchChange} placeholder={searchPlaceholder} data-search-input />
          </div>
          {sortOptions && sortOptions.length > 0 && onSortChange && (
            <SortMenu
              options={sortOptions}
              value={activeSort ?? sortOptions[0].value}
              onChange={onSortChange}
              formatLabel={formatSortLabel}
            />
          )}
          {filters}
          {(view || action) && (
            <div className="ms-auto flex items-center gap-1.5">
              {view && (
                <div role="radiogroup" aria-label="View" className="flex items-center gap-0.5 rounded-lg border p-0.5">
                  {VIEWS.map((v) => {
                    const on = view.value === v.value
                    return (
                      <button
                        key={v.value}
                        type="button"
                        role="radio"
                        aria-checked={on}
                        aria-label={v.label}
                        title={v.label}
                        onClick={() => view.onChange(v.value)}
                        className={cn(
                          'flex size-7 items-center justify-center rounded-md transition-colors [&_svg]:size-4',
                          on ? 'bg-accent text-foreground' : 'text-muted-foreground hover:text-foreground'
                        )}
                      >
                        <v.icon />
                      </button>
                    )
                  })}
                </div>
              )}
              {action}
            </div>
          )}
        </div>
        {children && <div className="border-t px-1.5 py-1.5 empty:hidden">{children}</div>}
      </div>
    </div>
  )
}
