import { useId, useState } from 'react'
import { useIntl, type MessageDescriptor } from 'react-intl'
import { cn } from '@/lib/shared/utils'
import type { PublicFeedView } from '@/lib/shared/types/filters'
import type { PublicFeedbackFilters } from '@/lib/shared/types'
import type { PostStatusEntity, PostTag } from '@/lib/shared/db-types'
import type { PublicPostFacetCounts } from '@/lib/shared/types/filters'
import { MoreButton } from './filter-pill'
import { toggleItem } from '@/components/shared/filter-utils'
import { groupTags } from '@/custom-docs/tag-groups'

interface PanelBoard {
  id: string
  slug: string
  name: string
}

interface PortalFilterPanelProps {
  filters: PublicFeedbackFilters
  setFilters: (updates: Partial<PublicFeedbackFilters>) => void
  boards: PanelBoard[]
  statuses: PostStatusEntity[]
  tags: PostTag[]
  /** Undefined while the counts load; the panel renders without them. */
  counts?: PublicPostFacetCounts | null
  /** Signed-in (non-anonymous) viewers get the Show section. */
  showViews: boolean
  /** The phone sheet puts Status before the board list. */
  layout?: 'sidebar' | 'sheet'
  /** Rendered at the foot of the panel (the less common filters). */
  footer?: React.ReactNode
}

/** Boards and tags listed before the rest fold behind "N more". */
const BOARDS_SHOWN = 6
const TAGS_SHOWN = 8

export const VIEW_LABELS: Record<'all' | PublicFeedView, MessageDescriptor> = {
  all: { id: 'portal.feedback.panel.view.all', defaultMessage: 'All posts' },
  mine: { id: 'portal.feedback.panel.view.mine', defaultMessage: 'My posts' },
  voted: { id: 'portal.feedback.panel.view.voted', defaultMessage: 'Voted' },
  following: { id: 'portal.feedback.panel.view.following', defaultMessage: 'Following' },
}

export const rowClass = (active: boolean) =>
  cn(
    'relative flex w-full max-w-full items-center gap-2 rounded-lg px-2.5 py-2 text-start text-sm transition-colors duration-150',
    active
      ? 'bg-primary/10 font-medium text-primary'
      : 'text-muted-foreground hover:text-accent-foreground'
  )

/** A row's post count. Zero shows nothing: the row is still there to pick. */
function Count({ value }: { value: number | undefined }) {
  const intl = useIntl()
  if (!value) return null
  return (
    <span className="ms-auto shrink-0 ps-1 text-xs text-muted-foreground tabular-nums">
      {intl.formatNumber(value)}
    </span>
  )
}

export function PanelSection({ title, children }: { title: string; children: React.ReactNode }) {
  const headingId = useId()
  return (
    <div role="group" aria-labelledby={headingId} className="portal-filter-section">
      <h2 id={headingId} className="mb-1.5 px-2.5 text-xs font-medium text-muted-foreground">
        {title}
      </h2>
      {children}
    </div>
  )
}

/**
 * The portal feed's filters with live counts: views, boards, statuses and
 * tags. One component, mounted in the desktop sidebar and the phone sheet.
 * A section only renders when it has a choice to offer.
 */
export function PortalFilterPanel({
  filters,
  setFilters,
  boards,
  statuses,
  tags,
  counts,
  showViews,
  layout = 'sidebar',
  footer,
}: PortalFilterPanelProps): React.ReactElement {
  const intl = useIntl()
  const [allBoardsShown, setAllBoardsShown] = useState(false)
  const [allTagsShown, setAllTagsShown] = useState(false)
  const pickedStatuses =
    filters.status?.map((value) => statuses.find((s) => s.id === value)?.slug ?? value) ?? []
  function toggleStatus(slug: string): void {
    const next = pickedStatuses.includes(slug) ? pickedStatuses.filter((s) => s !== slug) : [...pickedStatuses, slug]
    const inOrder = statuses.map((s) => s.slug).filter((s) => next.includes(s))
    setFilters({ status: inOrder.length > 0 ? inOrder : undefined })
  }

  const showSection = showViews && (
    <PanelSection
      key="show"
      title={intl.formatMessage({ id: 'portal.feedback.panel.show', defaultMessage: 'Show' })}
    >
      <div className="flex flex-col">
        {(['all', 'mine', 'voted', 'following'] as const).map((view) => {
          const active = (filters.view ?? 'all') === view
          return (
            <button
              key={view}
              type="button"
              aria-pressed={active}
              onClick={() => setFilters({ view: view === 'all' ? undefined : view })}
              className={rowClass(active)}
            >
              <span className="truncate">{intl.formatMessage(VIEW_LABELS[view])}</span>
              {view !== 'all' && <Count value={counts?.views?.[view]} />}
            </button>
          )
        })}
      </div>
    </PanelSection>
  )

  // The picked board stays listed when it is one of the folded ones.
  const shownBoards = allBoardsShown
    ? boards
    : boards.filter((board, i) => i < BOARDS_SHOWN || board.slug === filters.board)
  const boardSection = boards.length > 1 && (
    <PanelSection
      key="boards"
      title={intl.formatMessage({ id: 'portal.feedback.sidebar.boards', defaultMessage: 'Boards' })}
    >
      <div className="flex flex-col">
        <button
          type="button"
          aria-pressed={!filters.board}
          onClick={() => setFilters({ board: undefined })}
          className={rowClass(!filters.board)}
        >
          <span className="truncate">
            {intl.formatMessage({
              id: 'portal.feedback.panel.allBoards',
              defaultMessage: 'All boards',
            })}
          </span>
        </button>
        {shownBoards.map((board) => (
          <button
            key={board.id}
            type="button"
            aria-pressed={filters.board === board.slug}
            onClick={() => setFilters({ board: board.slug })}
            className={rowClass(filters.board === board.slug)}
          >
            <span className="min-w-0 truncate">{board.name}</span>
            <Count value={counts?.boards[board.slug]} />
          </button>
        ))}
        {boards.length > shownBoards.length && (
          <MoreButton
            hidden={boards.length - shownBoards.length}
            onClick={() => setAllBoardsShown(true)}
            className="py-1.5"
          />
        )}
      </div>
    </PanelSection>
  )

  const statusSection = (
    <PanelSection
      key="status"
      title={intl.formatMessage({
        id: 'portal.feedback.filter.category.status',
        defaultMessage: 'Status',
      })}
    >
      <div className="flex flex-col">
        {statuses.map((status) => {
          const on = pickedStatuses.includes(status.slug)
          return (
            <button key={status.id} type="button" aria-pressed={on} onClick={() => toggleStatus(status.slug)} className={rowClass(on)}>
              <span className="size-2 shrink-0 rounded-sm" style={{ backgroundColor: status.color }} />
              <span className="min-w-0 truncate">{status.name}</span>
              <Count value={counts?.statuses?.[status.slug]} />
            </button>
          )
        })}
      </div>
    </PanelSection>
  )

  // Once the counts are in, tags on no visible post drop out (a picked tag
  // stays, so it can be unpicked).
  const usableTags = tags.filter(
    (tag) => !counts || (counts.tags[tag.id] ?? 0) > 0 || filters.tagIds?.includes(tag.id)
  )
  const tagGroups = groupTags(usableTags)
  const tagSection = usableTags.length > 1 && (
    <div key="tags" className="flex flex-col gap-5">
      {tagGroups.map(([name, items]) => {
        const shown = allTagsShown ? items : items.filter((t, i) => i < TAGS_SHOWN || filters.tagIds?.includes(t.tag.id))
        return (
          <PanelSection key={name} title={name}>
            <div className="flex flex-col">
              {shown.map(({ tag, label }) => {
                const on = filters.tagIds?.includes(tag.id) ?? false
                return (
                  <button
                    key={tag.id}
                    type="button"
                    aria-pressed={on}
                    onClick={() => {
                      const next = toggleItem(filters.tagIds, tag.id)
                      setFilters({ tagIds: next?.length ? next : undefined })
                    }}
                    className={rowClass(on)}
                  >
                    <span className="size-2 shrink-0 rounded-sm" style={{ backgroundColor: tag.color }} />
                    <span className="min-w-0 truncate">{label}</span>
                    <Count value={counts?.tags[tag.id]} />
                  </button>
                )
              })}
              {items.length > shown.length && (
                <MoreButton hidden={items.length - shown.length} onClick={() => setAllTagsShown(true)} className="py-1.5" />
              )}
            </div>
          </PanelSection>
        )
      })}
    </div>
  )

  const sections =
    layout === 'sheet'
      ? [showSection, statusSection, boardSection, tagSection]
      : [showSection, boardSection, statusSection, tagSection]

  return (
    <div
      className={cn(
        'flex flex-col gap-5',
        layout === 'sheet' && 'overflow-hidden rounded-xl border border-border bg-card p-3'
      )}
    >
      {sections}
      {footer}
    </div>
  )
}
