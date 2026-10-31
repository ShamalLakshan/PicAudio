import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Rectangle {
    id: root
    width: Math.min(840, parent ? parent.width - 40 : 840)
    height: Math.min(420, parent ? parent.height - 40 : 380)
    x: parent ? Math.max(16, Math.round((parent.width - width) / 2)) : 50
    y: parent ? Math.max(16, Math.round((parent.height - height) / 2)) : 50
    radius: Theme.cornerRadiusMedium
    color: Theme.surface
    border.color: Theme.panelBorder
    border.width: 1
    clip: true

    property bool eqEnabled: true
    property real preampDb: 0.0
    property var bandGains: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    readonly property var bandFreqs: ["31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz"]

    signal closed()

    function resetPosition() {
        if (parent) {
            x = Math.max(16, Math.round((parent.width - width) / 2));
            y = Math.max(16, Math.round((parent.height - height) / 2));
        }
    }

    onVisibleChanged: {
        if (visible && (x <= 0 || y <= 0 || (parent && (x > parent.width - 60 || y > parent.height - 60)))) {
            resetPosition();
        }
    }

    onWidthChanged: if (eqCurveCanvas) eqCurveCanvas.requestPaint()
    onHeightChanged: if (eqCurveCanvas) eqCurveCanvas.requestPaint()

    function setPreset(name) {
        var presets = {
            "Flat": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            "Bass Boost": [5.5, 4.5, 3.5, 2.0, 1.0, 0, 0, 0, 0, 0],
            "Treble Boost": [0, 0, 0, 0, 0.5, 1.5, 2.5, 4.0, 5.0, 6.0],
            "Rock": [4.5, 3.0, 1.5, 0.0, -1.0, -0.5, 1.5, 3.0, 4.0, 4.5],
            "Pop": [-1.0, 0.5, 2.0, 3.0, 3.5, 2.5, 1.0, 0.0, 1.0, 2.0],
            "Jazz": [3.0, 2.0, 1.0, 1.5, -1.0, -1.0, 0.0, 1.5, 2.5, 3.5],
            "Classical": [4.0, 3.0, 2.0, 1.5, -1.0, -1.0, 0.0, 2.0, 3.0, 3.5],
            "Vocal": [-2.0, -1.0, 0.0, 2.5, 4.0, 3.5, 2.0, 0.5, -1.0, -2.0],
            "Reference": [0.5, 0.2, 0.0, -0.2, -0.2, 0.0, 0.2, 0.4, 0.5, 0.5]
        };
        if (presets[name]) {
            var arr = [];
            for (var i = 0; i < 10; ++i) {
                arr.push(presets[name][i]);
            }
            bandGains = arr;
            eqCurveCanvas.requestPaint();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            MovableHeaderArea {
                target: root
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding + 4
                anchors.rightMargin: Theme.panelPadding + 4
                spacing: Theme.spacingMedium

                VectorIcon {
                    name: "equalizer"
                    width: 16
                    height: 16
                    color: Theme.accent
                }

                Text {
                    text: "Parametric DSP Equalizer"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                Rectangle {
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 22
                    radius: Theme.cornerRadiusSmall
                    color: root.eqEnabled ? Theme.selection : Theme.surface
                    border.color: root.eqEnabled ? Theme.accent : Theme.panelBorder
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: root.eqEnabled ? "ENABLED" : "BYPASS"
                        font.pixelSize: 9
                        font.bold: true
                        color: root.eqEnabled ? Theme.accentHover : Theme.textMuted
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.eqEnabled = !root.eqEnabled;
                            eqCurveCanvas.requestPaint();
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Preset selector
                Text {
                    text: "Preset:"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                ComboBox {
                    id: presetCombo
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 28
                    model: ["Flat", "Rock", "Pop", "Jazz", "Classical", "Vocal", "Bass Boost", "Treble Boost", "Reference"]
                    onActivated: function(index) {
                        root.setPreset(model[index]);
                    }

                    background: Rectangle {
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }

                    contentItem: Text {
                        leftPadding: 8
                        text: presetCombo.displayText
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textPrimary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: Theme.cornerRadiusSmall
                    color: resetMouse.containsMouse ? Theme.surface : "transparent"
                    border.color: Theme.panelBorder

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "clear"
                        width: 10
                        height: 10
                        color: Theme.textMuted
                    }

                    MouseArea {
                        id: resetMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            presetCombo.currentIndex = 0;
                            root.setPreset("Flat");
                            root.preampDb = 0.0;
                        }
                    }
                    ToolTip.visible: resetMouse.containsMouse
                    ToolTip.text: "Reset to Flat EQ"
                }

                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: Theme.cornerRadiusSmall
                    color: closeMouse.containsMouse ? Theme.surface : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "clear"
                        width: 11
                        height: 11
                        color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closed()
                    }
                }
            }
        }

        // Curve Canvas Display
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1
            clip: true

            Canvas {
                id: eqCurveCanvas
                anchors.fill: parent
                anchors.margins: 4
                opacity: root.eqEnabled ? 1.0 : 0.35

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.clearRect(0, 0, width, height);

                    // Grid lines (0 dB, +6 dB, -6 dB)
                    var midY = height / 2;
                    ctx.strokeStyle = Theme.panelBorder;
                    ctx.lineWidth = 1;

                    ctx.beginPath();
                    ctx.moveTo(0, midY);
                    ctx.lineTo(width, midY);
                    ctx.stroke();

                    // Frequency Curve
                    var numPoints = 10;
                    var stepX = width / (numPoints - 1);
                    var rangeDb = 12.0;

                    var pts = [];
                    for (var i = 0; i < numPoints; ++i) {
                        var g = root.bandGains[i] || 0.0;
                        var py = midY - (g / rangeDb) * (height * 0.42);
                        pts.push({ x: i * stepX, y: py });
                    }

                    // Fill under curve
                    ctx.beginPath();
                    ctx.moveTo(pts[0].x, pts[0].y);
                    for (var j = 0; j < pts.length - 1; ++j) {
                        var cpx = (pts[j].x + pts[j + 1].x) / 2;
                        var cpy = (pts[j].y + pts[j + 1].y) / 2;
                        ctx.quadraticCurveTo(pts[j].x, pts[j].y, cpx, cpy);
                    }
                    ctx.lineTo(pts[pts.length - 1].x, pts[pts.length - 1].y);
                    ctx.lineTo(width, height);
                    ctx.lineTo(0, height);
                    ctx.closePath();

                    var grad = ctx.createLinearGradient(0, 0, 0, height);
                    grad.addColorStop(0, Theme.selection);
                    grad.addColorStop(1, "transparent");
                    ctx.fillStyle = grad;
                    ctx.fill();

                    // Stroke curve
                    ctx.beginPath();
                    ctx.moveTo(pts[0].x, pts[0].y);
                    for (var k = 0; k < pts.length - 1; ++k) {
                        var cpx2 = (pts[k].x + pts[k + 1].x) / 2;
                        var cpy2 = (pts[k].y + pts[k + 1].y) / 2;
                        ctx.quadraticCurveTo(pts[k].x, pts[k].y, cpx2, cpy2);
                    }
                    ctx.lineTo(pts[pts.length - 1].x, pts[pts.length - 1].y);
                    ctx.strokeStyle = Theme.accent;
                    ctx.lineWidth = 2;
                    ctx.stroke();

                    // Control points
                    for (var m = 0; m < pts.length; ++m) {
                        ctx.beginPath();
                        ctx.arc(pts[m].x, pts[m].y, 3.5, 0, Math.PI * 2);
                        ctx.fillStyle = Theme.accentHover;
                        ctx.fill();
                        ctx.strokeStyle = Theme.background;
                        ctx.lineWidth = 1;
                        ctx.stroke();
                    }
                }
            }

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 6
                text: "0 dB REFERENCE"
                font.pixelSize: 8
                font.bold: true
                color: Theme.textMuted
            }
        }

        // Sliders Section
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.panelPadding
            spacing: 6

            // Preamp Column
            ColumnLayout {
                Layout.preferredWidth: 60
                Layout.fillHeight: true
                spacing: 4

                Text {
                    text: (root.preampDb >= 0 ? "+" : "") + root.preampDb.toFixed(1) + " dB"
                    font.pixelSize: 9
                    font.bold: true
                    color: root.preampDb !== 0 ? Theme.accentHover : Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }

                Slider {
                    id: preampSlider
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignHCenter
                    orientation: Qt.Vertical
                    from: -12.0
                    to: 12.0
                    stepSize: 0.5
                    value: root.preampDb
                    onMoved: {
                        root.preampDb = value;
                        eqCurveCanvas.requestPaint();
                    }
                }

                Text {
                    text: "PREAMP"
                    font.pixelSize: 9
                    font.bold: true
                    color: Theme.accent
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.panelBorder
            }

            // 10 Frequency Band Sliders
            Repeater {
                model: 10

                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 4

                    Text {
                        text: {
                            var v = root.bandGains[index] || 0.0;
                            return (v >= 0 ? "+" : "") + v.toFixed(1) + " dB";
                        }
                        font.pixelSize: 8
                        font.bold: true
                        color: {
                            var v = root.bandGains[index] || 0.0;
                            return Math.abs(v) > 0.01 ? Theme.accentHover : Theme.textMuted;
                        }
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: bandSlider
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignHCenter
                        orientation: Qt.Vertical
                        from: -12.0
                        to: 12.0
                        stepSize: 0.5
                        value: root.bandGains[index] || 0.0
                        onMoved: {
                            var copy = [];
                            for (var b = 0; b < 10; ++b) {
                                copy.push(b === index ? value : (root.bandGains[b] || 0.0));
                            }
                            root.bandGains = copy;
                            eqCurveCanvas.requestPaint();
                        }
                    }

                    Text {
                        text: root.bandFreqs[index]
                        font.pixelSize: 8
                        color: Theme.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 620
        minHeight: 320
    }
}
