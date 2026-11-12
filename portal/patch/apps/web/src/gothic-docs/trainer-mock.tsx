/**
 * A small replica of a card from the trainer's in-game menu (ImGui), so docs can show what
 * an option looks like. Written in articles as a fenced block:
 *
 * ```trainer
 * {"path": "Character › Combat", "title": "Free aim", "rows": [
 *   {"t": "toggle", "label": "Free aim", "on": true, "note": "bow, crossbow, spells"},
 *   {"t": "slider", "label": "Camera distance", "value": 2.4, "min": 1, "max": 6, "unit": "m"},
 *   {"t": "select", "label": "Key", "value": "Mouse side button 4"},
 *   {"t": "tabs", "items": ["Aimbot", "Trigger bot"], "active": 0},
 *   {"t": "button", "label": "Max everything"},
 *   {"t": "checks", "items": [["Humans", true], ["Monsters", false]]}
 * ]}
 * ```
 */
export interface TrainerMockRow {
  t: 'toggle' | 'slider' | 'value' | 'select' | 'button' | 'tabs' | 'checks' | 'text'
  label?: string
  note?: string
  on?: boolean
  value?: number | string
  min?: number
  max?: number
  unit?: string
  items?: (string | [string, boolean])[]
  active?: number
}
export interface TrainerMockSpec {
  path?: string
  title?: string
  rows: TrainerMockRow[]
}

const C = {
  win: '#131319',
  card: '#1b1b24',
  card2: '#212129',
  text: '#ebebf5',
  dim: '#8c8ca1',
  faint: '#5c5c6e',
  acc: '#a88cff',
  on: '#73eb8c',
  track: '#383849',
  line: '#2b2b38',
}

function Toggle({ on }: { on?: boolean }) {
  return (
    <span style={{ width: 34, height: 18, borderRadius: 9, background: on ? C.on : C.track, position: 'relative', display: 'inline-block', flexShrink: 0 }}>
      <span style={{ position: 'absolute', top: 2, left: on ? 18 : 2, width: 14, height: 14, borderRadius: 7, background: on ? '#0e1a12' : C.dim }} />
    </span>
  )
}

function Slider({ value = 0, min = 0, max = 1, unit }: { value?: number; min?: number; max?: number; unit?: string }) {
  const p = Math.max(0, Math.min(1, (Number(value) - min) / (max - min || 1)))
  return (
    <span style={{ display: 'flex', alignItems: 'center', gap: 10, width: 190, flexShrink: 0 }}>
      <span style={{ flex: 1, height: 4, borderRadius: 2, background: C.track, position: 'relative' }}>
        <span style={{ position: 'absolute', inset: 0, width: `${p * 100}%`, borderRadius: 2, background: C.acc }} />
        <span style={{ position: 'absolute', top: -5, left: `calc(${p * 100}% - 7px)`, width: 14, height: 14, borderRadius: 7, background: C.text, boxShadow: `0 0 0 3px ${C.acc}55` }} />
      </span>
      <span style={{ minWidth: 44, textAlign: 'right', fontSize: 12, color: C.text, fontVariantNumeric: 'tabular-nums' }}>
        {value}
        {unit ? <span style={{ color: C.dim }}> {unit}</span> : null}
      </span>
    </span>
  )
}

export function TrainerMock({ spec }: { spec: TrainerMockSpec }) {
  return (
    <div className="gd-trainer not-prose" style={{ background: C.win, border: `1px solid ${C.acc}26`, borderRadius: 12, padding: 14, margin: '1.5rem 0', fontFamily: 'Inter, ui-sans-serif, system-ui, sans-serif' }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 10, fontSize: 11, color: C.dim, letterSpacing: 0.3 }}>
        <span style={{ display: 'inline-flex', gap: 4 }}>
          <span style={{ width: 8, height: 8, borderRadius: 4, background: '#ff6666' }} />
          <span style={{ width: 8, height: 8, borderRadius: 4, background: '#fbcc4d' }} />
          <span style={{ width: 8, height: 8, borderRadius: 4, background: C.on }} />
        </span>
        <span>Trainer menu{spec.path ? ` · ${spec.path}` : ''}</span>
      </div>
      <div style={{ background: C.card, borderRadius: 10, padding: '10px 12px' }}>
        {spec.title && <div style={{ fontSize: 13, fontWeight: 600, color: C.text, marginBottom: 6 }}>{spec.title}</div>}
        {spec.rows.map((r, i) => (
          <div key={i} style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '7px 0', borderTop: i ? `1px solid ${C.line}` : 'none', minHeight: 30 }}>
            {r.t === 'tabs' ? (
              <span style={{ display: 'flex', gap: 4, background: C.card2, borderRadius: 8, padding: 3 }}>
                {(r.items ?? []).map((it, k) => (
                  <span key={k} style={{ fontSize: 12, padding: '4px 10px', borderRadius: 6, background: k === (r.active ?? 0) ? C.acc : 'transparent', color: k === (r.active ?? 0) ? '#14111b' : C.dim, fontWeight: 500 }}>
                    {typeof it === 'string' ? it : it[0]}
                  </span>
                ))}
              </span>
            ) : r.t === 'button' ? (
              <span style={{ fontSize: 12, padding: '5px 12px', borderRadius: 7, background: `${C.acc}33`, color: C.acc, fontWeight: 600 }}>{r.label}</span>
            ) : r.t === 'checks' ? (
              <span style={{ display: 'flex', flexWrap: 'wrap', gap: '6px 14px' }}>
                {(r.items ?? []).map((it, k) => {
                  const [label, on] = typeof it === 'string' ? [it, true] : it
                  return (
                    <span key={k} style={{ display: 'inline-flex', alignItems: 'center', gap: 6, fontSize: 12, color: C.text }}>
                      <span style={{ width: 14, height: 14, borderRadius: 4, background: on ? C.acc : C.card2, border: `1px solid ${on ? C.acc : C.track}`, color: '#14111b', fontSize: 10, lineHeight: '12px', textAlign: 'center' }}>{on ? '✓' : ''}</span>
                      {label}
                    </span>
                  )
                })}
              </span>
            ) : (
              <>
                <span style={{ flex: 1, minWidth: 0 }}>
                  <span style={{ display: 'block', fontSize: 13, color: C.text }}>{r.label}</span>
                  {r.note && <span style={{ display: 'block', fontSize: 11, color: C.dim, marginTop: 1 }}>{r.note}</span>}
                </span>
                {r.t === 'toggle' && <Toggle on={r.on} />}
                {(r.t === 'slider' || r.t === 'value') && <Slider value={Number(r.value)} min={r.min} max={r.max} unit={r.unit} />}
                {r.t === 'select' && (
                  <span style={{ fontSize: 12, color: C.text, background: C.card2, border: `1px solid ${C.track}`, borderRadius: 7, padding: '4px 10px', minWidth: 150 }}>
                    {String(r.value ?? '')} <span style={{ color: C.dim, float: 'right', marginLeft: 8 }}>▾</span>
                  </span>
                )}
                {r.t === 'text' && <span style={{ fontSize: 12, color: C.dim }}>{String(r.value ?? '')}</span>}
              </>
            )}
          </div>
        ))}
      </div>
    </div>
  )
}
