// TVDeck page bridge. Injected into the service's main world at DocumentReady.
//
// Native -> page: TVDeck calls window.__tvdeck.handle(action, arg) and acts on
// the return value:
//   true        handled
//   'fallback'  not handled; TVDeck sends the service's keyFallbacks key instead
//   'exit'      leave the service and return to the launcher
//   'keyboard'  a text field has focus; open Steam's on-screen keyboard
//
// Page -> native: messages are console lines prefixed with "__tvdeck:". The
// page can forge these, so the native side only accepts a few harmless types.
(function () {
  'use strict';
  if (window.__tvdeck) return;

  const S = window.__tvdeck = {
    actions: Object.create(null),

    post(type, data) {
      console.info('__tvdeck:' + JSON.stringify({ type, data }));
    },
    osd(text) { S.post('osd', String(text)); },
    on(name, fn) { S.actions[name] = fn; },

    handle(name, arg) {
      const fn = S.actions[name];
      let result = 'fallback';
      if (fn) {
        try {
          const r = fn(arg);
          result = r === undefined ? true : r;
        } catch (e) {
          console.warn('tvdeck: action failed', name, e);
        }
      }
      // Belt and braces for SPA routers that bypass the history hooks below.
      setTimeout(S.updateContext, 400);
      return result;
    },

    isTextInput(el) {
      if (!el) return false;
      if (el.isContentEditable || el.tagName === 'TEXTAREA') return true;
      return el.tagName === 'INPUT' &&
        !/^(button|submit|checkbox|radio|range|reset|image|file|color)$/i.test(el.type || '');
    },

    // Services override this to report 'player' while video is up.
    detectContext() { return 'browse'; },
  };

  // --- Context tracking (drives which controller bindings are active) ---
  let lastContext = null;
  S.updateContext = function () {
    const ctx = S.detectContext();
    if (ctx !== lastContext) {
      lastContext = ctx;
      S.post('context', ctx);
      if (S.nav) S.nav.setVisible(ctx !== 'player');
    }
  };
  for (const method of ['pushState', 'replaceState']) {
    const orig = history[method];
    history[method] = function () {
      const r = orig.apply(this, arguments);
      queueMicrotask(S.updateContext);
      return r;
    };
  }
  window.addEventListener('popstate', S.updateContext);

  // --- Playback state (native side inhibits sleep while true) ---
  // Media events don't bubble, but a capturing listener still sees them.
  let lastPlaying = false;
  const onMedia = () => {
    const playing = Array.from(document.querySelectorAll('video'))
      .some(v => !v.paused && !v.ended);
    if (playing !== lastPlaying) {
      lastPlaying = playing;
      S.post('playing', playing);
    }
  };
  for (const t of ['play', 'playing', 'pause', 'ended', 'emptied'])
    document.addEventListener(t, onMedia, true);

  // --- Generic actions; services override what they know better ---
  S.on('page', dir => {
    window.scrollBy({ top: (dir === 'up' ? -0.8 : 0.8) * innerHeight, behavior: 'smooth' });
    if (S.nav) setTimeout(() => S.nav.refocus(), 450);
  });
  S.on('keyboard', () => {
    if (S.isTextInput(document.activeElement)) return 'keyboard';
    return S.nav && S.nav.focusFirstInput() ? 'keyboard' : true;
  });
  S.on('pause', () => {
    document.querySelectorAll('video').forEach(v => v.pause());
  });
  S.on('back', () => 'fallback');

  setTimeout(S.updateContext, 0);
})();
