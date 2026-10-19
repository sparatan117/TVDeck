// D-pad navigation for sites built for a mouse. Finds focusable elements,
// moves between them geometrically and draws a focus ring on top of the page.
//
// The ring is a fixed-position overlay rather than a style on the element so
// the site's own CSS can't hide it, and it needs no <style> tag (CSP-safe).
(function () {
  'use strict';
  const S = window.__streamdeck;
  if (!S || S.nav) return;

  const nav = S.nav = {
    selector: 'a[href], button, input, select, textarea, [role="button"], [role="link"], ' +
              '[role="tab"], [role="menuitem"], [role="option"], [tabindex]:not([tabindex="-1"])',
    // Services: elements that should never take focus.
    exclude: null,
    // Focus is confined to the top-most open dialog.
    modalSelector: '[role="dialog"], [aria-modal="true"]',
    // Services: (dir, current) => true when they handled hitting an edge
    // (e.g. paging a carousel).
    onEdge: null,
    current: null,
  };

  // --- Focus ring ---
  const ring = document.createElement('div');
  Object.assign(ring.style, {
    position: 'fixed', left: '0', top: '0', zIndex: '2147483647', pointerEvents: 'none',
    boxSizing: 'border-box', border: '3px solid #fff', borderRadius: '6px',
    boxShadow: '0 0 0 2px rgba(0,0,0,.55), 0 0 16px rgba(255,255,255,.35)',
    transition: 'transform 90ms ease-out, width 90ms ease-out, height 90ms ease-out',
    display: 'none',
  });
  let ringVisible = true;
  let rafPending = false;

  function drawRing() {
    rafPending = false;
    const el = nav.current;
    if (!ringVisible || !el || !el.isConnected) {
      ring.style.display = 'none';
      return;
    }
    if (!ring.isConnected) document.documentElement.appendChild(ring);
    const r = el.getBoundingClientRect();
    ring.style.display = 'block';
    ring.style.width = (r.width + 8) + 'px';
    ring.style.height = (r.height + 8) + 'px';
    ring.style.transform = `translate(${r.left - 4}px, ${r.top - 4}px)`;
  }
  function scheduleRing() {
    if (rafPending) return;
    rafPending = true;
    requestAnimationFrame(drawRing);
  }
  // Only redraw on events; no polling loop, so an idle page stays idle.
  window.addEventListener('scroll', scheduleRing, { passive: true, capture: true });
  window.addEventListener('resize', scheduleRing, { passive: true });

  nav.setVisible = function (visible) {
    ringVisible = visible;
    scheduleRing();
  };

  // --- Candidates ---
  function isVisible(el) {
    const r = el.getBoundingClientRect();
    if (r.width < 2 || r.height < 2) return false;
    if (el.checkVisibility)
      return el.checkVisibility({ checkOpacity: true, checkVisibilityCSS: true });
    const cs = getComputedStyle(el);
    return cs.visibility !== 'hidden' && cs.display !== 'none' && cs.opacity !== '0';
  }

  function scope() {
    const modals = Array.from(document.querySelectorAll(nav.modalSelector)).filter(isVisible);
    return modals.length ? modals[modals.length - 1] : document;
  }

  function candidates() {
    const root = scope();
    const all = Array.from(root.querySelectorAll(nav.selector)).filter(el =>
      !el.disabled && el.getAttribute('aria-hidden') !== 'true' &&
      !(nav.exclude && el.matches(nav.exclude)) && isVisible(el));
    const set = new Set(all);
    return all.filter(el => {
      // Carousels: anything not fully on screen horizontally is reached by
      // paging (onEdge), not by focusing it off-screen.
      const r = el.getBoundingClientRect();
      if (r.left < -8 || r.right > innerWidth + 8) return false;
      // Prefer the innermost of nested focusables (card link inside a row).
      for (const other of el.querySelectorAll(nav.selector))
        if (set.has(other)) return false;
      return true;
    });
  }

  function isCandidate(el) {
    return el && el.isConnected && candidates().includes(el);
  }

  // --- Movement ---
  function score(dir, from, to) {
    const fx = from.left + from.width / 2, fy = from.top + from.height / 2;
    const tx = to.left + to.width / 2, ty = to.top + to.height / 2;
    let ahead, gap, orthoGap, orthoCenter;
    switch (dir) {
      case 'right': ahead = tx > fx + 1; gap = to.left - from.right; break;
      case 'left':  ahead = tx < fx - 1; gap = from.left - to.right; break;
      case 'down':  ahead = ty > fy + 1; gap = to.top - from.bottom; break;
      case 'up':    ahead = ty < fy - 1; gap = from.top - to.bottom; break;
    }
    if (!ahead) return Infinity;
    if (dir === 'left' || dir === 'right') {
      orthoGap = Math.max(0, Math.max(from.top, to.top) - Math.min(from.bottom, to.bottom));
      orthoCenter = Math.abs(ty - fy);
    } else {
      orthoGap = Math.max(0, Math.max(from.left, to.left) - Math.min(from.right, to.right));
      orthoCenter = Math.abs(tx - fx);
    }
    // Strongly prefer staying in the same row/column.
    return Math.max(0, gap) + orthoGap * 4 + orthoCenter * 0.3;
  }

  function nearestToCenter(list) {
    const cx = innerWidth / 2, cy = innerHeight / 2;
    let best = null, bestD = Infinity;
    for (const el of list) {
      const r = el.getBoundingClientRect();
      if (r.bottom < 0 || r.top > innerHeight) continue;
      const d = Math.hypot(r.left + r.width / 2 - cx, (r.top + r.height / 2 - cy) * 1.5);
      if (d < bestD) { bestD = d; best = el; }
    }
    return best || list[0] || null;
  }

  nav.focus = function (el) {
    if (!el) return;
    nav.current = el;
    try { el.focus({ preventScroll: true }); } catch (e) { /* not focusable */ }
    // Scroll the window only. scrollIntoView would also scroll overflow:hidden
    // carousel tracks and desync the site's own slider state.
    const r = el.getBoundingClientRect();
    const margin = innerHeight * 0.2;
    if (r.top < margin || r.bottom > innerHeight - margin)
      window.scrollBy({ top: r.top + r.height / 2 - innerHeight / 2, behavior: 'smooth' });
    scheduleRing();
  };

  nav.move = function (dir) {
    const list = candidates();
    const cur = nav.current && list.includes(nav.current) ? nav.current : null;
    if (!cur) {
      nav.focus(nearestToCenter(list));
      return true;
    }
    const from = cur.getBoundingClientRect();
    let best = null, bestScore = Infinity;
    for (const el of list) {
      if (el === cur) continue;
      const s = score(dir, from, el.getBoundingClientRect());
      if (s < bestScore) { bestScore = s; best = el; }
    }
    if (best) {
      nav.focus(best);
    } else if (!(nav.onEdge && nav.onEdge(dir, cur))) {
      if (dir === 'up' || dir === 'down')
        window.scrollBy({ top: (dir === 'up' ? -0.5 : 0.5) * innerHeight, behavior: 'smooth' });
    }
    return true;
  };

  nav.refocus = function () {
    if (!isCandidate(nav.current)) nav.focus(nearestToCenter(candidates()));
    else scheduleRing();
  };

  nav.activate = function () {
    const el = nav.current;
    if (!el || !el.isConnected) {
      nav.refocus();
      return true;
    }
    if (S.isTextInput(el)) {
      el.focus();
      return 'keyboard';
    }
    el.click();
    // Clicking usually opens or replaces content; land somewhere sensible.
    setTimeout(nav.refocus, 500);
    return true;
  };

  nav.focusFirstInput = function () {
    const input = candidates().find(el => S.isTextInput(el));
    if (!input) return false;
    nav.focus(input);
    input.focus();
    return true;
  };

  // Keep the ring in sync when focus moves by other means (pointer, site JS).
  document.addEventListener('focusin', e => {
    if (e.target !== nav.current && e.target.matches && e.target.matches(nav.selector)) {
      nav.current = e.target;
      scheduleRing();
    }
  }, true);

  S.on('nav', dir => nav.move(dir));
  S.on('activate', () => nav.activate());
})();
