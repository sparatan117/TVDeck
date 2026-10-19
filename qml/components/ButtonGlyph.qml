import QtQuick

// A controller button drawn as text, so no per-platform artwork is needed.
Rectangle {
    id: root
    property string button

    readonly property var glyphs: ({
        a: "A", b: "B", x: "X", y: "Y",
        leftshoulder: "L1", rightshoulder: "R1", lefttrigger: "L2", righttrigger: "R2",
        dpleft: "◀", dpright: "▶", dpup: "▲", dpdown: "▼",
        start: "☰", back: "⧉", leftstick: "L3", rightstick: "R3",
        rscw: "R ↻", rsccw: "R ↺",
    })
    readonly property bool face: ["a", "b", "x", "y"].indexOf(button) >= 0

    implicitHeight: 34
    implicitWidth: face ? 34 : Math.max(34, label.implicitWidth + 16)
    radius: face ? height / 2 : 8
    color: "#2c2c34"
    border.color: "#99ffffff"
    border.width: 1

    Text {
        id: label
        anchors.centerIn: parent
        text: root.glyphs[root.button] || root.button
        color: "white"
        font.pixelSize: 16
        font.bold: true
    }
}
