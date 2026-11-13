import { useEffect, useRef, type ReactNode } from 'react'
import { glide } from '@/custom-docs/glide'
import { useIntl } from 'react-intl'

/** The admin's one main region; pages inside it never open another. */
export const ADMIN_MAIN_ID = 'admin-main'

/** The admin shell: the sidebar beside an inset page sheet. */
export function AdminWorkspaceFrame({
  sidebar,
  notices,
  children,
}: {
  sidebar: ReactNode
  notices: ReactNode
  children: ReactNode
}) {
  // Formatted in place: a message component would render once more on every page.
  const intl = useIntl()
  const root = useRef<HTMLDivElement>(null)
  useAdminGlide(root)
  return (
    <div ref={root} className="flex h-dvh bg-background">
      {/* First stop for keyboard users: past the rail, straight to the page. */}
      <a
        href={`#${ADMIN_MAIN_ID}`}
        className="sr-only focus-visible:not-sr-only focus-visible:fixed focus-visible:start-3 focus-visible:top-3 focus-visible:z-[100] focus-visible:rounded-md focus-visible:bg-background focus-visible:px-3 focus-visible:py-2 focus-visible:text-sm focus-visible:font-medium focus-visible:text-foreground focus-visible:shadow-md focus-visible:outline-2 focus-visible:outline-solid focus-visible:outline-ring"
      >
        {intl.formatMessage({ id: 'admin.skipToContent', defaultMessage: 'Skip to content' })}
      </a>
      {sidebar}
      <main
        id={ADMIN_MAIN_ID}
        tabIndex={-1}
        data-admin-shell=""
        className="min-w-0 flex-1 overflow-hidden bg-chrome p-0 outline-none sm:h-dvh sm:py-2 sm:pe-2"
      >
        <div
          data-admin-canvas=""
          className="flex h-full flex-col overflow-hidden bg-background pt-14 text-foreground sm:rounded-[14px] sm:border sm:border-chrome-hairline sm:pt-0 sm:shadow-chrome-canvas"
        >
          {notices}
          <div className="min-h-0 flex-1 overflow-hidden">{children}</div>
        </div>
      </main>
    </div>
  )
}

/**
 * The rail and every side pane (settings, analytics, feedback filters...) get the app sidebar's
 * gliding hover. Panes come and go with the route, so new ones are picked up as they mount.
 */
function useAdminGlide(root: React.RefObject<HTMLDivElement | null>) {
  useEffect(() => {
    const el = root.current
    if (!el) return
    const done = new WeakSet<Element>()
    const stops: (() => void)[] = []
    const scan = () => {
      el.querySelectorAll('[data-admin-rail], [data-side-pane]').forEach((area) => {
        if (done.has(area) || !(area instanceof HTMLElement)) return
        done.add(area)
        // attached on the first hover, never while React is still hydrating the pane
        area.addEventListener(
          'pointerenter',
          () => {
            const stop = glide(area)
            if (stop) stops.push(stop)
          },
          { once: true }
        )
      })
    }
    scan()
    const observer = new MutationObserver(scan)
    observer.observe(el, { childList: true, subtree: true })
    return () => {
      observer.disconnect()
      stops.forEach((stop) => stop())
    }
  }, [root])
}
