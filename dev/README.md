# Development fixtures

`nav-test.html` is a small page for working on `scripts/common/spatial-nav.js`
without a streaming account. Register it as a user service:

```sh
mkdir -p ~/.local/share/tvdeck/services
cat > ~/.local/share/tvdeck/services/navtest.json <<JSON
{ "id": "navtest", "name": "Nav test", "url": "file://$PWD/dev/nav-test.html",
  "scripts": ["common/bridge.js", "common/spatial-nav.js"] }
JSON
QT_LOGGING_RULES="tvdeck.page.debug=true" ./build/tvdeck --windowed --service navtest
```

Page `console.log` output and bridge messages appear under the `tvdeck.page`
logging category.

Without a controller, the launcher accepts arrows / Enter / Escape, and F10
returns from a service to the launcher.
