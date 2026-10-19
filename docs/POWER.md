# Power and battery

Video playback dominates power draw, so most of the work is making sure
Sofa adds nothing on top of the browser and that the browser does as little
as possible.

## What Sofa does

| Area | Approach |
|---|---|
| Video decode | Chromium flags enable VA-API (`VaapiVideoDecoder`, `VaapiVideoDecodeLinuxGL`). Widevine L3 only decrypts; decode can still use the GPU. |
| Processes | One web view at a time; `--renderer-process-limit=1`; opening another service destroys the previous one. |
| Backgrounded service | On return to the launcher the video is paused, the view hidden, the page **frozen** (no JS, timers or rendering) and after `keepWarmMinutes` (default 10) **discarded**. |
| Launcher | Static QML. Qt Quick only renders when something changes; no looping animations, video backgrounds or per-second timers (the clock ticks every 30 s). |
| Input | SDL2 polled on an adaptive, coarse timer instead of SDL's wait loop (which busy-polls at ~1 kHz without a video subsystem). |
| Page scripts | Event-driven: no polling loops, no MutationObservers; the focus ring redraws on scroll/resize via `requestAnimationFrame`. Hover previews (autoplaying trailers) are never triggered by focus. |
| Screen | Sleep is inhibited only while video is actually playing. |

`keepWarmMinutes` lives in `~/.config/sofa/sofa.conf`:

```ini
[session]
keepWarmMinutes=10
```

## Verifying

Treat the Chromium flags in `src/main.cpp` as hypotheses until measured on
hardware:

1. **Is VA-API in use?** Run with
   `QTWEBENGINE_CHROMIUM_FLAGS="--enable-logging=stderr --v=1"` and look for
   `VaapiVideoDecoder` in the log, or check GPU video engine load with
   `radeontop` / `nvtop` while playing.
2. **Power draw.** On the Deck, Performance Overlay level 3+ shows battery
   watts. Compare 10 minutes of the same title in Sofa vs. a reference
   browser, with screen brightness fixed.
3. **Idle cost.** With the launcher showing, CPU should sit near 0%
   (`top -p $(pgrep sofa)`); with a frozen service, its renderer process
   should show no CPU at all.

Gamescope's frame limiter (Quick Access → Performance) also helps: 30 fps
is plenty for most video and halves compositor work.
