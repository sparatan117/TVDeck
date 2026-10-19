#!/usr/bin/env bash
# Points the Flathub manifest at a release tag: set-release.sh v0.1.0
# The tag must already exist and be pushed to GitHub.
set -euo pipefail

tag="${1:?usage: set-release.sh <tag>}"
cd "$(dirname "$0")"
commit="$(git rev-parse "$tag^{commit}")"
manifest="io.github.sparatan117.TVDeck.yml"

sed -i -E "s|^(        tag: ).*|\1$tag|; s|^(        commit: ).*|\1$commit|" "$manifest"
echo "$manifest now builds $tag ($commit)"
