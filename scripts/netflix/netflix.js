// Netflix adapter.
//
// Playback goes through the player API the Netflix web app uses internally.
// It's undocumented and can change without notice, so every action returns
// 'fallback' when it's unavailable and TVDeck sends the equivalent Netflix
// keyboard shortcut instead (see keyFallbacks in services/netflix.json).
//
// Don't seek through video.currentTime: Netflix detects it and stops
// playback with error M7375.
(function () {
  'use strict';
  const S = window.__tvdeck;
  if (!S || S.netflix) return;
  const N = S.netflix = {};

  const isWatch = () => location.pathname.startsWith('/watch');
  const isHome = () => location.pathname === '/browse' || location.pathname === '/';

  N.player = function () {
    try {
      const vp = window.netflix.appContext.state.playerApp.getAPI().videoPlayer;
      const ids = vp.getAllPlayerSessionIds();
      const id = ids.find(i => i.startsWith('watch')) || ids[0];
      return id ? vp.getVideoPlayerBySessionId(id) : null;
    } catch (e) {
      return null;
    }
  };

  function clickFirst(selectors) {
    for (const sel of selectors) {
      const el = document.querySelector(sel);
      if (el) { el.click(); return true; }
    }
    return false;
  }

  // Player controls are only in the DOM while the mouse is "active".
  function revealControls() {
    const target = document.querySelector('.watch-video, [data-uia="player"]') || document.body;
    const r = target.getBoundingClientRect();
    target.dispatchEvent(new MouseEvent('mousemove', {
      bubbles: true, clientX: r.left + r.width / 2, clientY: r.top + r.height / 2,
    }));
  }

  // Like fmt() but with milliseconds, so single frames are visible.
  function fmtPrecise(ms) {
    return fmt(Math.floor(ms / 1000) * 1000) + '.' + String(Math.floor(ms % 1000)).padStart(3, '0');
  }

  function fmt(ms) {
    const t = Math.max(0, Math.round(ms / 1000));
    const h = Math.floor(t / 3600), m = Math.floor(t / 60) % 60, s = t % 60;
    const mm = h ? String(m).padStart(2, '0') : String(m);
    return (h ? h + ':' : '') + mm + ':' + String(s).padStart(2, '0');
  }

  S.detectContext = () => isWatch() ? 'player' : 'browse';

  // --- Browse UI ---
  S.nav.exclude = '.handle, .pagination-indicator, .slider-refocus[tabindex="-1"]';
  S.nav.onEdge = function (dir, cur) {
    if (dir !== 'left' && dir !== 'right') return false;
    const row = cur.closest('.lolomoRow, .rowContainer, .slider');
    const handle = row && row.querySelector(dir === 'right' ? '.handleNext' : '.handlePrev');
    if (!handle) return false;
    handle.click();
    // Wait for the slide animation, then step onto the newly revealed card.
    setTimeout(() => S.nav.move(dir), 700);
    return true;
  };

  S.on('search', () => {
    if (!clickFirst(['[data-uia="search-box-launcher"]', '.searchTab'])) {
      history.pushState({}, '', '/search');
      dispatchEvent(new PopStateEvent('popstate'));
    }
    setTimeout(() => {
      const input = document.querySelector('[data-uia="search-box-input"], input[name="searchInput"]');
      if (input) { S.nav.focus(input); input.focus(); }
    }, 150);
    return 'keyboard';
  });

  S.on('back', () => {
    if (isWatch()) {
      if (!clickFirst(['[data-uia="nfplayer-exit"]', '[data-uia="player-back-to-browse"]']))
        history.back();
      return true;
    }
    if (clickFirst(['[data-uia="previewModal-closebtn"]', '.previewModal-close',
                    '[role="dialog"] [aria-label*="close" i]']))
      return true;
    if (S.isTextInput(document.activeElement)) {
      document.activeElement.blur();
      return true;
    }
    if (isHome()) return 'exit';
    history.back();
    return true;
  });

  // --- Player ---
  let seekBase = null, seekDelta = 0, seekTimer = null;
  S.on('seek', arg => {
    const p = N.player();
    if (!p) return 'fallback';
    const delta = parseFloat(arg) * 1000;
    if (seekBase === null) seekBase = p.getCurrentTime();
    seekDelta += delta;
    const target = Math.max(0, Math.min(p.getDuration() - 2000, seekBase + seekDelta));
    S.osd((seekDelta < 0 ? '« ' : '» ') + fmt(target) + ' / ' + fmt(p.getDuration()));
    // Coalesce repeated presses into one seek so we don't thrash the buffer.
    clearTimeout(seekTimer);
    seekTimer = setTimeout(() => {
      p.seek(target);
      seekBase = null;
      seekDelta = 0;
    }, 300);
  });

  // Jog dial (right-stick rotation). Netflix doesn't expose the frame rate;
  // 23.976 fps covers most of its film and TV catalogue.
  const FRAME_MS = 1001 / 24;
  let frameTarget = null, frameSeekTimer = null, frameIdleTimer = null;
  S.on('frame', arg => {
    const p = N.player();
    if (!p) return 'fallback';
    if (!p.isPaused()) p.pause();
    // Keep our own position while the user is jogging: the player's
    // reported time lags behind back-to-back seeks.
    if (frameTarget === null) frameTarget = p.getCurrentTime();
    frameTarget = Math.max(0, Math.min(p.getDuration() - 2000,
                                       frameTarget + parseFloat(arg) * FRAME_MS));
    S.osd((parseFloat(arg) < 0 ? '◂ ' : '▸ ') + fmtPrecise(frameTarget));
    // Seek at most every 60 ms so a fast spin doesn't queue up dozens of
    // seeks, but still lands each notch visibly.
    if (!frameSeekTimer) {
      frameSeekTimer = setTimeout(() => {
        frameSeekTimer = null;
        p.seek(Math.round(frameTarget));
      }, 60);
    }
    clearTimeout(frameIdleTimer);
    frameIdleTimer = setTimeout(() => { frameTarget = null; }, 1500);
  });

  S.on('play_pause', () => {
    // "Are you still watching?" interrupts playback; A answers it.
    if (clickFirst(['[data-uia="interrupt-autoplay-continue"]'])) return true;
    const p = N.player();
    if (!p) return 'fallback';
    if (p.isPaused()) { p.play(); S.osd('Play'); }
    else { p.pause(); S.osd('Paused'); }
  });

  S.on('pause', () => {
    const p = N.player();
    if (p && !p.isPaused()) p.pause();
  });

  S.on('volume', arg => {
    const p = N.player();
    if (!p) return 'fallback';
    const v = Math.max(0, Math.min(1, p.getVolume() + parseFloat(arg)));
    p.setVolume(v);
    if (v > 0 && p.isMuted && p.isMuted()) p.setMuted(false);
    S.osd('Volume ' + Math.round(v * 100) + '%');
  });

  S.on('subtitles', () => {
    const p = N.player();
    if (!p) return 'fallback';
    const tracks = p.getTextTrackList() || [];
    if (!tracks.length) return true;
    const cur = p.getTextTrack();
    const i = tracks.findIndex(t => cur && t.trackId === cur.trackId);
    const next = tracks[(i + 1) % tracks.length];
    p.setTextTrack(next);
    S.osd('Subtitles: ' + (next.displayName || 'Off'));
  });

  S.on('skip', () => {
    const skipped = clickFirst([
      '[data-uia="player-skip-intro"]',
      '[data-uia="player-skip-recap"]',
      '[data-uia="player-skip-preplay"]',
      'button[data-uia^="player-skip"]',
      '[data-uia="next-episode-seamless-button"]',
    ]);
    return skipped ? true : 'fallback';
  });

  S.on('next_episode', () => {
    if (clickFirst(['[data-uia="next-episode-seamless-button"]'])) return true;
    revealControls();
    setTimeout(() => clickFirst(['[data-uia="control-next"]']), 120);
  });
})();
