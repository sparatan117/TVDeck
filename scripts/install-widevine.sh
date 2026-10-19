#!/usr/bin/env bash
# Installs Google's Widevine CDM for TVDeck.
#
# Widevine can't be redistributed, so TVDeck never ships it. This script pulls
# the CDM out of Google's official Chrome package on your own machine, the
# same way Firefox and Chromium distributions do.
#
# Usage: tvdeck-install-widevine [--flatpak] [--uninstall]
#
# Flatpak users can also run it inside the sandbox, which installs to the
# right place without --flatpak:
#   flatpak run --command=tvdeck-install-widevine io.github.sparatan117.TVDeck
set -euo pipefail

APP_ID="io.github.sparatan117.TVDeck"
# Overridable for testing (any URL curl or Python can fetch, incl. file://).
CHROME_DEB="${TVDECK_CHROME_DEB_URL:-https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb}"

dest="${XDG_DATA_HOME:-$HOME/.local/share}/tvdeck/widevine"
uninstall=0
for arg in "$@"; do
  case "$arg" in
    --flatpak) dest="$HOME/.var/app/$APP_ID/data/tvdeck/widevine" ;;
    --uninstall) uninstall=1 ;;
    -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
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
if command -v curl >/dev/null; then
  curl -fL --progress-bar -o "$work/chrome.deb" "$CHROME_DEB"
else
  python3 -c 'import sys, urllib.request; urllib.request.urlretrieve(sys.argv[1], sys.argv[2])' \
    "$CHROME_DEB" "$work/chrome.deb"
fi

echo "Extracting Widevine..."
# SteamOS ships libarchive's bsdtar (pacman uses it) but not always binutils;
# the Flatpak runtime may have neither, but always has Python.
if command -v bsdtar >/dev/null; then
  bsdtar -xOf "$work/chrome.deb" 'data.tar.*' | bsdtar -xf - -C "$work" ./opt/google/chrome/WidevineCdm
elif command -v ar >/dev/null; then
  (cd "$work" && ar x chrome.deb && tar -xf data.tar.* ./opt/google/chrome/WidevineCdm)
else
  python3 - "$work/chrome.deb" "$work" <<'PY'
import io, sys, tarfile

deb, out = sys.argv[1], sys.argv[2]
prefix = './opt/google/chrome/WidevineCdm'
with open(deb, 'rb') as f:
    if f.read(8) != b'!<arch>\n':
        sys.exit('Not a .deb archive')
    while True:
        header = f.read(60)
        if len(header) < 60:
            sys.exit('No data.tar member in package')
        name = header[:16].decode().strip().rstrip('/')
        size = int(header[48:58])
        body = f.read(size)
        f.read(size % 2)  # members are 2-byte aligned
        if name.startswith('data.tar'):
            break

with tarfile.open(fileobj=io.BytesIO(body), mode='r:*') as tar:
    members = [m for m in tar.getmembers()
               if m.name == prefix or m.name.startswith(prefix + '/')]
    kwargs = {'filter': 'data'} if hasattr(tarfile, 'data_filter') else {}
    tar.extractall(out, members=members, **kwargs)
PY
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
echo "Restart TVDeck to pick it up."
