import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 960)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 540)) - height) / 2)
    width: Math.min(960, Overlay.overlay ? Math.max(720, Overlay.overlay.width - 48) : 960)
    height: Math.min(540, Overlay.overlay ? Math.max(420, Overlay.overlay.height - 48) : 540)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool eqEnabled: bridge ? bridge.eqEnabled : true
    property real preampDb: bridge ? bridge.eqPreampDb : 0.0
    property var bandGains: bridge ? bridge.eqBandGains : [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    readonly property var defaultBandFreqs: ["31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz"]
    property var bandFreqs: (bridge && bridge.eqBandLabels && bridge.eqBandLabels.length > 0) ? bridge.eqBandLabels : defaultBandFreqs

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 960);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 540);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
        if (bridge) {
            root.bandGains = bridge.eqBandGains;
            root.preampDb = bridge.eqPreampDb;
        }
        if (eqCurveCanvas) {
            eqCurveCanvas.requestPaint();
        }
    }

    Connections {
        target: root
        function onBandGainsChanged() {
            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
        }
        function onPreampDbChanged() {
            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
        }
    }

    function setPreset(name) {
        if (bridge) {
            bridge.setEqPreset(name);
            root.bandGains = bridge.eqBandGains;
            root.preampDb = bridge.eqPreampDb;
        }
        if (eqCurveCanvas) {
            eqCurveCanvas.requestPaint();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ================= HEADER =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
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
                    width: 18
                    height: 18
                    color: Theme.accent
                }

                Text {
                    text: "Parametric Equalizer"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                // Header drag area spacer
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // Master EQ Bypass/Enable Badge Toggle
                Rectangle {
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 28
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
                        hoverEnabled: true
                        onClicked: {
                            if (bridge) {
                                bridge.setEqEnabled(!root.eqEnabled);
                            }
                        }
                    }
                }

                // Band count resolution selector
                RowLayout {
                    spacing: 6

                    Text {
                        text: "Bands:"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    StyledComboBox {
                        id: bandCountCombo
                        Layout.preferredWidth: 105
                        Layout.preferredHeight: 28
                        model: ["10 Bands", "15 Bands", "31 Bands"]
                        currentIndex: {
                            var bc = bridge ? bridge.eqBandCount : 10;
                            if (bc === 31) return 2;
                            if (bc === 15) return 1;
                            return 0;
                        }
                        onActivated: function(index) {
                            if (!bridge) return;
                            if (index === 2) bridge.setEqBandCount(31);
                            else if (index === 1) bridge.setEqBandCount(15);
                            else bridge.setEqBandCount(10);
                            root.bandGains = bridge.eqBandGains;
                            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                        }
                    }
                }

                // Preset selector
                RowLayout {
                    spacing: 6

                    Text {
                        text: "Preset:"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    StyledComboBox {
                        id: presetCombo
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 28
                        model: ["Flat", "Rock", "Pop", "Jazz", "Classical", "Vocal", "Bass Boost", "Treble Boost", "Reference"]
                        currentIndex: Math.max(0, model.indexOf(bridge ? bridge.eqPreset : "Flat"))
                        onActivated: function(index) {
                            root.setPreset(model[index]);
                        }
                    }
                }

                // Dedicated Reset Flat Button
                Button {
                    text: "Reset"
                    implicitHeight: 28
                    implicitWidth: 64
                    padding: 0
                    hoverEnabled: true
                    onClicked: {
                        if (bridge) {
                            bridge.resetEq();
                            root.bandGains = bridge.eqBandGains;
                            root.preampDb = bridge.eqPreampDb;
                        }
                        if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                // Dedicated Close Button
                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: closeMouse.containsMouse ? Theme.selection : "transparent"
                    border.color: closeMouse.containsMouse ? Theme.accent : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: Theme.fontSizeSmall
                        color: closeMouse.containsMouse ? Theme.accentHover : Theme.textSecondary
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                    ToolTip.visible: closeMouse.containsMouse
                    ToolTip.text: "Close (Esc)"
                }
            }
        }

        // ================= EQUALIZER VIEW =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Frequency Curve Canvas
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
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

                        var midY = height / 2;
                        ctx.strokeStyle = Theme.panelBorder;
                        ctx.lineWidth = 1;

                        // 0 dB reference line
                        ctx.beginPath();
                        ctx.moveTo(0, midY);
                        ctx.lineTo(width, midY);
                        ctx.stroke();

                        var numPoints = (root.bandGains && root.bandGains.length > 0) ? root.bandGains.length : 10;
                        var stepX = width / Math.max(1, numPoints - 1);
                        var rangeDb = 12.0;

                        var pts = [];
                        for (var i = 0; i < numPoints; ++i) {
                            var g = (root.bandGains && root.bandGains[i] !== undefined) ? root.bandGains[i] : 0.0;
                            var py = midY - (g / rangeDb) * (height * 0.42);
                            pts.push({ x: i * stepX, y: py });
                        }

                        // Gradient Fill
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

                        // Stroke Curve
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

                        // Control Point Dots
                        var ptRadius = numPoints > 15 ? 2.0 : 3.5;
                        for (var m = 0; m < pts.length; ++m) {
                            ctx.beginPath();
                            ctx.arc(pts[m].x, pts[m].y, ptRadius, 0, Math.PI * 2);
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
                    Layout.preferredWidth: (root.bandFreqs && root.bandFreqs.length > 15) ? 44 : 56
                    Layout.fillHeight: true
                    spacing: 4

                    Text {
                        text: (root.preampDb >= 0 ? "+" : "") + root.preampDb.toFixed(1) + " dB"
                        font.pixelSize: 9
                        font.bold: true
                        color: Math.abs(root.preampDb) > 0.05 ? Theme.accentHover : Theme.textMuted
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
                            if (bridge) {
                                bridge.setEqPreampDb(value);
                            }
                            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                        }
                    }

                    Binding {
                        target: preampSlider
                        property: "value"
                        value: root.preampDb
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

                // Dynamic Frequency Band Sliders (10, 15, or 31 Bands)
                Repeater {
                    model: root.bandFreqs ? root.bandFreqs.length : 10

                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: (root.bandFreqs && root.bandFreqs.length > 15) ? 2 : 4

                        Text {
                            text: {
                                var v = (root.bandGains && root.bandGains[index] !== undefined) ? root.bandGains[index] : 0.0;
                                var isCompact = root.bandFreqs && root.bandFreqs.length > 15;
                                return (v >= 0 ? "+" : "") + (isCompact ? v.toFixed(0) : v.toFixed(1) + " dB");
                            }
                            font.pixelSize: (root.bandFreqs && root.bandFreqs.length > 15) ? 7 : 8
                            font.bold: true
                            color: {
                                var v = (root.bandGains && root.bandGains[index] !== undefined) ? root.bandGains[index] : 0.0;
                                return Math.abs(v) > 0.05 ? Theme.accentHover : Theme.textMuted;
                            }
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Slider {
                            id: bandSlider
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            orientation: Qt.Vertical
                            from: -12.0
                            to: 12.0
                            stepSize: 0.5
                            value: (root.bandGains && root.bandGains[index] !== undefined) ? root.bandGains[index] : 0.0
                            onMoved: {
                                if (bridge) {
                                    bridge.setEqBandGain(index, value);
                                    root.bandGains[index] = value;
                                }
                                if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                            }
                        }

                        Binding {
                            target: bandSlider
                            property: "value"
                            value: (root.bandGains && root.bandGains[index] !== undefined) ? root.bandGains[index] : 0.0
                        }

                        Text {
                            text: (root.bandFreqs && root.bandFreqs[index]) ? root.bandFreqs[index] : ""
                            font.pixelSize: (root.bandFreqs && root.bandFreqs.length > 15) ? 7 : 8
                            color: Theme.textSecondary
                            Layout.alignment: Qt.AlignHCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        // ================= FOOTER ACTION BAR =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding + 4
                anchors.rightMargin: Theme.panelPadding + 4
                spacing: 12

                VectorIcon {
                    name: "info"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: {
                        var bc = bridge ? bridge.eqBandCount : 10;
                        var pr = bridge ? bridge.eqPreset : "Flat";
                        var en = root.eqEnabled ? "Active" : "Bypassed";
                        var pre = (root.preampDb >= 0 ? "+" : "") + root.preampDb.toFixed(1) + " dB";
                        return "Parametric EQ: " + en + " • " + bc + " Bands • Preset: " + pr + " • Preamp: " + pre;
                    }
                    font.pixelSize: Theme.fontSizeSmall - 1
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Button {
                    text: "Done"
                    implicitHeight: 32
                    implicitWidth: 90
                    padding: Theme.spacingSmall
                    leftPadding: Theme.spacingMedium
                    rightPadding: Theme.spacingMedium
                    hoverEnabled: true
                    highlighted: true
                    onClicked: root.close()
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 640
        minHeight: 360
    }
}
