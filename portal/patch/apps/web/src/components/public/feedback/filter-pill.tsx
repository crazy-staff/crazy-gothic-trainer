import { useIntl } from 'react-intl'
import { cn } from '@/lib/shared/utils'

interface FilterPillProps {
  pressed: boolean
  onClick: () => void
  children: React.ReactNode
  /** A status or tag color, shown as a dot before the label. */
  color?: string
  /** Square dot for tags, round for statuses. */
  dot?: 'round' | 'square'
  count?: string
  /** Bigger touch targets in the phone sheet. */
  size?: 'sm' | 'md'
  /** The full label, for a name the pill cuts short. */
  title?: string
  'aria-label'?: string
}

/**
 * One on/off choice in the filter panel: outlined when off, filled when on.
 * Only the label shortens in a narrow column; the count always shows.
 */
export function FilterPill({
  pressed,
  onClick,
  children,
  color,
  dot = 'round',
  count,
  size = 'sm',
  title,
  'aria-label': ariaLabel,
}: FilterPillProps): React.ReactElement {
  return (
    <button
      type="button"
      aria-pressed={pressed}
      aria-label={ariaLabel}
      title={title}
      onClick={onClick}
      className={cn(
        'inline-flex max-w-full items-center gap-1.5 rounded-lg border font-medium transition-colors duration-150',
        size === 'md' ? 'h-9 px-3 text-sm' : 'h-7 px-2 text-xs',
        pressed
          ? 'border-primary/40 bg-primary/10 text-primary'
          : 'border-border bg-secondary/50 text-muted-foreground hover:bg-accent hover:text-accent-foreground'
      )}
    >
      {color && (
        <span
          aria-hidden
          className={cn('size-1.5 shrink-0', dot === 'round' ? 'rounded-full' : 'rounded-[2px]')}
          style={{ backgroundColor: color }}
        />
      )}
      <span className="min-w-0 truncate">{children}</span>
      {count && <span className="shrink-0 font-normal tabular-nums opacity-70">{count}</span>}
    </button>
  )
}

/** Unfolds the rest of a long list. */
export function MoreButton({
  hidden,
  onClick,
  className,
}: {
  hidden: number
  onClick: () => void
  className?: string
}) {
  const intl = useIntl()
  return (
    <button
      type="button"
      onClick={onClick}
      className={cn(
        'rounded-lg px-2 py-1 text-xs text-muted-foreground transition-colors hover:bg-accent/60 hover:text-accent-foreground',
        className
      )}
    >
      {intl.formatMessage(
        { id: 'portal.feedback.panel.more', defaultMessage: '{count} more' },
        { count: hidden }
      )}
    </button>
  )
}
