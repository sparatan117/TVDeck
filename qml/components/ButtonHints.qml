import QtQuick

// Bottom bar listing what each button does in the current context.
// Inside a service it only appears briefly (context change or Start).
Item {
    id: root
    property var model: []
    property bool persistent: true

    function flash() { flashTimer.restart() }

    implicitHeight: 72
    opacity: persistent || flashTimer.running ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Timer { id: flashTimer; interval: 3500 }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#e0000000" }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 40
        anchors.verticalCenter: parent.verticalCenter
        spacing: 26
        // Busy contexts (the player) list more buttons than fit at 1280 px.
        transformOrigin: Item.Right
        scale: Math.min(1, (root.width - 80) / Math.max(1, implicitWidth))

        Repeater {
            model: root.model
            Row {
                required property var modelData
                spacing: 10
                Text {
                    visible: modelData.hold
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Hold")
                    color: "#aaffffff"
                    font.pixelSize: 16
                }
                ButtonGlyph { button: modelData.button; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: "white"
                    font.pixelSize: 18
                }
            }
        }
    }
}
