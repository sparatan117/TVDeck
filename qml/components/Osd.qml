import QtQuick

// Short-lived on-screen message: seek position, volume, toasts.
Rectangle {
    id: root

    function show(text) {
        label.text = text
        hideTimer.restart()
    }

    implicitWidth: label.implicitWidth + 48
    implicitHeight: 52
    radius: height / 2
    color: "#d0141418"
    opacity: hideTimer.running ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 150 } }

    Timer { id: hideTimer; interval: 1600 }

    Text {
        id: label
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 22
        font.weight: Font.DemiBold
    }
}
