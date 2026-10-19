# Adding StreamDeck to Steam

StreamDeck runs as a non-Steam game so it works in Game Mode on Steam Deck and
Steam Machine.

1. Switch to Desktop Mode and install StreamDeck (Flatpak or a local build).
2. Install Widevine once: `streamdeck-install-widevine`
   (Flatpak: `scripts/install-widevine.sh --flatpak`).
3. In Steam: **Games → Add a Non-Steam Game to My Library…**, pick StreamDeck.
4. Optional: add it a second time with launch option `--service netflix`
   to get a tile that jumps straight into Netflix.
5. Back in Game Mode, open the shortcut's **Controller settings** and pick
   the **Gamepad** template. StreamDeck reads the virtual gamepad Steam Input
   exposes; the "Web browser" / mouse-and-keyboard templates will fight it.

Steam's own buttons (Steam, Quick Access) stay with Steam. Back grips
(L4/L5/R4/R5) can be mapped to any gamepad button in the Steam layout and
then bound in `~/.config/streamdeck/bindings.json`.
