import { cn } from '@/lib/shared/utils'

/** A rail row, styled like the app sidebar (custom-docs/sidebar.tsx): 36px, 14px, 8px corners. */
export function railControlClass(isActive = false) {
  return cn(
    'relative box-border flex min-h-9 w-full items-center gap-3 rounded-lg px-2.5 text-sm transition-colors duration-150',
    'outline-none focus-visible:ring-2 focus-visible:ring-ring [&>svg]:size-4 [&>svg]:shrink-0',
    isActive
      ? 'bg-primary/10 font-medium text-primary'
      : 'text-muted-foreground hover:text-accent-foreground'
  )
}
