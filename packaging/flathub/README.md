# Submitting TVDeck to Flathub

Files here are copied into the Flathub submission:

- `io.github.sparatan117.TVDeck.yml` – manifest building a tagged release
- `flathub.json` – builds x86_64 only (Widevine has no Linux ARM build)

## Before submitting

- [ ] Tested on a Steam Deck: Netflix plays, controller works end to end
- [ ] Screenshots added to the metainfo (see the TODO in
      `packaging/io.github.sparatan117.TVDeck.metainfo.xml`)
- [ ] Release published (Actions → Flatpak → Run workflow with a version),
      then `git fetch --tags && packaging/flathub/set-release.sh <tag>` run
      and committed
- [ ] The "Flathub lint" job passes in GitHub Actions
- [ ] Read the current requirements, especially on website wrappers and
      proprietary downloads: https://docs.flathub.org/docs/for-app-authors/requirements

## Submitting

1. Fork https://github.com/flathub/flathub and create a branch from its
   `new-pr` branch (not `master`).
2. Copy both files from this directory into the root of that branch.
3. Open a pull request against `new-pr`, titled `Add io.github.sparatan117.TVDeck`.
   Explain why `--device=all` is needed (gamepads) and that Widevine is
   fetched by the user on demand, never bundled.
4. Comment `bot, build` on the PR to trigger a test build; reviewers take
   it from there.
5. After it's merged, sign in to flathub.org with GitHub to verify the
   `io.github.sparatan117` app ID.

Later updates go through pull requests to the
`flathub/io.github.sparatan117.TVDeck` repository Flathub creates.
