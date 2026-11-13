/**
 * The public roadmap (replaces Quackback's board). On desktop its controls live in the app
 * sidebar: the roadmap list (one per area: trainer, docs, Discord...), sorting, boards and tags
 * grouped by their "kind: value" prefix. The page itself is the columns, using the full width.
 * Without the sidebar (phones) the same controls render above the columns.
 */
import { useEffect, useMemo, useRef, useState } from 'react'
import { useSuspenseQuery } from '@tanstack/react-query'
import { useIntl } from 'react-intl'
import { AnimatePresence, motion } from 'framer-motion'
import { MagnifyingGlassIcon, MapIcon, XMarkIcon } from '@heroicons/react/20/solid'
import { ChevronLeftIcon, ChevronRightIcon } from '@heroicons/react/24/outline'
import { cn } from '@/lib/shared/utils'
import { usePublicRoadmaps, useRoadmapDateBuckets, type RoadmapView } from '@/lib/client/hooks/use-roadmaps-query'
import { useSegments } from '@/lib/client/hooks/use-segments-queries'
import { usePillsScroll } from '@/lib/client/hooks/use-pills-scroll'
import { portalQueries } from '@/lib/client/queries/portal'
import type { RoadmapFilters } from '@/lib/shared/types'
import type { PostTag } from '@/lib/shared/db-types'
import { SidebarSlot } from '@/custom-docs/sidebar-slot'
import { sidebarItem, sidebarLabel } from '@/custom-docs/sidebar'
import { EASE } from '@/custom-docs/page-transition'
import { groupTags } from '@/custom-docs/tag-groups'
import { RoadmapColumn } from './roadmap-column'
import { usePublicRoadmapFilters } from './use-public-roadmap-filters'
import { usePublicRoadmapSelection } from './use-public-roadmap-selection'

interface RoadmapBoardProps {
  initialRoadmaps?: RoadmapView[]
  initialSelectedRoadmapId?: string | null
  isTeamMember?: boolean
}

type Sort = NonNullable<RoadmapFilters['sort']>
const SORTS: { value: Sort; label: string }[] = [
  { value: 'votes', label: 'Most votes' },
  { value: 'newest', label: 'Newest' },
  { value: 'oldest', label: 'Oldest' },
]

const TAGS_SHOWN = 6

function TagGroup({ name, items, selected, onToggle }: {
  name: string
  items: { tag: PostTag; label: string }[]
  selected: string[]
  onToggle: (id: string) => void
}) {
  const [all, setAll] = useState(false)
  const shown = all ? items : items.filter((t, i) => i < TAGS_SHOWN || selected.includes(t.tag.id))
  return (
    <div className="flex flex-col">
      <p className={sidebarLabel}>{name}</p>
      {shown.map(({ tag, label }) => {
        const on = selected.includes(tag.id)
        return (
          <button key={tag.id} type="button" aria-pressed={on} onClick={() => onToggle(tag.id)} className={sidebarItem(on)}>
            <span className="size-2 shrink-0 rounded-sm" style={{ backgroundColor: tag.color ?? 'var(--color-fd-muted-foreground)' }} />
            <span className="truncate">{label}</span>
          </button>
        )
      })}
      {items.length > shown.length && (
        <button type="button" onClick={() => setAll(true)} className={cn(sidebarItem(), 'text-xs')}>
          {items.length - shown.length} more
        </button>
      )}
    </div>
  )
}

interface PanelProps {
  roadmaps: RoadmapView[]
  selectedId: string | null | undefined
  onSelect: (id: string) => void
  filters: RoadmapFilters
  setFilters: (u: Partial<RoadmapFilters>) => void
  clearFilters: () => void
  boards: { id: string; name: string }[]
  tags: PostTag[]
  toggleBoard: (id: string) => void
  toggleTag: (id: string) => void
}

/** Roadmaps, sorting and filters: the sidebar panel (or the phone layout's top). */
function RoadmapPanel(p: PanelProps) {
  const groups = useMemo(() => groupTags(p.tags), [p.tags])
  const active = (p.filters.board?.length ?? 0) + (p.filters.tags?.length ?? 0)
  return (
    <div className="flex flex-col gap-5">
      {p.roadmaps.length > 1 && (
        <nav className="flex flex-col">
          <p className={sidebarLabel}>Roadmaps</p>
          {p.roadmaps.map((r) => {
            const on = r.id === p.selectedId
            return (
              <button key={r.id} type="button" aria-pressed={on} onClick={() => p.onSelect(r.id)} className={sidebarItem(on)}>
                <MapIcon />
                <span className="truncate">{r.name}</span>
              </button>
            )
          })}
        </nav>
      )}
      <div className="flex flex-col">
        <p className={sidebarLabel}>Sort</p>
        {SORTS.map((s) => {
          const on = (p.filters.sort ?? 'votes') === s.value
          return (
            <button key={s.value} type="button" aria-pressed={on} onClick={() => p.setFilters({ sort: s.value })} className={sidebarItem(on)}>
              {s.label}
            </button>
          )
        })}
      </div>
      {p.boards.length > 1 && (
        <div className="flex flex-col">
          <p className={sidebarLabel}>Boards</p>
          {p.boards.map((b) => {
            const on = p.filters.board?.includes(b.id) ?? false
            return (
              <button key={b.id} type="button" aria-pressed={on} onClick={() => p.toggleBoard(b.id)} className={sidebarItem(on)}>
                <span className="truncate">{b.name}</span>
              </button>
            )
          })}
        </div>
      )}
      {groups.map(([name, items]) => (
        <TagGroup key={name} name={name} items={items} selected={p.filters.tags ?? []} onToggle={p.toggleTag} />
      ))}
      {active > 0 && (
        <button type="button" onClick={p.clearFilters} className={cn(sidebarItem(), 'text-xs')}>
          <XMarkIcon />
          Clear {active} {active === 1 ? 'filter' : 'filters'}
        </button>
      )}
    </div>
  )
}

/** Search that filters as you type (no popover). */
function SearchField({ value, onChange }: { value?: string; onChange: (v: string | undefined) => void }) {
  const [text, setText] = useState(value ?? '')
  const timer = useRef<ReturnType<typeof setTimeout>>(undefined)
  useEffect(() => setText(value ?? ''), [value])
  return (
    <label className="flex h-9 w-full items-center gap-2 rounded-lg border bg-fd-secondary/50 px-2.5 text-sm text-fd-muted-foreground transition-colors focus-within:border-fd-primary/60 focus-within:bg-fd-background sm:w-64">
      <MagnifyingGlassIcon className="size-4 shrink-0" />
      <input
        value={text}
        onChange={(e) => {
          const v = e.target.value
          setText(v)
          clearTimeout(timer.current)
          timer.current = setTimeout(() => onChange(v.trim() || undefined), 250)
        }}
        placeholder="Search the roadmap"
        className="min-w-0 flex-1 bg-transparent text-fd-foreground outline-none placeholder:text-fd-muted-foreground"
      />
      {text && (
        <button type="button" aria-label="Clear search" onClick={() => { setText(''); onChange(undefined) }} className="rounded-md p-0.5 hover:text-fd-foreground">
          <XMarkIcon className="size-3.5" />
        </button>
      )}
    </label>
  )
}

export function RoadmapBoard({ initialRoadmaps, initialSelectedRoadmapId, isTeamMember }: RoadmapBoardProps): React.ReactElement {
  const intl = useIntl()
  const { selectedRoadmapId, setSelectedRoadmap } = usePublicRoadmapSelection()
  const { data: roadmaps } = usePublicRoadmaps({ enabled: !initialRoadmaps })
  const columnsScroll = usePillsScroll()
  const { filters, setFilters, clearFilters, toggleBoard, toggleTag } = usePublicRoadmapFilters()
  const { data: boards } = useSuspenseQuery(portalQueries.boards())
  const { data: tags } = useSuspenseQuery(portalQueries.tags())
  useSegments({ enabled: !!isTeamMember })

  const availableRoadmaps = initialRoadmaps ?? roadmaps ?? []
  const effectiveSelectedId = selectedRoadmapId ?? initialSelectedRoadmapId
  const selectedIndex = Math.max(0, availableRoadmaps.findIndex((r) => r.id === effectiveSelectedId))
  const selectedRoadmap = availableRoadmaps[selectedIndex]
  const { data: dateBuckets = [] } = useRoadmapDateBuckets(
    (effectiveSelectedId ?? 'roadmap_00000000000000000000000000') as `roadmap_${string}`,
    { public: true, enabled: selectedRoadmap?.type === 'date' }
  )
  const columns =
    selectedRoadmap?.type === 'date'
      ? dateBuckets.map((b) => ({ id: b.id, statusId: undefined, bucketId: b.id, name: b.label, icon: null, color: b.noEta ? '#6b7280' : '#3b82f6' }))
      : (selectedRoadmap?.columns ?? []).map((c) => ({ id: c.id, statusId: c.statusId, bucketId: undefined, name: c.name, icon: c.icon, color: c.color }))

  // which way the columns slide: down the roadmap list -> from below
  const previous = useRef(selectedIndex)
  const direction = selectedIndex === previous.current ? 0 : selectedIndex > previous.current ? 1 : -1
  useEffect(() => {
    previous.current = selectedIndex
  }, [selectedIndex])

  useEffect(() => {
    if (availableRoadmaps.length > 0 && !selectedRoadmapId && initialSelectedRoadmapId) setSelectedRoadmap(initialSelectedRoadmapId)
  }, [availableRoadmaps, selectedRoadmapId, initialSelectedRoadmapId, setSelectedRoadmap])

  if (availableRoadmaps.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center gap-2 py-16 text-center">
        <MapIcon className="size-10 text-fd-muted-foreground" />
        <p className="font-medium">{intl.formatMessage({ id: 'portal.roadmap.empty.title', defaultMessage: 'No roadmaps available' })}</p>
      </div>
    )
  }

  const panel = (
    <RoadmapPanel
      roadmaps={availableRoadmaps}
      selectedId={effectiveSelectedId}
      onSelect={setSelectedRoadmap}
      filters={filters}
      setFilters={setFilters}
      clearFilters={clearFilters}
      boards={boards}
      tags={tags}
      toggleBoard={toggleBoard}
      toggleTag={toggleTag}
    />
  )
  const chips = [
    ...(filters.board ?? []).map((id) => ({ id, label: boards.find((b) => b.id === id)?.name ?? id, off: () => toggleBoard(id) })),
    ...(filters.tags ?? []).map((id) => ({ id, label: tags.find((t) => t.id === id)?.name ?? id, off: () => toggleTag(id) })),
  ]

  return (
    <div className="flex min-h-0 flex-1 flex-col gap-4">
      <SidebarSlot fallback={<details className="rounded-xl border p-3 [&_summary]:cursor-pointer"><summary className="text-sm font-medium">Roadmaps and filters</summary><div className="mt-3">{panel}</div></details>}>
        {panel}
      </SidebarSlot>

      <div className="flex flex-wrap items-end justify-between gap-3">
        <div className="min-w-0">
          <AnimatePresence mode="wait" initial={false}>
            <motion.div
              key={selectedRoadmap?.id}
              initial={{ opacity: 0, y: direction * 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -direction * 10 }}
              transition={{ duration: 0.25, ease: EASE }}
            >
              <h2 className="text-lg font-semibold">{selectedRoadmap?.name}</h2>
              {selectedRoadmap?.description && <p className="text-sm text-fd-muted-foreground">{selectedRoadmap.description}</p>}
            </motion.div>
          </AnimatePresence>
        </div>
        <SearchField value={filters.search} onChange={(search) => setFilters({ search })} />
      </div>

      {chips.length > 0 && (
        <div className="flex flex-wrap items-center gap-1.5">
          {chips.map((c) => (
            <button key={c.id} type="button" onClick={c.off} className="flex items-center gap-1 rounded-lg border bg-fd-secondary/50 py-1 ps-2.5 pe-1.5 text-xs transition-colors hover:border-fd-primary/50">
              {c.label}
              <XMarkIcon className="size-3.5 text-fd-muted-foreground" />
            </button>
          ))}
          <button type="button" onClick={clearFilters} className="rounded-lg px-2 py-1 text-xs text-fd-muted-foreground hover:text-fd-foreground">
            Clear all
          </button>
        </div>
      )}

      {effectiveSelectedId && (
        <div className="relative min-h-[24rem] flex-1 sm:min-h-0">
          <div ref={columnsScroll.ref} className="h-full overflow-x-auto overflow-y-hidden pb-2 scrollbar-none snap-x snap-mandatory">
          <AnimatePresence mode="wait" initial={false}>
            <motion.div
              key={effectiveSelectedId}
              initial={{ opacity: 0, y: direction * 28 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -direction * 28 }}
              transition={{ duration: 0.35, ease: EASE }}
              className="grid h-full auto-cols-[minmax(17rem,1fr)] grid-flow-col gap-3"
            >
              {columns.map((column, index) => (
                <motion.div
                  key={column.id}
                  initial={{ opacity: 0, y: 12 }}
                  animate={{ opacity: 1, y: 0 }}
                  transition={{ duration: 0.4, ease: EASE, delay: index * 0.05 }}
                  className="flex min-w-0 snap-start flex-col"
                >
                  <RoadmapColumn
                    roadmapId={effectiveSelectedId as `roadmap_${string}`}
                    statusId={column.statusId}
                    bucketId={column.bucketId}
                    title={column.name}
                    icon={column.icon}
                    color={column.color}
                    filters={filters}
                  />
                </motion.div>
              ))}
            </motion.div>
          </AnimatePresence>
          </div>
          {columnsScroll.canScrollLeft && (
            <button type="button" onClick={() => columnsScroll.scrollBy(-320)} aria-label="Scroll left" className="absolute inset-y-0 start-0 z-10 flex items-center bg-gradient-to-r from-fd-background/80 to-transparent ps-1 pe-8">
              <ChevronLeftIcon className="size-5 text-fd-muted-foreground" />
            </button>
          )}
          {columnsScroll.canScrollRight && (
            <button type="button" onClick={() => columnsScroll.scrollBy(320)} aria-label="Scroll right" className="absolute inset-y-0 end-0 z-10 flex items-center bg-gradient-to-l from-fd-background/80 to-transparent ps-8 pe-1">
              <ChevronRightIcon className="size-5 text-fd-muted-foreground" />
            </button>
          )}
        </div>
      )}
    </div>
  )
}
