-- Makes the custom "Docs" tab (a nav link whose URL contains /docs) open in the SAME tab.
-- The admin UI always saves custom links as "open in new tab". Data only - Quackback updates keep it.
-- docker exec -i <db container> psql -U quackback -d quackback -v docs_url="https://<domain>/docs" < docs-link.sql
UPDATE settings
SET portal_config = jsonb_set(
  portal_config::jsonb, '{nav,items}',
  (SELECT jsonb_agg(CASE WHEN e->>'type' = 'link' AND e->>'url' LIKE '%/docs%'
                         THEN e || jsonb_build_object('url', :'docs_url', 'newTab', false)
                         ELSE e END ORDER BY ord)
   FROM jsonb_array_elements(portal_config::jsonb -> 'nav' -> 'items') WITH ORDINALITY AS t(e, ord))
)::text
WHERE portal_config IS NOT NULL AND portal_config::jsonb -> 'nav' -> 'items' IS NOT NULL;

SELECT jsonb_pretty(portal_config::jsonb -> 'nav') AS nav FROM settings;
