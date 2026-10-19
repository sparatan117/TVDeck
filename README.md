# TVDeck

A Streaming Deck Experience for your SteamOS Machine.

A controller-first launcher for streaming services on **Steam Deck** and
**Steam Machine**, in the spirit of ES-DE: a 10-foot carousel of services,
each one opened in a lightweight Chromium view and driven entirely from the
gamepad.

TVDeck is a web wrapper. It doesn't use private service APIs to fetch content;
it loads the service's normal website, then adds:

- **D-pad navigation** of the site (focus ring + geometric "spatial nav")
- **Player shortcuts** on the controller: play/pause, ±10s/±30s seek,
  skip intro, next episode, subtitles, volume
- **A right-stick cursor** for anything spatial nav can't reach
- **Steam on-screen keyboard** when a text field is selected
- **Battery-conscious behavior** (see [docs/POWER.md](docs/POWER.md))

Netflix is the first and only service for now.

> TVDeck is not affiliated with or endorsed by Netflix or Valve. You need your
> own Netflix account. Service names are used only to identify them; no
> logos are shipped.

## Default controls

| Button | Launcher | Browsing a service | Watching |
|---|---|---|---|
| D-pad / left stick | Move | Move focus | ◀▶ seek ∓10s, ▲▼ volume |
| A | Open | Select (or click at cursor) | Play / pause |
| B | Quit | Back | Exit player |
| Hold B | — | Home (back to TVDeck) | Home |
| X | — | Keyboard | Cycle subtitles |
| Y | — | Search | Skip intro / recap |
| L1 / R1 | — | Page up / down | Seek ∓30s |
| R2 | — | — | Next episode |
| Start | — | Show controls | Show controls |
| Right stick | — | Cursor | Cursor |

Rebind anything by copying [`config/bindings.json`](config/bindings.json) to
`~/.config/tvdeck/bindings.json` and editing it; entries there override the
defaults, and `null` unbinds a button.

## Building

Requirements: CMake ≥ 3.21, a C++17 compiler, Qt ≥ 6.4 (Core, Gui, Qml,
Quick, DBus, WebEngineQuick) and SDL2.

**Use Qt 6.8 or newer for real use.** Netflix checks the Chromium version,
and Qt 6.4 ships Chromium 102. Qt WebEngine must also be built with
proprietary codecs (H.264/AAC) — most distro builds are; the qt.io binaries
are not.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tvdeck --windowed
```

Options: `--windowed`, `--service <id>` (skip the launcher).

### Widevine

Netflix video is DRM-protected. Widevine can't be redistributed, so install
it locally once:

```sh
scripts/install-widevine.sh            # native build
scripts/install-widevine.sh --flatpak  # Flatpak build
```

TVDeck also finds an existing Google Chrome or Chromium install, or a path in
`TVDECK_WIDEVINE_PATH`.

### Flatpak / Steam

See [`packaging/flatpak`](packaging/flatpak) and
[`packaging/steam/README.md`](packaging/steam/README.md) for adding TVDeck to
Steam and choosing the right controller layout.

## Known limitations

- **Resolution.** Browsers on Linux get Widevine L3, which Netflix typically
  caps at 720p. Fine on the Deck's 800p panel; noticeable on a TV.
- **Netflix's player API is undocumented.** TVDeck uses the same internal
  player object the Netflix site uses (seeking via `video.currentTime`
  triggers error M7375). If Netflix changes it, actions fall back to
  Netflix's own keyboard shortcuts, sent as real key events.
- **Steam on-screen keyboard** is opened via `steam://open/keyboard`, which
  only works in Game Mode. Steam+X always works as a fallback.
- **Controller input goes through SDL**, i.e. the virtual gamepad Steam
  Input provides. A native `ISteamInput` backend (action sets, Steam's
  binding UI, glyphs) needs a Steam AppID, so it waits for a Steam release.

## Project layout

```
src/input/       SDL2 gamepad, action router (bindings), right-stick pointer
src/services/    Service registry (services/*.json + user overrides)
src/platform/    Widevine lookup, sleep inhibit, Steam keyboard, key injection
qml/             Launcher, service view, overlays
services/        Service definitions
scripts/         Page scripts injected into services, Widevine installer
config/          Default controller bindings
packaging/       Desktop entry, Flatpak manifest, Steam notes
dev/             Fixtures for working without a streaming account
```

How the pieces fit together: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## License

GPL-3.0 — see [LICENSE](LICENSE).
