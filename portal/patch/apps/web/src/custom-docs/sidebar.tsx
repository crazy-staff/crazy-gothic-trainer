/**
 * The one sidebar of the app (portal and docs). Based on the sidebar of Fumadocs' "spacious"
 * layout (packages/base-ui/src/layouts/spacious/slots/sidebar.tsx, MIT (c) Fuma), with our order:
 *   workspace · search · portal sections · [docs: game/renderer, page tree] · page panel · footer
 * The search field and the sections never move; what a page adds appears under them.
 */
import { Fragment, type ReactNode } from 'react'
import { Link as RouterLink, useRouterState } from '@tanstack/react-router'
import { useIntl } from 'react-intl'
import { AnimatePresence, motion } from 'framer-motion'
import { Menu } from '@base-ui/react/menu'
import { ScrollArea } from '@base-ui/react/scroll-area'
import { CheckIcon, ChevronsUpDownIcon, SidebarIcon } from 'lucide-react'
import * as Base from '@fumadocs/base-ui/components/sidebar/base'
import { createPageTreeRenderer } from '@fumadocs/base-ui/components/sidebar/page-tree'
import { buttonVariants } from '@fumadocs/base-ui/components/ui/button'
import { useSpaciousLayout } from '@fumadocs/base-ui/layouts/spacious'
import {
  BookOpenIcon,
  ChatBubbleLeftRightIcon,
  CpuChipIcon,
  InformationCircleIcon,
  LifebuoyIcon,
  LinkIcon,
  MapIcon,
  MegaphoneIcon,
  SignalIcon,
} from '@heroicons/react/20/solid'
import { cn } from '@/lib/shared/utils'
import { EASE } from './page-transition'
import { navItemLabel, useSectionIndex, useSections } from './shell-parts'

export const SIDEBAR_SLOT_ID = 'app-sidebar-slot'

/** One look for every row of the sidebar (sections, docs pages, filters). */
export const sidebarItem = (active = false) =>
  cn(
    'relative flex w-full items-center gap-3 rounded-lg px-2.5 py-2 text-start text-sm wrap-anywhere outline-none transition-colors focus-visible:ring-2 focus-visible:ring-fd-ring [&_svg]:size-4 [&_svg]:shrink-0',
    active ? 'bg-fd-primary/10 font-medium text-fd-primary' : 'text-fd-muted-foreground hover:text-fd-accent-foreground'
  )
export const sidebarLabel = 'mb-1.5 px-2.5 text-xs font-medium text-fd-muted-foreground'

const ICONS: Record<string, ReactNode> = {
  feedback: <ChatBubbleLeftRightIcon />,
  roadmap: <MapIcon />,
  changelog: <MegaphoneIcon />,
  help: <BookOpenIcon />,
  support: <LifebuoyIcon />,
  status: <SignalIcon />,
}

function Sections() {
  const intl = useIntl()
  const items = useSections()
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const current = useSectionIndex(pathname)
  return (
    <nav className="flex flex-col">
      {items.map((item, i) => {
        const label = navItemLabel(item, intl.formatMessage)
        if (item.kind === 'link')
          return (
            <a key={item.id} href={item.href} target={item.newTab ? '_blank' : undefined} rel="noopener noreferrer" className={sidebarItem()}>
              <LinkIcon />
              {label}
            </a>
          )
        return (
          <RouterLink key={item.id} to={item.to as '/'} data-active={i === current} className={sidebarItem(i === current)}>
            {ICONS[item.type]}
            {label}
          </RouterLink>
        )
      })}
    </nav>
  )
}

// ---------------------------------------------------------------- docs: game and renderer

interface Option {
  title: string
  description: string
  url: string
  icon: ReactNode
}
const badge = (t: string) => (
  <span className="flex size-4 items-center justify-center rounded bg-fd-primary/15 text-[9px] font-bold text-fd-primary">{t}</span>
)
const GAMES: Option[] = [
  { title: 'Gothic 1', description: 'Gothic 1 Crazy Trainer', url: '/hc', icon: badge('G1') },
  { title: 'Gothic 2', description: 'Coming later', url: '/hc/soon/gothic-2', icon: badge('G2') },
]
const RENDERERS: Option[] = [
  { title: 'GD3D11', description: 'DirectX 11 renderer', url: '/hc', icon: <CpuChipIcon /> },
  { title: 'Original', description: 'Without GD3D11 · coming later', url: '/hc/soon/original-renderer', icon: <CpuChipIcon /> },
]

function Picker({ options, pathname }: { options: Option[]; pathname: string }) {
  const selected = options.find((o) => o.url !== '/hc' && pathname.startsWith(o.url)) ?? options[0]
  return (
    <Menu.Root>
      <Menu.Trigger className="flex h-10 w-full items-center gap-3 ps-[calc(--spacing(2.5)-1px)] pe-2.5 text-start text-sm font-medium outline-none transition-colors hover:bg-fd-accent focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-fd-ring data-popup-open:bg-fd-accent [&_svg]:size-4 [&_svg]:shrink-0">
        {selected.icon}
        <span className="flex-1 truncate">{selected.title}</span>
        <ChevronsUpDownIcon className="size-3.5! text-fd-muted-foreground" />
      </Menu.Trigger>
      <Menu.Portal>
        <Menu.Positioner align="start" sideOffset={8} alignOffset={-4} positionMethod="fixed" className="z-50">
          <Menu.Popup className="flex w-[calc(var(--anchor-width)+--spacing(2))] flex-col rounded-2xl border bg-fd-popover p-1 text-sm text-fd-popover-foreground shadow-lg outline-none origin-(--transform-origin) transition-[opacity,scale] duration-150 ease-[cubic-bezier(0.22,1,0.36,1)] data-starting-style:scale-95 data-starting-style:opacity-0 data-ending-style:scale-97 data-ending-style:opacity-0">
            {options.map((o) => (
              <Menu.LinkItem
                key={o.url + o.title}
                closeOnClick
                render={<RouterLink to={o.url as '/'} />}
                className="flex min-h-9 w-full items-start gap-3 rounded-lg px-[calc(--spacing(2.5)-1px)] py-2 text-start outline-none transition-colors duration-100 data-highlighted:bg-fd-accent data-highlighted:text-fd-accent-foreground [&_svg]:size-4 [&_svg]:shrink-0"
              >
                <span className="mt-0.5">{o.icon}</span>
                <span className="flex min-w-0 flex-1 flex-col">
                  <span className="truncate font-medium">{o.title}</span>
                  <span className="truncate text-xs text-fd-muted-foreground">{o.description}</span>
                </span>
                {o === selected && <CheckIcon className="mt-0.5 text-fd-primary" />}
              </Menu.LinkItem>
            ))}
          </Menu.Popup>
        </Menu.Positioner>
      </Menu.Portal>
    </Menu.Root>
  )
}

function DocsSection({ pathname }: { pathname: string }) {
  return (
    <motion.div
      key="docs"
      initial={{ opacity: 0, height: 0 }}
      animate={{ opacity: 1, height: 'auto' }}
      exit={{ opacity: 0, height: 0 }}
      transition={{ duration: 0.4, ease: EASE }}
      className="overflow-clip"
    >
      <div className="mt-4 mb-5 flex flex-col divide-y overflow-clip rounded-xl border bg-fd-secondary/50">
        <Picker options={GAMES} pathname={pathname} />
        <Picker options={RENDERERS} pathname={pathname} />
      </div>
      <div className="flex flex-col">
        <PageTree />
      </div>
    </motion.div>
  )
}

// ---------------------------------------------------------------- the sidebar

function Viewport() {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const onDocs = pathname === '/hc' || pathname.startsWith('/hc/')
  return (
    <ScrollArea.Root className="mt-2 min-h-0 flex-1">
      <ScrollArea.Viewport className="size-full overscroll-contain px-3 pt-3 pb-8 mask-t-from-[calc(100%-12px)] mask-b-from-[calc(100%-32px)]">
        <ScrollArea.Content ref={follow} className="relative flex flex-col">
          <div
            aria-hidden
            className="pointer-events-none absolute top-0 left-0 rounded-lg bg-fd-accent/60 bg-clip-content py-0.5 opacity-0 transition-opacity duration-200 ease-[cubic-bezier(0.22,1,0.36,1)] motion-safe:transition-[translate,width,height,opacity]"
          />
          <Sections />
          <AnimatePresence initial={false}>{onDocs && <DocsSection pathname={pathname} />}</AnimatePresence>
          {/* the page's own panel (SidebarSlot) */}
          <div id={SIDEBAR_SLOT_ID} className="empty:hidden" />
        </ScrollArea.Content>
      </ScrollArea.Viewport>
      <ScrollArea.Scrollbar className={(s) => cn('flex w-1.5 select-none py-2 transition-opacity', !s.hovering && 'opacity-0')}>
        <ScrollArea.Thumb className="flex-1 rounded-full bg-fd-border" />
      </ScrollArea.Scrollbar>
    </ScrollArea.Root>
  )
}

function Footer() {
  return (
    <div className="flex items-center gap-1 px-3 pb-3">
      <RouterLink to={'/about' as '/'} className={cn(sidebarItem(), 'hover:bg-fd-accent/60')}>
        <InformationCircleIcon />
        About
      </RouterLink>
    </div>
  )
}

function Top() {
  const { slots } = useSpaciousLayout()
  return (
    <div className="flex flex-col gap-2 px-3">
      {slots.searchTrigger && (
        <slots.searchTrigger.full hideIfDisabled className="h-10 gap-3 rounded-xl ps-[calc(--spacing(2.5)-1px)] pe-2 [&_svg]:shrink-0" />
      )}
    </div>
  )
}

export function Sidebar() {
  const { collapsed, mode } = Base.useSidebar()
  const { slots } = useSpaciousLayout()
  if (mode !== 'full') return null
  return (
    <aside
      id="nd-sidebar"
      data-collapsed={collapsed}
      inert={collapsed}
      className={cn(
        '[grid-area:sidebar] flex min-h-0 overflow-clip text-sm md:layout:[--fd-sidebar-width:268px] max-md:hidden',
        collapsed && 'md:layout:[--fd-sidebar-col:--spacing(2)]'
      )}
    >
      <div
        className={cn(
          'flex w-(--fd-sidebar-width) shrink-0 flex-col transition-[opacity,translate] duration-300 ease-[cubic-bezier(0.22,1,0.36,1)]',
          collapsed && '-translate-x-3 opacity-0'
        )}
      >
        <div className="my-2 flex h-(--fd-header-height) items-center gap-2 ps-5.5 pe-3">
          <slots.navTitle className="me-auto inline-flex min-w-0 items-center gap-2 text-[0.9375rem] font-semibold" />
          <Base.SidebarCollapseTrigger className={cn(buttonVariants({ variant: 'ghost', size: 'icon-sm' }), 'size-8 text-fd-muted-foreground')}>
            <SidebarIcon />
          </Base.SidebarCollapseTrigger>
        </div>
        <Top />
        <Viewport />
        <Footer />
      </div>
    </aside>
  )
}

export function SidebarDrawer() {
  const { slots } = useSpaciousLayout()
  return (
    <Fragment>
      <Base.SidebarDrawerOverlay className="fixed inset-0 z-40 backdrop-blur-xs data-[state=open]:animate-fd-fade-in data-[state=closed]:animate-fd-fade-out" />
      <Base.SidebarDrawerContent className="fixed inset-y-0 inset-e-0 z-40 flex w-[85%] max-w-95 flex-col border-s bg-fd-background text-[0.9375rem] shadow-lg data-[state=open]:animate-fd-sidebar-in data-[state=closed]:animate-fd-sidebar-out">
        <div className="flex h-(--fd-header-height) items-center gap-1.5 ps-5 pe-2.5">
          <slots.navTitle className="me-auto inline-flex min-w-0 items-center gap-2 font-semibold" />
          <Base.SidebarTrigger className={buttonVariants({ variant: 'ghost', size: 'icon-sm', className: 'p-2' })}>
            <SidebarIcon />
          </Base.SidebarTrigger>
        </div>
        <Top />
        <Viewport />
        <Footer />
      </Base.SidebarDrawerContent>
    </Fragment>
  )
}

export function SidebarProvider(props: Base.SidebarProviderProps) {
  return <Base.SidebarProvider {...props} />
}

/** A soft block glides to the row under the mouse or the keyboard focus (from Fumadocs). */
function follow(area: HTMLDivElement | null) {
  const block = area?.firstElementChild
  if (!area || !(block instanceof HTMLElement)) return
  let current: Element | null = null
  const show = (item: Element | null) => {
    if (item === current) return
    block.style.transitionProperty = current ? '' : 'opacity'
    current = item
    if (!item) {
      block.style.opacity = '0'
      return
    }
    const rect = item.getBoundingClientRect()
    const origin = area.getBoundingClientRect()
    block.style.translate = `${rect.left - origin.left}px ${rect.top - origin.top}px`
    block.style.width = `${rect.width}px`
    block.style.height = `${rect.height}px`
    block.style.opacity = item.matches('[data-active=true], [aria-pressed=true], [data-no-glide]') ? '0' : '1'
  }
  const itemOf = (target: EventTarget | null) => (target instanceof Element ? target.closest('a[href], button') : null)
  const controller = new AbortController()
  const { signal } = controller
  area.addEventListener('pointerover', (e) => {
    const item = itemOf(e.target)
    if (item && e.pointerType === 'mouse') show(item)
    else if (!item) show(null)
  }, { signal })
  area.addEventListener('pointerleave', () => show(null), { signal })
  area.addEventListener('focusin', (e) => {
    if (e.target instanceof Element && e.target.matches(':focus-visible')) show(itemOf(e.target))
  }, { signal })
  area.addEventListener('focusout', () => {
    if (!area.matches(':hover')) show(null)
  }, { signal })
  return () => controller.abort()
}

// ---------------------------------------------------------------- docs page tree

function TreeSeparator({ className, ...props }: React.ComponentProps<'p'>) {
  return <Base.SidebarSeparator className={cn(sidebarLabel, 'mt-5 first:mt-0 [&_svg]:size-3.5 [&_svg]:shrink-0', className)} {...props} />
}
function TreeItem({ active, className, children, ...props }: React.ComponentProps<typeof Base.SidebarItem>) {
  return (
    <Base.SidebarItem active={active} className={cn(sidebarItem(active), className)} {...props}>
      {children}
    </Base.SidebarItem>
  )
}
function TreeFolderTrigger(props: React.ComponentProps<typeof Base.SidebarFolderTrigger>) {
  return <Base.SidebarFolderTrigger {...props} className={cn(sidebarItem(), '[&>[data-icon]]:size-3.5')} />
}
function TreeFolderLink({ active, className, children, ...props }: React.ComponentProps<typeof Base.SidebarFolderLink>) {
  return (
    <Base.SidebarFolderLink active={active} className={cn(sidebarItem(active), className)} {...props}>
      {children}
    </Base.SidebarFolderLink>
  )
}
function TreeFolderContent({ children, ...props }: React.ComponentProps<typeof Base.SidebarFolderContent>) {
  return (
    <Base.SidebarFolderContent {...props} className="relative ms-3 flex flex-col ps-1">
      <span className="absolute inset-y-1 start-0 w-px bg-fd-border" />
      {children}
    </Base.SidebarFolderContent>
  )
}
const PageTree = createPageTreeRenderer({
  SidebarFolder: Base.SidebarFolder,
  SidebarFolderContent: TreeFolderContent,
  SidebarFolderLink: TreeFolderLink,
  SidebarFolderTrigger: TreeFolderTrigger,
  SidebarItem: TreeItem,
  SidebarSeparator: TreeSeparator,
})
