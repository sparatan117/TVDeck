import QtQuick

// Yes/no prompt driven by launcher bindings (A confirm, B cancel).
Item {
    id: root
    visible: false

    property string text
    property var acceptCallback: null

    function ask(text, onAccept) {
        root.text = text
        acceptCallback = onAccept
        visible = true
    }

    function handleAction(name, arg) {
        if (name === "activate") {
            visible = false
            if (acceptCallback)
                acceptCallback()
        } else if (name === "back") {
            visible = false
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#b0000000"
    }

    Rectangle {
        anchors.centerIn: parent
        width: 520
        height: 220
        radius: 18
        color: "#1d1d24"
        border.color: "#44ffffff"

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 48
            text: root.text
            color: "white"
            font.pixelSize: 28
            font.bold: true
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 36
            spacing: 40
            Row {
                spacing: 10
                ButtonGlyph { button: "a" }
                Text { text: qsTr("Confirm"); color: "white"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 10
                ButtonGlyph { button: "b" }
                Text { text: qsTr("Cancel"); color: "white"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
            }
        }
    }
}
