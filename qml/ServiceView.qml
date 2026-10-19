import QtQuick
import QtWebEngine
import Sofa.Native

// Hosts one streaming service in a Chromium view and translates controller
// actions into calls on the injected page bridge (scripts/common/bridge.js).
Item {
    id: root

    required property var service
    property bool active: false

    // Reported by the page bridge.
    property string pageContext: "browse"
    property bool playing: false

    signal exitRequested()
    signal osdRequested(string text)

    function handleAction(name, arg) {
        switch (name) {
        case "exit_to_launcher":
            exitRequested()
            return
        case "reload":
            web.reload()
            return
        case "activate":
            // A clicks wherever the right-stick cursor is while it's showing.
            if (Pointer.visible) {
                Pointer.click()
                return
            }
            break
        }

        const raw = arg ? name + ":" + arg : name
        const call = "window.__sofa ? window.__sofa.handle(" + JSON.stringify(name) + ","
                     + JSON.stringify(arg) + ") : 'fallback'"
        web.runJavaScript(call, function (result) {
            if (result === "exit") {
                exitRequested()
            } else if (result === "keyboard") {
                Platform.showKeyboard()
            } else if (result === "fallback") {
                const key = (service.keyFallbacks || {})[raw]
                if (key) {
                    web.forceActiveFocus()
                    Platform.sendKey(key)
                }
            }
        })
    }

    function onBridgeMessage(json) {
        let msg
        try { msg = JSON.parse(json) } catch (e) { return }
        // The page can forge these, so only accept a small, harmless set.
        console.debug(pageLog, "bridge:", json)
        switch (msg.type) {
        case "context":
            if (msg.data === "browse" || msg.data === "player")
                pageContext = msg.data
            break
        case "playing":
            playing = msg.data === true
            break
        case "osd":
            if (typeof msg.data === "string")
                osdRequested(msg.data.slice(0, 80))
            break
        }
    }

    // --- Battery: freeze the page while the launcher is showing, then drop
    // it entirely after a while. Frozen pages run no JS timers or rendering.
    onActiveChanged: {
        if (active) {
            freezeTimer.stop()
            discardTimer.stop()
            web.lifecycleState = WebEngineView.LifecycleState.Active
            web.forceActiveFocus()
        } else {
            web.runJavaScript("window.__sofa && window.__sofa.handle('pause')")
            playing = false
            // Let the pause land before freezing; a page can't be frozen
            // while it's still visible anyway.
            freezeTimer.start()
        }
    }

    Timer {
        id: freezeTimer
        interval: 1500
        onTriggered: {
            if (root.active)
                return
            if (Platform.keepWarmMinutes > 0) {
                web.lifecycleState = WebEngineView.LifecycleState.Frozen
                discardTimer.start()
            } else {
                web.lifecycleState = WebEngineView.LifecycleState.Discarded
            }
        }
    }

    Timer {
        id: discardTimer
        interval: Math.max(1, Platform.keepWarmMinutes) * 60 * 1000
        onTriggered: web.lifecycleState = WebEngineView.LifecycleState.Discarded
    }

    Timer {
        id: crashReloadTimer
        interval: 1000
        onTriggered: web.reload()
    }

    // Page console output: QT_LOGGING_RULES="sofa.page.debug=true"
    LoggingCategory {
        id: pageLog
        name: "sofa.page"
        defaultLogLevel: LoggingCategory.Warning
    }

    WebEngineProfile {
        id: profile
        // Separate cookie jar and cache per service.
        storageName: "sofa-" + root.service.id
        offTheRecord: false
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpCacheType: WebEngineProfile.DiskHttpCache
        httpCacheMaximumSize: 128 * 1024 * 1024
    }

    WebEngineView {
        id: web
        anchors.fill: parent
        profile: profile
        visible: root.active
        backgroundColor: "black"

        // Input arrives as JS clicks, which aren't user gestures; without this
        // Chromium's autoplay policy would block playback with sound.
        settings.playbackRequiresUserGesture: false
        settings.showScrollBars: false
        settings.javascriptCanOpenWindows: false
        settings.screenCaptureEnabled: false
        settings.focusOnNavigationEnabled: true

        onJavaScriptConsoleMessage: function (level, message, lineNumber, sourceId) {
            if (message.startsWith("__sofa:"))
                root.onBridgeMessage(message.slice(7))
            else
                console.debug(pageLog, message)
        }

        // The window is already fullscreen; just let the page believe it.
        onFullScreenRequested: function (request) { request.accept() }

        // Keep everything in this one view.
        onNewWindowRequested: function (request) { web.url = request.requestedUrl }

        onFeaturePermissionRequested: function (origin, feature) {
            web.grantFeaturePermission(origin, feature, false)
        }

        onLoadingChanged: function (request) {
            if (request.status === WebEngineView.LoadStartedStatus) {
                root.pageContext = "browse"
                root.playing = false
            }
        }

        onRenderProcessTerminated: function (status, exitCode) {
            if (status !== WebEngineView.NormalTerminationStatus)
                crashReloadTimer.start()
        }
    }

    Component.onCompleted: {
        // Present as plain Chrome; services gate features on the UA string.
        profile.httpUserAgent = profile.httpUserAgent.replace(/ ?QtWebEngine\/\S+/, "")

        const script = WebEngine.script()
        script.name = "sofa-" + service.id
        script.sourceCode = Services.scriptBundle(service.id)
        script.injectionPoint = WebEngineScript.DocumentReady
        script.worldId = WebEngineScript.MainWorld
        script.runsOnSubFrames = false
        web.userScripts.insert(script)

        web.url = service.url
    }
}
