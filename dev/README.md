# Development fixtures

`nav-test.html` is a small page for working on `scripts/common/spatial-nav.js`
without a streaming account. Register it as a user service:

```sh
mkdir -p ~/.local/share/sofa/services
cat > ~/.local/share/sofa/services/navtest.json <<JSON
{ "id": "navtest", "name": "Nav test", "url": "file://$PWD/dev/nav-test.html",
  "scripts": ["common/bridge.js", "common/spatial-nav.js"] }
JSON
QT_LOGGING_RULES="sofa.page.debug=true" ./build/sofa --windowed --service navtest
```

Page `console.log` output and bridge messages appear under the `sofa.page`
logging category.

Without a controller, the launcher accepts arrows / Enter / Escape, and F10
returns from a service to the launcher.
