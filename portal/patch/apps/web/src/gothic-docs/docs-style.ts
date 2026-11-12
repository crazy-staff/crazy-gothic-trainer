/** Docs look (inspired by elysiajs.com): roomy text, clear headings, framed code, callouts, cards. */
export const DOCS_CSS = `
.gothic-docs .gd-content{font-size:15.5px;line-height:1.75}
.gothic-docs .gd-content .prose{font-size:inherit;line-height:inherit;max-width:none}
.gothic-docs .gd-content .prose :where(h2){font-size:1.5rem;letter-spacing:-.01em;margin-top:2.6em;margin-bottom:.8em;padding-top:1.4em;border-top:1px solid var(--border)}
.gothic-docs .gd-content .prose :where(h2):first-child{margin-top:0;padding-top:0;border-top:0}
.gothic-docs .gd-content .prose :where(h3){font-size:1.15rem;margin-top:2em;margin-bottom:.6em}
.gothic-docs .gd-content .prose :where(p,li){color:color-mix(in oklab,var(--foreground) 88%,transparent)}
.gothic-docs .gd-content .prose :where(strong){color:var(--foreground)}
.gothic-docs .gd-content .prose :where(a){color:var(--primary);text-decoration:none;font-weight:500;border-bottom:1px solid color-mix(in oklab,var(--primary) 35%,transparent)}
.gothic-docs .gd-content .prose :where(a):hover{border-bottom-color:var(--primary)}
.gothic-docs .prose :where(code)::before,.gothic-docs .prose :where(code)::after{content:none}
.gothic-docs .prose :where(code):not(:where(pre *)){background:var(--muted);border:1px solid var(--border);border-radius:6px;padding:.12em .4em;font-weight:500;font-size:.85em;color:var(--foreground)}
.gothic-docs .gd-content .prose :where(pre){background:color-mix(in oklab,var(--card) 70%,black);border:1px solid var(--border);border-radius:12px;padding:1rem 1.15rem;font-size:13px;line-height:1.6}
.gothic-docs .gd-content .prose :where(table){display:table;width:100%;border-collapse:separate;border-spacing:0;border:1px solid var(--border);border-radius:12px;overflow:hidden;font-size:14px}
.gothic-docs .gd-content .prose :where(th){background:var(--muted);font-weight:600;text-align:left;padding:.6rem .9rem;border-bottom:1px solid var(--border)}
.gothic-docs .gd-content .prose :where(td){padding:.6rem .9rem;border-top:1px solid var(--border);vertical-align:top}
.gothic-docs .gd-content .prose :where(tr:first-child td){border-top:0}
.gothic-docs .gd-content .prose :where(th p,td p){margin:0}
.gothic-docs .gd-content .prose :where(blockquote){border-left:3px solid var(--primary);background:color-mix(in oklab,var(--primary) 6%,transparent);border-radius:0 10px 10px 0;padding:.6rem 1rem;font-style:normal}
.gothic-docs .gd-content .prose :where(ol>li)::marker{color:var(--primary);font-weight:600}
.gothic-docs .gd-content .prose :where(ul>li)::marker{color:var(--primary)}
.gothic-docs .gd-content .prose :where(hr){border-color:var(--border);margin:2.5em 0}
.gothic-docs .gd-callout{display:flex;gap:.75rem;margin:1.5rem 0;padding:.9rem 1rem;border-radius:12px;border:1px solid;font-size:14.5px;line-height:1.65}
.gothic-docs .gd-callout-title{font-weight:600;font-size:13px;letter-spacing:.02em;margin-bottom:.15rem}
.gothic-docs .gd-callout-body.prose{font-size:14.5px}
.gothic-docs .gd-callout-body.prose :where(p){margin:.25em 0}
.gothic-docs .gd-callout--note{border-color:color-mix(in oklab,#6bb3ff 35%,transparent);background:color-mix(in oklab,#6bb3ff 8%,transparent)}
.gothic-docs .gd-callout--note .gd-callout-icon,.gothic-docs .gd-callout--note .gd-callout-title{color:#6bb3ff}
.gothic-docs .gd-callout--tip{border-color:color-mix(in oklab,#73eb8c 35%,transparent);background:color-mix(in oklab,#73eb8c 7%,transparent)}
.gothic-docs .gd-callout--tip .gd-callout-icon,.gothic-docs .gd-callout--tip .gd-callout-title{color:#73eb8c}
.gothic-docs .gd-callout--warn{border-color:color-mix(in oklab,#fbcc4d 40%,transparent);background:color-mix(in oklab,#fbcc4d 8%,transparent)}
.gothic-docs .gd-callout--warn .gd-callout-icon,.gothic-docs .gd-callout--warn .gd-callout-title{color:#fbcc4d}
.gothic-docs .gd-media-box{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:.5rem;aspect-ratio:16/9;border:1px dashed color-mix(in oklab,var(--primary) 45%,var(--border));border-radius:14px;color:color-mix(in oklab,var(--primary) 75%,var(--muted-foreground));background:repeating-linear-gradient(135deg,transparent 0 14px,color-mix(in oklab,var(--primary) 5%,transparent) 14px 28px)}
.gothic-docs .gd-card{display:block;padding:1rem 1.1rem;border:1px solid var(--border);border-radius:12px;background:color-mix(in oklab,var(--card) 80%,transparent);transition:border-color .15s,background-color .15s}
.gothic-docs .gd-card:hover{border-color:color-mix(in oklab,var(--primary) 50%,var(--border));background:color-mix(in oklab,var(--primary) 5%,var(--card))}
.portal-nav a[href$="/docs"],.portal-nav a[href$="/hc"]{}
`
