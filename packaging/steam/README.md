# Adding TVDeck to Steam

TVDeck runs as a non-Steam game so it works in Game Mode on Steam Deck and
Steam Machine.

1. Switch to Desktop Mode and install TVDeck (see the main README).
2. Install Widevine once:
   `flatpak run --command=tvdeck-install-widevine io.github.sparatan117.TVDeck`
   (for a native build: `tvdeck-install-widevine`).
3. In Steam: **Games → Add a Non-Steam Game to My Library…**, pick TVDeck.
   The Flatpak shows up in the list like any other app.
4. Optional: add it a second time with launch option `--service netflix`
   to get a tile that jumps straight into Netflix.
5. Back in Game Mode, open the shortcut's **Controller settings** and pick
   the **Gamepad** template. TVDeck reads the virtual gamepad Steam Input
   exposes; the "Web browser" / mouse-and-keyboard templates will fight it.

Steam's own buttons (Steam, Quick Access) stay with Steam. Back grips
(L4/L5/R4/R5) can be mapped to any gamepad button in the Steam layout and
then bound in `~/.config/tvdeck/bindings.json`.
