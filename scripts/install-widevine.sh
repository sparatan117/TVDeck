#!/usr/bin/env bash
# Installs Google's Widevine CDM for Sofa.
#
# Widevine can't be redistributed, so Sofa never ships it. This script pulls
# the CDM out of Google's official Chrome package on your own machine, the
# same way Firefox and Chromium distributions do.
#
# Usage: sofa-install-widevine [--flatpak] [--uninstall]
set -euo pipefail

APP_ID="io.github.sofa.Sofa"
CHROME_DEB="https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb"

dest="${XDG_DATA_HOME:-$HOME/.local/share}/sofa/widevine"
uninstall=0
for arg in "$@"; do
  case "$arg" in
    --flatpak) dest="$HOME/.var/app/$APP_ID/data/sofa/widevine" ;;
    --uninstall) uninstall=1 ;;
    -h|--help) sed -n '2,9p' "$0"; exit 0 ;;
    *) echo "Unknown option: $arg" >&2; exit 2 ;;
  esac
done

if [[ $uninstall == 1 ]]; then
  rm -rf "$dest"
  echo "Removed $dest"
  exit 0
fi

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "Only x86_64 is supported (Steam Deck / Steam Machine)." >&2
  exit 1
fi

cat <<'NOTICE'
Widevine is proprietary software from Google. By continuing you download it
from Google and accept the Google Chrome Terms of Service:
  https://www.google.com/chrome/terms/
NOTICE
if [[ -t 0 ]]; then
  read -r -p "Continue? [y/N] " answer
  [[ "$answer" =~ ^[Yy]$ ]] || exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

echo "Downloading Chrome package..."
curl -fL --progress-bar -o "$work/chrome.deb" "$CHROME_DEB"

echo "Extracting Widevine..."
# SteamOS ships libarchive's bsdtar (pacman uses it) but not always binutils.
if command -v bsdtar >/dev/null; then
  bsdtar -xOf "$work/chrome.deb" 'data.tar.*' | bsdtar -xf - -C "$work" ./opt/google/chrome/WidevineCdm
elif command -v ar >/dev/null; then
  (cd "$work" && ar x chrome.deb && tar -xf data.tar.* ./opt/google/chrome/WidevineCdm)
else
  echo "Need either bsdtar or ar to unpack the package." >&2
  exit 1
fi

src="$work/opt/google/chrome/WidevineCdm"
if [[ ! -f "$src/_platform_specific/linux_x64/libwidevinecdm.so" ]]; then
  echo "Widevine wasn't where expected in the Chrome package; layout may have changed." >&2
  exit 1
fi

rm -rf "$dest"
mkdir -p "$(dirname "$dest")"
cp -r "$src" "$dest"
version="$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' "$dest/manifest.json" | head -1)"
echo "Installed Widevine ${version:-?} to $dest"
echo "Restart Sofa to pick it up."
