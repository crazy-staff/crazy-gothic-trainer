/**
 * The hover of every menu on the site (from Fumadocs): one soft block per column glides to the
 * row under the mouse or the keyboard focus. Used by the app sidebar, the admin rail and the
 * admin side panes, so they all move the same way.
 */
export const GLIDE_BLOCK_CLASS =
  'pointer-events-none absolute top-0 left-0 z-0 rounded-lg bg-fd-accent/70 opacity-0 transition-opacity duration-200 ease-[cubic-bezier(0.22,1,0.36,1)] motion-safe:transition-[translate,width,height,opacity]'

/** Rows that already show their own fill: the block hides on them. */
const FILLED = '[data-active=true], [data-active=""], [aria-pressed=true], [aria-current=page], [aria-selected=true], [data-no-glide], .bg-primary\\/10'

/**
 * Starts the glide in `area`. The block is the area's first child with [data-glide-block], or
 * one made here. With `zoned`, only rows inside a [data-glide-zone] count.
 */
export function glide(area: HTMLElement | null, { zoned = false } = {}) {
  if (!area) return
  let block = area.firstElementChild
  if (!(block instanceof HTMLElement) || !block.hasAttribute('data-glide-block')) {
    block = document.createElement('div')
    block.setAttribute('data-glide-block', '')
    block.setAttribute('aria-hidden', 'true')
    ;(block as HTMLElement).className = GLIDE_BLOCK_CLASS
    area.prepend(block)
  }
  const el = block as HTMLElement
  if (getComputedStyle(area).position === 'static') area.style.position = 'relative'
  area.setAttribute('data-glide', '')
  let current: Element | null = null
  const show = (item: Element | null) => {
    if (item === current) return
    el.style.transitionProperty = current ? '' : 'opacity'
    current = item
    if (!item) {
      el.style.opacity = '0'
      return
    }
    const rect = item.getBoundingClientRect()
    const origin = area.getBoundingClientRect()
    el.style.translate = `${rect.left - origin.left}px ${rect.top - origin.top}px`
    el.style.width = `${rect.width}px`
    el.style.height = `${rect.height}px`
    el.style.opacity = item.matches(FILLED) ? '0' : '1'
  }
  const itemOf = (target: EventTarget | null) => {
    if (!(target instanceof Element)) return null
    if (zoned && !target.closest('[data-glide-zone]')) return null
    const item = target.closest('a[href], button, [role=menuitem], [role=option]')
    return item && area.contains(item) && !item.closest('[data-no-glide-area]') ? item : null
  }
  const controller = new AbortController()
  const { signal } = controller
  area.addEventListener('pointerover', (e) => {
    const item = itemOf(e.target)
    if (item && e.pointerType === 'mouse') show(item)
    else if (!item) show(null)
  }, { signal })
  area.addEventListener('pointerleave', () => show(null), { signal })
  area.addEventListener('scroll', () => show(null), { signal, capture: true })
  area.addEventListener('click', () => requestAnimationFrame(() => current && el.style.setProperty('opacity', current.matches(FILLED) ? '0' : '1')), { signal })
  area.addEventListener('focusin', (e) => {
    if (e.target instanceof Element && e.target.matches(':focus-visible')) show(itemOf(e.target))
  }, { signal })
  area.addEventListener('focusout', () => {
    if (!area.matches(':hover')) show(null)
  }, { signal })
  return () => controller.abort()
}
