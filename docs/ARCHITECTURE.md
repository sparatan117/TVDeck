# Architecture

```
 SDL2 GameController ──► GamepadInput ──► ActionRouter ──► QML (Main.qml)
   (Steam Input's        buttons, sticks   bindings.json      │
    virtual gamepad)          │            per context        ├─► Launcher
                              ▼                               └─► ServiceView
                        VirtualPointer ── QMouseEvent ──►          │ runJavaScript
                                          (trusted)                ▼
                                                        window.__sofa.handle()
                                                          bridge.js
                                                          spatial-nav.js
                                                          <service>.js
                                                               │ console "__sofa:{...}"
                                                               ▼
                                                        context / playing / osd
```

## Input

`GamepadInput` polls SDL2 on an adaptive timer (8 ms while something is held
or deflected, 33 ms idle, 100 ms when unfocused). It emits SDL button names
(`a`, `dpleft`, `leftshoulder`, …), turns triggers into `lefttrigger` /
`righttrigger` buttons, and makes the left stick act as a second d-pad
(`lsleft`…, which fall back to the `dp*` bindings).

`ActionRouter` maps buttons to actions using the bindings for the current
context:

- `launcher` — the carousel and dialogs
- `browse` — inside a service, navigating its UI
- `player` — a service reports video is on screen

Actions are strings, optionally with an argument: `activate`, `seek:+10`.
A button may have a tap and a `hold:<button>` binding; then the tap fires
on release. D-pad buttons auto-repeat.

## Services

A service is a JSON file:

```json
{
  "id": "netflix",
  "name": "Netflix",
  "url": "https://www.netflix.com/browse",
  "accent": "#E50914",
  "requiresDrm": true,
  "scripts": ["common/bridge.js", "common/spatial-nav.js", "netflix/netflix.js"],
  "keyFallbacks": { "play_pause": "Space", "seek:+10": "Right" }
}
```

Built-ins live in `services/`. Files in `~/.local/share/sofa/services/`
add services or replace a built-in with the same `id`; scripts are looked up
in `~/.local/share/sofa/scripts/` first, so a site change can be patched
without a rebuild.

Each service gets its own persistent `WebEngineProfile` (cookies, cache), so
logins survive restarts and services can't see each other's data.

## Page bridge

The scripts are concatenated and injected into the page's main world at
DocumentReady (main world, because the Netflix adapter needs the site's own
`window.netflix` object).

Native → page: `window.__sofa.handle(action, arg)`, whose return value tells
Sofa what to do next:

| Return | Meaning |
|---|---|
| `true` | handled |
| `'fallback'` | send the service's `keyFallbacks` key as a real key event |
| `'exit'` | go back to the launcher (e.g. B on the service's home page) |
| `'keyboard'` | a text field is focused; open Steam's keyboard |

Page → native: console messages prefixed `__sofa:` with a JSON body. Only
`context`, `playing` and `osd` are accepted, and the native side treats them
as untrusted (the site could print the same thing).

## Adding a service

1. Add `services/<id>.json`.
2. Start with just `bridge.js` + `spatial-nav.js`; many sites are usable as is.
3. Add `scripts/<id>/<id>.js` to set `S.detectContext` (so player bindings
   apply), tune `S.nav.exclude` / `S.nav.onEdge` for carousels, and implement
   player actions (`play_pause`, `seek`, `skip`, …). Return `'fallback'`
   whenever you can't act, and list keyboard equivalents in `keyFallbacks`.
4. Register the JSON and script in `CMakeLists.txt` (`qt_add_resources`).
