import { portalNav } from '@/lib/shared';

/**
 * A copy of the Quackback portal header (same sizes, spacing and colours), so the
 * docs feel like one more tab of the portal. Links go to the portal on the same domain.
 * Logo letter / name come from the portal's workspace name.
 */
export function PortalHeader() {
  return (
    <header className="qb-header">
      <div className="qb-wrap">
        <div className="qb-row">
          <a href="/" className="qb-logo">
            <span className="qb-logo-box">G</span>
            <span className="qb-logo-name">gothic-crazy-trainer</span>
          </a>
          <a className="qb-btn" href="/auth/login">Sign in</a>
        </div>
      </div>
      <div className="qb-navrow">
        <div className="qb-wrap">
          <nav className="qb-nav">
            {portalNav.map((i) => (
              <a key={i.href} href={i.href} className={i.active ? 'qb-tab qb-tab--active' : 'qb-tab'}>
                {i.label}
              </a>
            ))}
          </nav>
        </div>
      </div>
    </header>
  );
}

/** The faint grid + brand glow from the Quackback help-center hero. */
export function PortalGlow() {
  return (
    <div className="qb-glow" aria-hidden="true">
      <div className="qb-glow-grid" />
      <div className="qb-glow-light" />
    </div>
  );
}
