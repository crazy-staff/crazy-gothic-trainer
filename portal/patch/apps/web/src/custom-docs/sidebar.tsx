/**
 * The one sidebar of the app (portal and docs). Based on the sidebar of Fumadocs' "spacious"
 * layout (packages/base-ui/src/layouts/spacious/slots/sidebar.tsx, MIT (c) Fuma), with our order:
 *   workspace · search · portal sections · [docs: game/renderer, page tree] · page panel · footer
 * The search field and the sections never move; what a page adds appears under them.
 */
import { Fragment, useEffect, useRef, type ReactNode } from 'react'
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
  InformationCircleIcon,
  LifebuoyIcon,
  LinkIcon,
  MapIcon,
  MegaphoneIcon,
  SignalIcon,
} from '@heroicons/react/20/solid'
import { cn } from '@/lib/shared/utils'
import { EASE } from './page-transition'
import { GLIDE_BLOCK_CLASS, glide } from './glide'
import { menuItem, menuPopup, navItemLabel, ThemeToggle, useSectionIndex, useSections } from './shell-parts'
import { GAMES, RENDERERS, STATUS_LABEL, variantUrl, type Variant } from './variants'

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

const badge = (t: string) => (
  <span className="flex h-5 min-w-5 items-center justify-center rounded-md bg-fd-primary/15 px-1 text-[9px] font-bold text-fd-primary">{t}</span>
)
const statusTag = (v: Variant) => (
  <span
    className={cn(
      'shrink-0 rounded-md px-1.5 py-px text-[10px] font-medium',
      v.status === 'available' ? 'bg-fd-primary/12 text-fd-primary' : 'bg-fd-muted text-fd-muted-foreground'
    )}
  >
    {STATUS_LABEL[v.status]}
  </span>
)

function Picker({ kind, options, pathname }: { kind: 'game' | 'renderer'; options: Variant[]; pathname: string }) {
  const selected =
    options.find((o) => o.status !== 'available' && pathname.startsWith(variantUrl(kind, o))) ??
    options.find((o) => o.status === 'available') ??
    options[0]
  return (
    <Menu.Root>
      <Menu.Trigger className="flex h-10 w-full items-center gap-2.5 ps-2 pe-2.5 text-start text-sm font-medium outline-none transition-colors hover:bg-fd-accent/60 focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-fd-ring data-popup-open:bg-fd-accent/60">
        {badge(selected.short)}
        <span className="flex min-w-0 flex-1 flex-col leading-tight">
          <span className="text-[10px] font-normal uppercase tracking-wide text-fd-muted-foreground">{kind === 'game' ? 'Game' : 'Renderer'}</span>
          <span className="truncate">{selected.title}</span>
        </span>
        <ChevronsUpDownIcon className="size-3.5 shrink-0 text-fd-muted-foreground" />
      </Menu.Trigger>
      <Menu.Portal>
        <Menu.Positioner align="start" sideOffset={6} alignOffset={0} positionMethod="fixed" className="z-50">
          <Menu.Popup className={cn(menuPopup, 'w-[max(var(--anchor-width),17rem)]')}>
            <Menu.Group>
              <Menu.GroupLabel className="px-2.5 pt-1.5 pb-1 text-xs font-medium text-fd-muted-foreground">
                {kind === 'game' ? 'Game' : 'Renderer'}
              </Menu.GroupLabel>
              {options.map((o) => (
                <Menu.LinkItem
                  key={o.id}
                  closeOnClick
                  render={<RouterLink to={variantUrl(kind, o) as '/'} />}
                  className={cn(menuItem, 'items-start py-2')}
                >
                  <span className="mt-0.5">{badge(o.short)}</span>
                  <span className="flex min-w-0 flex-1 flex-col gap-0.5">
                    <span className="flex items-center gap-1.5">
                      <span className="truncate font-medium">{o.title}</span>
                      {o === selected && <CheckIcon className="size-3.5! text-fd-primary" />}
                    </span>
                    <span className="truncate text-xs text-fd-muted-foreground">{o.description}</span>
                    <span className="mt-0.5 flex flex-wrap gap-1">
                      {statusTag(o)}
                      {o.tags?.map((t) => (
                        <span key={t} className="rounded-md border px-1.5 py-px text-[10px] text-fd-muted-foreground">
                          {t}
                        </span>
                      ))}
                    </span>
                  </span>
                </Menu.LinkItem>
              ))}
            </Menu.Group>
          </Menu.Popup>
        </Menu.Positioner>
      </Menu.Portal>
    </Menu.Root>
  )
}

/** Game and renderer: fixed under the sections on docs pages. */
function DocsPickers({ pathname }: { pathname: string }) {
  return (
    <motion.div
      key="docs-pickers"
      initial={{ opacity: 0, height: 0 }}
      animate={{ opacity: 1, height: 'auto' }}
      exit={{ opacity: 0, height: 0 }}
      transition={{ duration: 0.35, ease: EASE }}
      className="overflow-clip"
    >
      <div className="mt-3 flex flex-col divide-y overflow-clip rounded-xl border bg-fd-secondary/50">
        <Picker kind="game" options={GAMES} pathname={pathname} />
        <Picker kind="renderer" options={RENDERERS} pathname={pathname} />
      </div>
    </motion.div>
  )
}

// ---------------------------------------------------------------- the sidebar

/**
 * Only this part scrolls: the docs tree and the page's own panel. The sections and the
 * pickers above stay put, and the scroll goes back to the top when the section changes.
 */
function Viewport() {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const onDocs = pathname === '/hc' || pathname.startsWith('/hc/')
  const section = useSectionIndex(pathname)
  const viewport = useRef<HTMLDivElement>(null)
  useEffect(() => {
    viewport.current?.scrollTo({ top: 0 })
  }, [section])
  return (
    <ScrollArea.Root className="min-h-0 flex-1">
      <ScrollArea.Viewport
        ref={viewport}
        className="size-full overscroll-contain px-3 pt-4 pb-8 mask-t-from-[calc(100%-16px)] mask-b-from-[calc(100%-32px)]"
      >
        <ScrollArea.Content className="flex flex-col" data-glide-zone="">
          <AnimatePresence initial={false} mode="popLayout">
            {onDocs && (
              <motion.div
                key="docs-tree"
                initial={{ opacity: 0, y: 8 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0, y: 8 }}
                transition={{ duration: 0.35, ease: EASE }}
                className="flex flex-col"
              >
                <PageTree />
              </motion.div>
            )}
          </AnimatePresence>
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

/** The fixed part: portal sections, then (on docs pages) game and renderer. */
function Fixed() {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const onDocs = pathname === '/hc' || pathname.startsWith('/hc/')
  return (
    <div className="shrink-0 px-3 pt-3">
      <div data-glide-zone="">
        <Sections />
      </div>
      <AnimatePresence initial={false}>{onDocs && <DocsPickers pathname={pathname} />}</AnimatePresence>
      <div className="mt-3 h-px bg-fd-border/70" />
    </div>
  )
}

function Footer() {
  return (
    <div className="flex shrink-0 items-center gap-2 border-t border-fd-border/70 px-3 pt-2 pb-3" data-glide-zone="">
      <RouterLink to={'/about' as '/'} className={cn(sidebarItem(), 'w-auto flex-1')}>
        <InformationCircleIcon />
        About
      </RouterLink>
      <ThemeToggle />
    </div>
  )
}

function Top() {
  const { slots } = useSpaciousLayout()
  return (
    <div className="flex shrink-0 flex-col gap-2 px-3">
      {slots.searchTrigger && (
        <slots.searchTrigger.full hideIfDisabled className="h-10 gap-3 rounded-xl ps-[calc(--spacing(2.5)-1px)] pe-2 [&_svg]:shrink-0" />
      )}
    </div>
  )
}

/** The glide block: one per column, it follows the hovered row of any glide zone. */
const glideBlock = <div data-glide-block="" aria-hidden className={GLIDE_BLOCK_CLASS} />

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
        ref={follow}
        className={cn(
          'relative flex w-(--fd-sidebar-width) shrink-0 flex-col transition-[opacity,translate] duration-300 ease-[cubic-bezier(0.22,1,0.36,1)]',
          collapsed && '-translate-x-3 opacity-0'
        )}
      >
        {glideBlock}
        <div className="my-2 flex h-(--fd-header-height) shrink-0 items-center gap-2 ps-5.5 pe-3">
          <slots.navTitle className="me-auto inline-flex min-w-0 items-center gap-2 text-[0.9375rem] font-semibold" />
          <Base.SidebarCollapseTrigger className={cn(buttonVariants({ variant: 'ghost', size: 'icon-sm' }), 'size-8 text-fd-muted-foreground')}>
            <SidebarIcon />
          </Base.SidebarCollapseTrigger>
        </div>
        <Top />
        <Fixed />
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
        <div ref={follow} className="relative flex min-h-0 flex-1 flex-col">
          {glideBlock}
          <div className="flex h-(--fd-header-height) shrink-0 items-center gap-1.5 ps-5 pe-2.5">
            <slots.navTitle className="me-auto inline-flex min-w-0 items-center gap-2 font-semibold" />
            <Base.SidebarTrigger className={buttonVariants({ variant: 'ghost', size: 'icon-sm', className: 'p-2' })}>
              <SidebarIcon />
            </Base.SidebarTrigger>
          </div>
          <Top />
          <Fixed />
          <Viewport />
          <Footer />
        </div>
      </Base.SidebarDrawerContent>
    </Fragment>
  )
}

export function SidebarProvider(props: Base.SidebarProviderProps) {
  return <Base.SidebarProvider {...props} />
}

const follow = (area: HTMLDivElement | null) => glide(area, { zoned: true })

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
