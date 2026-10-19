import QtQuick
import QtQuick.Window
import TVDeck.Native
import "components"

Window {
    id: win
    width: 1280
    height: 800
    visible: true
    visibility: Platform.windowed ? Window.Windowed : Window.FullScreen
    color: "#08080b"
    title: "TVDeck"

    property var activeService: null
    readonly property bool inService: activeService !== null
    readonly property var session: sessionLoader.item
    readonly property bool shouldInhibit: inService && session !== null && session.playing

    onShouldInhibitChanged: Platform.setInhibitSleep(shouldInhibit)

    function openService(svc) {
        // One web engine at a time: opening another service tears down the
        // previous one instead of keeping both resident.
        if (!sessionLoader.item || sessionLoader.serviceId !== svc.id) {
            sessionLoader.serviceId = svc.id
            sessionLoader.setSource("ServiceView.qml", { service: svc })
        }
        activeService = svc
    }

    function closeService() {
        activeService = null
        launcher.forceActiveFocus()
    }

    Binding {
        target: Router
        property: "context"
        value: dialog.visible || !win.inService ? "launcher"
               : (win.session ? win.session.pageContext : "browse")
    }

    Binding {
        target: Pointer
        property: "enabled"
        value: win.inService && !dialog.visible && !Router.rightStickJog
    }

    Connections {
        target: Router
        function onAction(name, arg) {
            if (dialog.visible)
                dialog.handleAction(name, arg)
            else if (name === "help")
                hints.flash()
            else if (win.inService && win.session)
                win.session.handleAction(name, arg)
            else
                launcher.handleAction(name, arg)
        }
        function onContextChanged() {
            if (win.inService)
                hints.flash()
        }
    }

    Connections {
        target: Gamepad
        function onConnectedChanged() {
            osd.show(Gamepad.connected ? "Controller connected" : "Controller disconnected")
        }
    }

    Launcher {
        id: launcher
        anchors.fill: parent
        visible: !win.inService
        focus: !win.inService
        services: Services.services
        onLaunchRequested: (svc) => win.openService(svc)
        onQuitRequested: dialog.ask(qsTr("Quit TVDeck?"), () => Qt.quit())

        // Desktop testing without a controller.
        Keys.onPressed: (event) => win.forwardKey(event, true)
        Keys.onReleased: (event) => win.forwardKey(event, false)
    }

    readonly property var keyMap: ({
        [Qt.Key_Left]: "dpleft", [Qt.Key_Right]: "dpright",
        [Qt.Key_Up]: "dpup", [Qt.Key_Down]: "dpdown",
        [Qt.Key_Return]: "a", [Qt.Key_Enter]: "a",
        [Qt.Key_Escape]: "b", [Qt.Key_Backspace]: "b",
    })

    function forwardKey(event, pressed) {
        const button = keyMap[event.key]
        if (!button || event.isAutoRepeat)
            return
        Router.inject(button, pressed)
        event.accepted = true
    }

    Loader {
        id: sessionLoader
        property string serviceId
        anchors.fill: parent
        onLoaded: {
            item.active = Qt.binding(() => win.inService)
            item.exitRequested.connect(win.closeService)
            item.osdRequested.connect(osd.show)
        }
    }

    Shortcut {
        sequence: "F10"
        enabled: win.inService
        onActivated: win.closeService()
    }

    // Right-stick cursor.
    Rectangle {
        visible: Pointer.visible
        x: Pointer.x - width / 2
        y: Pointer.y - height / 2
        width: 22
        height: 22
        radius: 11
        color: "#66ffffff"
        border.color: "white"
        border.width: 2
    }

    ButtonHints {
        id: hints
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: dialog.visible ? [] : Router.hints
        persistent: !win.inService
    }

    Osd {
        id: osd
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 36
    }

    ConfirmDialog {
        id: dialog
        anchors.fill: parent
    }

    Component.onCompleted: {
        if (Platform.startupService) {
            const svc = Services.service(Platform.startupService)
            if (svc.id)
                openService(svc)
            else
                osd.show(qsTr("Unknown service: %1").arg(Platform.startupService))
        }
    }
}
