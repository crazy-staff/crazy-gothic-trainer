/**
 * /design-system (team only): every token and component of the app's look in one place, so a
 * change can be checked before it reaches real pages. Tokens come from theme.css and
 * fumadocs.css; components are Quackback's own UI kit and the docs components.
 */
import { useState, type ReactNode } from 'react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Textarea } from '@/components/ui/textarea'
import { Badge } from '@/components/ui/badge'
import { Switch } from '@/components/ui/switch'
import { Checkbox } from '@/components/ui/checkbox'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Callout } from '@fumadocs/base-ui/components/callout'
import { Card as DocCard, Cards as DocCards } from '@fumadocs/base-ui/components/card'
import { Step, Steps } from '@fumadocs/base-ui/components/steps'
import { Tab, Tabs as DocTabs } from '@fumadocs/base-ui/components/tabs'
import { Accordion, Accordions } from '@fumadocs/base-ui/components/accordion'
import { DynamicCodeBlock } from '@fumadocs/base-ui/components/dynamic-codeblock'
import { motion } from 'framer-motion'
import { BookOpenIcon, ChevronDownIcon, ChevronUpIcon, PlusIcon, SparklesIcon } from '@heroicons/react/20/solid'
import { FilterPill } from '@/components/public/feedback/filter-pill'
import { SidebarSlot } from './sidebar-slot'
import { sidebarItem, sidebarLabel } from './sidebar'
import { Keys, StatusBadge } from './content'
import { TrainerMock } from './trainer-mock'
import { EASE } from './page-transition'

const SECTIONS = ['Colours', 'Typography', 'Spacing', 'Radius & elevation', 'Navigation', 'Buttons', 'Forms', 'Filters & voting', 'Badges', 'Cards', 'Tabs', 'Docs components', 'Motion']
const slug = (s: string) => s.toLowerCase().replace(/[^a-z]+/g, '-')

function Section({ title, children }: { title: string; children: ReactNode }) {
  return (
    <section id={slug(title)} className="scroll-mt-20 border-t border-fd-border py-10 first:border-0 first:pt-0">
      <h2 className="mb-1 text-xl font-semibold tracking-tight">{title}</h2>
      <div className="mt-5 space-y-6">{children}</div>
    </section>
  )
}

function Row({ label, children }: { label: string; children: ReactNode }) {
  return (
    <div className="grid gap-3 sm:grid-cols-[10rem_1fr] sm:items-center">
      <p className="text-sm text-fd-muted-foreground">{label}</p>
      <div className="flex flex-wrap items-center gap-3">{children}</div>
    </div>
  )
}

const COLOURS: [string, string][] = [
  ['Frame', '--color-fd-card'],
  ['Page', '--color-fd-background'],
  ['Popover', '--color-fd-popover'],
  ['Secondary', '--color-fd-secondary'],
  ['Muted', '--color-fd-muted'],
  ['Accent (hover)', '--color-fd-accent'],
  ['Border', '--color-fd-border'],
  ['Foreground', '--color-fd-foreground'],
  ['Muted text', '--color-fd-muted-foreground'],
  ['Primary', '--color-fd-primary'],
  ['Info', '--color-fd-info'],
  ['Success', '--color-fd-success'],
  ['Warning', '--color-fd-warning'],
  ['Error', '--color-fd-error'],
  ['Idea', '--color-fd-idea'],
]
const CHARTS = ['--chart-1', '--chart-2', '--chart-3', '--chart-4', '--chart-5', '--metric-posts', '--metric-votes', '--metric-users']

function Swatch({ name, token }: { name: string; token: string }) {
  return (
    <div className="flex items-center gap-3">
      <span className="size-10 shrink-0 rounded-lg border border-fd-border" style={{ background: `var(${token})` }} />
      <span className="min-w-0">
        <span className="block text-sm font-medium">{name}</span>
        <code className="block truncate text-xs text-fd-muted-foreground">{token}</code>
      </span>
    </div>
  )
}

export function DesignSystemPage() {
  const [replay, setReplay] = useState(0)
  return (
    <div className="mx-auto w-full max-w-5xl px-4 py-8 sm:px-6">
      <p className="text-sm font-medium text-fd-primary">Team only</p>
      <h1 className="mt-1 text-3xl font-semibold tracking-tight">Design system</h1>
      <p className="mt-2 max-w-2xl text-fd-muted-foreground">
        The look of the portal, the docs and the admin: Fumadocs' neutral theme with the purple of fumadocs.dev.
        Tokens live in <code className="docs-code">custom-docs/theme.css</code> and <code className="docs-code">fumadocs.css</code>.
      </p>
      <SidebarSlot>
        <nav className="flex flex-col">
          <p className={sidebarLabel}>Design system</p>
          {SECTIONS.map((s) => (
            <a key={s} href={`#${slug(s)}`} className={sidebarItem()}>
              {s}
            </a>
          ))}
        </nav>
      </SidebarSlot>

      <div className="mt-10">
        <Section title="Colours">
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
            {COLOURS.map(([n, t]) => (
              <Swatch key={t} name={n} token={t} />
            ))}
          </div>
          <p className="text-sm font-medium">Charts and metrics</p>
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
            {CHARTS.map((t) => (
              <Swatch key={t} name={t.replace('--', '')} token={t} />
            ))}
          </div>
        </Section>

        <Section title="Typography">
          <div className="space-y-3">
            <p className="text-3xl font-semibold tracking-tight">Page title · 30/600</p>
            <p className="text-2xl font-semibold tracking-tight">Heading 2 · 24/600</p>
            <p className="text-xl font-semibold">Heading 3 · 20/600</p>
            <p className="text-base font-semibold">Heading 4 · 16/600</p>
            <p className="text-lg text-fd-muted-foreground">Lead · 18, muted — the line under a page title.</p>
            <p className="text-[15.5px] leading-7">Body · 15.5/28 — articles and long text read at this size and line height.</p>
            <p className="text-sm">Small · 14 — interface text, menus, buttons, tables.</p>
            <p className="text-xs text-fd-muted-foreground">Caption · 12, muted — labels above groups, counts, dates.</p>
            <p className="font-mono text-sm">Mono · 14 — code, file names, keys.</p>
          </div>
        </Section>

        <Section title="Spacing">
          <p className="text-sm text-fd-muted-foreground">4 px grid. Inside controls 8–12, between related items 8–16, between sections 40.</p>
          <div className="space-y-2">
            {[1, 2, 3, 4, 6, 8, 10, 12, 16].map((n) => (
              <div key={n} className="flex items-center gap-3">
                <code className="w-16 text-xs text-fd-muted-foreground">{n * 4}px</code>
                <span className="h-3 rounded bg-fd-primary/60" style={{ width: n * 4 * 2 }} />
                <span className="text-xs text-fd-muted-foreground">spacing-{n}</span>
              </div>
            ))}
          </div>
        </Section>

        <Section title="Radius & elevation">
          <div className="flex flex-wrap gap-4">
            {[
              ['Controls 8', '0.5rem'],
              ['Cards 12', '0.75rem'],
              ['Panels, menus 16', '1rem'],
              ['Avatars, dots', '999px'],
            ].map(([n, r]) => (
              <div key={n} className="flex size-24 items-center justify-center border border-fd-border bg-fd-card text-xs text-fd-muted-foreground" style={{ borderRadius: r }}>
                {n}
              </div>
            ))}
          </div>
          <div className="flex flex-wrap gap-4">
            <div className="rounded-xl border border-fd-border bg-fd-card px-5 py-4 text-sm">Flat — border only</div>
            <div className="rounded-xl border border-fd-border bg-fd-card px-5 py-4 text-sm shadow-sm">Raised — shadow-sm</div>
            <div className="rounded-2xl border border-fd-border bg-fd-popover px-5 py-4 text-sm shadow-lg">Floating — popovers, menus</div>
          </div>
        </Section>

        <Section title="Navigation">
          <p className="text-sm text-fd-muted-foreground">
            One row style for every list you pick from: sidebar sections, docs pages, filters, settings menus and the
            admin rail. 36 px, 14 px text, 8 px corners; a soft block glides under the pointer, the current row is tinted.
          </p>
          <div className="w-64 rounded-2xl border border-fd-border bg-fd-card p-3">
            <p className={sidebarLabel}>Section</p>
            <span className={sidebarItem(true)}>
              <BookOpenIcon /> Current page
            </span>
            <span className={sidebarItem()}>
              <SparklesIcon /> Another page
            </span>
            <span className={sidebarItem()}>
              <PlusIcon /> One more
            </span>
          </div>
        </Section>

        <Section title="Buttons">
          <Row label="Variants">
            <Button>Primary</Button>
            <Button variant="secondary">Secondary</Button>
            <Button variant="outline">Outline</Button>
            <Button variant="ghost">Ghost</Button>
            <Button variant="link">Link</Button>
            <Button variant="destructive">Delete</Button>
            <Button variant="outline-destructive">Danger zone</Button>
          </Row>
          <Row label="Sizes">
            <Button size="sm">Small</Button>
            <Button>Default</Button>
            <Button size="lg">Large</Button>
            <Button size="icon" variant="outline" aria-label="Add">
              <PlusIcon />
            </Button>
          </Row>
          <Row label="With icon / disabled">
            <Button>
              <SparklesIcon /> New idea
            </Button>
            <Button disabled>Disabled</Button>
          </Row>
        </Section>

        <Section title="Forms">
          <div className="grid max-w-xl gap-5">
            <div>
              <label htmlFor="ds-name" className="text-sm font-medium">
                Full name
              </label>
              <Input id="ds-name" placeholder="Diego" />
            </div>
            <div>
              <label htmlFor="ds-text" className="text-sm font-medium">
                Description
              </label>
              <Textarea id="ds-text" placeholder="What happened?" />
            </div>
            <div className="flex items-center gap-6">
              <label className="flex items-center gap-2 text-sm">
                <Switch defaultChecked /> Notifications
              </label>
              <label className="flex items-center gap-2 text-sm">
                <Checkbox defaultChecked /> I tested this
              </label>
            </div>
          </div>
        </Section>

        <Section title="Filters & voting">
          <Row label="Filter chips">
            <FilterPill pressed={false} onClick={() => {}} color="#6bb3ff" count="12">
              Open
            </FilterPill>
            <FilterPill pressed onClick={() => {}} color="#a88cff" count="4">
              Planned
            </FilterPill>
          </Row>
          <Row label="Up and down">
            <div className="flex flex-col items-center gap-1">
              <span className="flex w-12 flex-col items-center gap-0.5 rounded-lg border border-fd-primary/50 bg-fd-primary/15 py-2 text-fd-primary">
                <ChevronUpIcon className="size-4" />
                <span className="text-sm font-semibold">12</span>
              </span>
              <span className="flex h-7 w-12 items-center justify-center rounded-lg border border-fd-border bg-fd-muted/40 text-fd-muted-foreground">
                <ChevronDownIcon className="size-4" />
              </span>
            </div>
            <p className="max-w-sm text-sm text-fd-muted-foreground">
              Signed-in users vote ideas up or down. A downvote takes one point off the score, so weak ideas sink instead
              of sitting at zero.
            </p>
          </Row>
        </Section>

        <Section title="Badges">
          <Row label="Quackback">
            <Badge>Default</Badge>
            <Badge variant="secondary">Secondary</Badge>
            <Badge variant="outline">Outline</Badge>
            <Badge variant="success">Success</Badge>
            <Badge variant="warning">Warning</Badge>
            <Badge variant="destructive">Destructive</Badge>
          </Row>
          <Row label="Status flags">
            {['OK', 'BUG', 'CRASH', 'TEST', 'NEW'].map((s) => (
              <StatusBadge key={s} value={s} />
            ))}
          </Row>
          <Row label="Keys">
            <Keys value="F1" />
            <Keys value="Ctrl+K" />
            <Keys value="Shift+F5" />
          </Row>
        </Section>

        <Section title="Cards">
          <div className="grid gap-4 sm:grid-cols-2">
            <Card>
              <CardHeader>
                <CardTitle>Quackback card</CardTitle>
                <CardDescription>Settings, admin panels, forms.</CardDescription>
              </CardHeader>
              <CardContent className="text-sm text-fd-muted-foreground">Content sits 24 px from the edges.</CardContent>
            </Card>
            <DocCards>
              <DocCard icon={<BookOpenIcon />} title="Docs card" description="Links between pages and topics." href="/hc" />
            </DocCards>
          </div>
        </Section>

        <Section title="Tabs">
          <Tabs defaultValue="a">
            <TabsList>
              <TabsTrigger value="a">Overview</TabsTrigger>
              <TabsTrigger value="b">Activity</TabsTrigger>
              <TabsTrigger value="c">Settings</TabsTrigger>
            </TabsList>
            <TabsContent value="a" className="pt-3 text-sm text-fd-muted-foreground">
              Quackback tabs (admin, settings).
            </TabsContent>
            <TabsContent value="b" className="pt-3 text-sm text-fd-muted-foreground">
              Activity
            </TabsContent>
            <TabsContent value="c" className="pt-3 text-sm text-fd-muted-foreground">
              Settings
            </TabsContent>
          </Tabs>
          <DocTabs items={['Steam', 'GOG']}>
            <Tab value="Steam">Docs tabs, for alternatives in an article.</Tab>
            <Tab value="GOG">Second tab.</Tab>
          </DocTabs>
        </Section>

        <Section title="Docs components">
          <Callout type="info" title="Note">
            Callouts: note, tip, warning, danger, success.
          </Callout>
          <Callout type="warn" title="Warning">
            Something that can go wrong.
          </Callout>
          <Steps>
            <Step>
              <p className="font-semibold">Step one</p>
              <p className="text-sm text-fd-muted-foreground">Numbered instructions.</p>
            </Step>
            <Step>
              <p className="font-semibold">Step two</p>
              <p className="text-sm text-fd-muted-foreground">With a line between them.</p>
            </Step>
          </Steps>
          <Accordions>
            <Accordion title="An accordion">Questions and answers.</Accordion>
          </Accordions>
          <DynamicCodeBlock lang="ini" code={'[Menu]\nHotkey=F1\nScale=1.0'} />
          <TrainerMock
            spec={{
              path: 'Character › Cheats',
              title: 'Cheats',
              rows: [
                { t: 'toggle', label: 'Immortality', on: true },
                { t: 'slider', label: 'Damage multiplier', value: 5, min: 1, max: 50 },
              ],
            }}
          />
        </Section>

        <Section title="Motion">
          <p className="text-sm text-fd-muted-foreground">
            One easing for everything, <code className="docs-code">cubic-bezier(0.22, 1, 0.36, 1)</code>: 150 ms for hovers and menus,
            300 ms for pages and panels. Pages slide in from the side of the link that opened them.
          </p>
          <Button variant="outline" onClick={() => setReplay((n) => n + 1)}>
            Replay
          </Button>
          <div className="flex gap-3">
            {[-1, 0, 1].map((d) => (
              <motion.div
                key={`${replay}-${d}`}
                initial={{ opacity: 0, x: d * 24, y: d === 0 ? 6 : 0 }}
                animate={{ opacity: 1, x: 0, y: 0 }}
                transition={{ duration: 0.3, ease: EASE }}
                className="flex h-20 flex-1 items-center justify-center rounded-xl border border-fd-border bg-fd-card text-sm text-fd-muted-foreground"
              >
                {d === -1 ? '← from the left' : d === 1 ? 'from the right →' : 'fade up'}
              </motion.div>
            ))}
          </div>
        </Section>
      </div>
    </div>
  )
}
