import QtQuick
import StreamDeck.Native

// ES-DE style carousel. Everything here is static unless input arrives:
// Qt Quick only renders when something changes, so an idle launcher costs
// close to nothing. Avoid looping animations, video backgrounds and
// once-per-second timers.
FocusScope {
    id: root

    property var services: []
    readonly property var current: services.length > 0 ? services[list.currentIndex] : null
    readonly property color accent: current && current.accent ? current.accent : "#444455"

    signal launchRequested(var service)
    signal quitRequested()

    function handleAction(name, arg) {
        switch (name) {
        case "nav":
            if (arg === "left") list.decrementCurrentIndex()
            else if (arg === "right") list.incrementCurrentIndex()
            break
        case "activate":
            if (current) launchRequested(current)
            break
        case "back":
            quitRequested()
            break
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.darker(root.accent, 3.0) }
            GradientStop { position: 0.7; color: "#08080b" }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 40
        text: "StreamDeck"
        color: "white"
        font.pixelSize: 34
        font.weight: Font.Black
    }

    Text {
        id: clock
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 40
        color: "#ccffffff"
        font.pixelSize: 26
        function update() {
            const fmt = Qt.locale().timeFormat(Locale.ShortFormat).replace(/:ss/, "")
            text = Qt.formatTime(new Date(), fmt)
        }
        Component.onCompleted: update()
        // Minute resolution is all a clock needs; avoid waking every second.
        Timer {
            interval: 30000
            repeat: true
            running: root.visible
            triggeredOnStart: true
            onTriggered: clock.update()
        }
    }

    ListView {
        id: list
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -40
        width: parent.width
        height: 300
        orientation: ListView.Horizontal
        spacing: 36
        model: root.services
        interactive: false
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: width / 2 - 220
        preferredHighlightEnd: width / 2 + 220
        highlightMoveDuration: 160
        keyNavigationEnabled: false

        delegate: Rectangle {
            required property var modelData
            required property int index
            readonly property bool isCurrent: ListView.isCurrentItem

            width: 440
            height: 250
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            radius: 20
            color: modelData.accent || "#333"
            scale: isCurrent ? 1.0 : 0.8
            opacity: isCurrent ? 1.0 : 0.55
            border.width: isCurrent ? 4 : 0
            border.color: "white"
            Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: 140 } }

            // Plain wordmarks: we don't ship service logos (trademarks).
            Text {
                anchors.centerIn: parent
                text: modelData.name
                color: "white"
                font.pixelSize: 58
                font.weight: Font.Black
            }
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: list.bottom
        anchors.topMargin: 24
        spacing: 12
        width: Math.min(parent.width - 80, 900)

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.current ? root.current.description || "" : qsTr("No services configured")
            color: "#ddffffff"
            font.pixelSize: 22
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: root.current !== null && root.current.requiresDrm === true && !Platform.drmAvailable
            text: qsTr("Widevine DRM isn't installed, so video won't play. "
                       + "Run streamdeck-install-widevine from Desktop Mode.")
            color: "#ffb74d"
            font.pixelSize: 18
        }
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 40
        anchors.bottomMargin: 90
        text: Gamepad.connected ? Gamepad.controllerName : qsTr("No controller detected")
        color: "#88ffffff"
        font.pixelSize: 16
    }
}
