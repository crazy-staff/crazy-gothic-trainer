/**
 * Docs navigation: every public category with its published articles, in position order.
 * Feeds the sidebar, the docs home and the category pages.
 */
import { createServerFn } from '@tanstack/react-start'
import { z } from 'zod'
import type { DocsNavCategory } from './types'

export const getDocsNavFn = createServerFn({ method: 'GET' })
  .validator(z.object({ locale: z.string().optional() }))
  .handler(async ({ data }): Promise<DocsNavCategory[]> => {
    const { loadDocsNav } = await import('./nav-loader')
    return loadDocsNav(data.locale)
  })
