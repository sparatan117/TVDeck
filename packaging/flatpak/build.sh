#!/usr/bin/env bash
# Builds TVDeck.flatpak locally (Linux desktop, or Steam Deck in Desktop Mode).
#
# Needs flatpak and flatpak-builder. On SteamOS, where the system is
# read-only, install the builder as a Flatpak instead:
#   flatpak install --user flathub org.flatpak.Builder
#
# Usage: packaging/flatpak/build.sh [--install]
set -euo pipefail

cd "$(dirname "$0")/../.."
APP_ID="io.github.sparatan117.TVDeck"
MANIFEST="packaging/flatpak/$APP_ID.yml"

if command -v flatpak-builder >/dev/null; then
  builder=(flatpak-builder)
else
  builder=(flatpak run org.flatpak.Builder)
fi

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

"${builder[@]}" --user --force-clean --install-deps-from=flathub \
  --repo=repo build-flatpak "$MANIFEST"

flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
  repo TVDeck.flatpak "$APP_ID"
echo "Built $(pwd)/TVDeck.flatpak"

if [[ "${1:-}" == "--install" ]]; then
  flatpak install --user -y TVDeck.flatpak
fi
