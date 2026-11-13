import { createFileRoute } from '@tanstack/react-router'
import { z } from 'zod'
import { withApiKeyAuth } from '@/lib/server/domains/api/auth'
import { successResponse, badRequestResponse, handleDomainError } from '@/lib/server/domains/api/responses'
import { PERMISSIONS } from '@/lib/shared/permissions'

/**
 * GET  /api/v1/changelog/categories  list the changelog categories
 * POST /api/v1/changelog/categories  {"categories":[{"name","color"}]}: creates the missing
 *      ones (matched by name, case-insensitive) and updates the colour of existing ones
 */
const schema = z.object({
  categories: z
    .array(z.object({ name: z.string().min(1).max(50), color: z.string().regex(/^#[0-9a-fA-F]{6}$/).optional() }))
    .min(1)
    .max(30),
})

export const Route = createFileRoute('/api/v1/changelog/categories')({
  server: {
    handlers: {
      GET: async ({ request }) => {
        try {
          await withApiKeyAuth(request, { permission: PERMISSIONS.CHANGELOG_VIEW_DRAFT })
          const { listChangelogCategories } = await import('@/lib/server/domains/changelog/changelog-category.service')
          const list = await listChangelogCategories()
          return successResponse(list.map((c) => ({ id: c.id, name: c.name, color: c.color, position: c.position })))
        } catch (error) {
          return handleDomainError(error)
        }
      },
      POST: async ({ request }) => {
        try {
          await withApiKeyAuth(request, { permission: PERMISSIONS.CHANGELOG_MANAGE })
          const parsed = schema.safeParse(await request.json())
          if (!parsed.success) return badRequestResponse(parsed.error.issues[0]?.message ?? 'Invalid body')
          const svc = await import('@/lib/server/domains/changelog/changelog-category.service')
          const existing = await svc.listChangelogCategories()
          const out = []
          for (const c of parsed.data.categories) {
            const found = existing.find((e) => e.name.toLowerCase() === c.name.toLowerCase())
            if (!found) out.push(await svc.createChangelogCategory({ name: c.name, color: c.color }))
            else if (c.color && found.color !== c.color)
              out.push(await svc.updateChangelogCategory(found.id, { color: c.color } as never))
            else out.push(found)
          }
          return successResponse(out.map((c) => ({ id: c.id, name: c.name, color: c.color })))
        } catch (error) {
          return handleDomainError(error)
        }
      },
    },
  },
})
