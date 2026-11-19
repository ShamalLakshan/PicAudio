import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 1040)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 620)) - height) / 2)
    width: Math.min(1040, Overlay.overlay ? Math.max(760, Overlay.overlay.width - 48) : 1040)
    height: Math.min(620, Overlay.overlay ? Math.max(480, Overlay.overlay.height - 48) : 620)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool eqEnabled: bridge ? bridge.eqEnabled : true
    property int eqMode: bridge ? bridge.eqMode : 0 // 0: Graphic, 1: Parametric
    property real preampDb: bridge ? bridge.eqPreampDb : 0.0
    property var bandGains: bridge ? bridge.eqBandGains : [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    readonly property var defaultBandFreqs: ["31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz"]
    property var bandFreqs: (bridge && bridge.eqBandLabels && bridge.eqBandLabels.length > 0) ? bridge.eqBandLabels : defaultBandFreqs
    property var peqBands: bridge ? bridge.peqBands : []

    property bool showAutoEqModal: false
    property bool showImportModal: false
    property bool showAttributionModal: false
    property var autoEqResults: []
    property int draggingBandIndex: -1

    readonly property var filterTypeNames: [
        "Peaking (PK)",
        "Low Shelf (LSC)",
        "High Shelf (HSC)",
        "Low Pass (LPF)",
        "High Pass (HPF)",
        "Band Pass (BP)",
        "Notch"
    ]

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 1040);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 620);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
        refreshState();
        if (bridge) {
            root.autoEqResults = bridge.searchAutoEq("");
        }
    }

    onShowAutoEqModalChanged: {
        if (showAutoEqModal && bridge) {
            root.autoEqResults = bridge.searchAutoEq(searchInput.text);
            searchInput.forceActiveFocus();
        }
    }

    function refreshState() {
        if (bridge) {
            root.eqEnabled = bridge.eqEnabled;
            root.eqMode = bridge.eqMode;
            root.preampDb = bridge.eqPreampDb;
            root.bandGains = bridge.eqBandGains;
            root.bandFreqs = (bridge.eqBandLabels && bridge.eqBandLabels.length > 0) ? bridge.eqBandLabels : defaultBandFreqs;
            root.peqBands = bridge.peqBands;
        }
        if (eqCurveCanvas) {
            eqCurveCanvas.requestPaint();
        }
    }

    Connections {
        target: bridge
        function onEqEnabledChanged() { root.refreshState(); }
        function onEqModeChanged() { root.refreshState(); }
        function onEqBandCountChanged() { root.refreshState(); }
        function onEqPreampDbChanged() { root.refreshState(); }
        function onEqBandGainsChanged() { root.refreshState(); }
        function onPeqBandsChanged() { root.refreshState(); }
        function onEqPresetChanged() { root.refreshState(); }
        function onOnlineAutoEqLoaded(success, message) {
            if (success) {
                root.refreshState();
                root.showAutoEqModal = false;
            }
        }
    }

    FileDialog {
        id: importFileDialog
        title: "Import Equalizer Configuration"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Equalizer Configurations (*.txt *.peace *.csv)", "All Files (*)"]
        onAccepted: {
            if (selectedFile) {
                var path = selectedFile.toString();
                if (path.indexOf("file://") === 0) {
                    path = path.substring(7);
                }
                if (bridge) {
                    bridge.importEqFile(path);
                    root.refreshState();
                    root.showImportModal = false;
                }
            }
        }
    }

    FileDialog {
        id: exportFileDialog
        title: "Export Equalizer Configuration (Equalizer APO / Peace format)"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Configuration Text (*.txt)", "Peace Preset (*.peace)", "All Files (*)"]
        defaultSuffix: "txt"
        onAccepted: {
            if (selectedFile) {
                var path = selectedFile.toString();
                if (path.indexOf("file://") === 0) {
                    path = path.substring(7);
                }
                if (bridge) {
                    bridge.exportEqFile(path);
                }
            }
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
                    text: "Equalizer & Acoustic DSP"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                // Mode Selector Segmented Tabs
                Rectangle {
                    Layout.preferredWidth: 230
                    Layout.preferredHeight: 28
                    radius: Theme.cornerRadiusSmall
                    color: Theme.background
                    border.color: Theme.panelBorder
                    clip: true

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: root.eqMode === 0 ? Theme.selection : "transparent"
                            radius: Theme.cornerRadiusSmall

                            Text {
                                anchors.centerIn: parent
                                text: "Graphic (ISO)"
                                font.pixelSize: Theme.fontSizeSmall - 1
                                font.bold: root.eqMode === 0
                                color: root.eqMode === 0 ? Theme.accentHover : Theme.textSecondary
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (bridge) bridge.setEqMode(0);
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: root.eqMode === 1 ? Theme.selection : "transparent"
                            radius: Theme.cornerRadiusSmall

                            Text {
                                anchors.centerIn: parent
                                text: "Parametric (PEQ)"
                                font.pixelSize: Theme.fontSizeSmall - 1
                                font.bold: root.eqMode === 1
                                color: root.eqMode === 1 ? Theme.accentHover : Theme.textSecondary
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (bridge) bridge.setEqMode(1);
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // Master EQ Enable/Bypass Badge Toggle
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
                            if (bridge) bridge.setEqEnabled(!root.eqEnabled);
                        }
                    }
                }

                // Dedicated Reset Flat Button
                Button {
                    id: headerResetBtn
                    text: "Reset"
                    implicitHeight: 28
                    implicitWidth: 64
                    padding: 0
                    hoverEnabled: true
                    contentItem: Text {
                        text: headerResetBtn.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: headerResetBtn.hovered ? Theme.accentHover : Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: headerResetBtn.hovered ? Theme.surfaceElevated : Theme.surface
                        border.color: headerResetBtn.hovered ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                    onClicked: {
                        if (bridge) {
                            if (root.eqMode === 1) {
                                bridge.resetPeq();
                            } else {
                                bridge.resetEq();
                            }
                            root.refreshState();
                        }
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

        // ================= EQUALIZER CURVE & GRAPH =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1
            clip: true

            Canvas {
                id: eqCurveCanvas
                anchors.fill: parent
                anchors.margins: 4
                opacity: root.eqEnabled ? 1.0 : 0.35

                function freqToX(freq, w) {
                    var minL = Math.log10(20.0);
                    var maxL = Math.log10(20000.0);
                    var fL = Math.log10(Math.max(20.0, Math.min(20000.0, freq)));
                    return ((fL - minL) / (maxL - minL)) * w;
                }

                function xToFreq(x, w) {
                    var minL = Math.log10(20.0);
                    var maxL = Math.log10(20000.0);
                    var fL = minL + (x / Math.max(1, w)) * (maxL - minL);
                    return Math.pow(10.0, fL);
                }

                function dbToY(db, h) {
                    var midY = h / 2;
                    var maxDb = 18.0;
                    return midY - (db / maxDb) * (h * 0.44);
                }

                function yToDb(y, h) {
                    var midY = h / 2;
                    var maxDb = 18.0;
                    return ((midY - y) / (h * 0.44)) * maxDb;
                }

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.clearRect(0, 0, width, height);

                    var midY = height / 2;

                    // Draw Grid lines
                    ctx.strokeStyle = Theme.panelBorder;
                    ctx.lineWidth = 1;

                    // 0 dB reference line
                    ctx.beginPath();
                    ctx.moveTo(0, midY);
                    ctx.lineTo(width, midY);
                    ctx.stroke();

                    // Grid frequency markers
                    var gridFreqs = [50, 100, 200, 500, 1000, 2000, 5000, 10000];
                    ctx.setLineDash([2, 4]);
                    for (var g = 0; g < gridFreqs.length; ++g) {
                        var gx = freqToX(gridFreqs[g], width);
                        ctx.beginPath();
                        ctx.moveTo(gx, 0);
                        ctx.lineTo(gx, height);
                        ctx.stroke();
                    }
                    ctx.setLineDash([]); // Reset dash

                    var pts = [];

                    if (root.eqMode === 1 && bridge) {
                        // Parametric PEQ mode: get exact biquad response curve from C++
                        var curve = bridge.calculateResponseCurve(160);
                        if (curve && curve.length > 0) {
                            for (var c = 0; c < curve.length; ++c) {
                                var ptX = freqToX(curve[c].f, width);
                                var ptY = dbToY(curve[c].db, height);
                                pts.push({ x: ptX, y: ptY });
                            }
                        }
                    } else {
                        // Graphic mode: interpolate across band sliders
                        var numPoints = (root.bandGains && root.bandGains.length > 0) ? root.bandGains.length : 10;
                        var stepX = width / Math.max(1, numPoints - 1);
                        var rangeDb = 12.0;
                        for (var i = 0; i < numPoints; ++i) {
                            var gVal = (root.bandGains && root.bandGains[i] !== undefined) ? root.bandGains[i] : 0.0;
                            var py = midY - (gVal / rangeDb) * (height * 0.42);
                            pts.push({ x: i * stepX, y: py });
                        }
                    }

                    if (pts.length > 1) {
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
                    }

                    // Draw Control Point Handles for PEQ filters
                    if (root.eqMode === 1 && root.peqBands) {
                        for (var m = 0; m < root.peqBands.length; ++m) {
                            var b = root.peqBands[m];
                            if (!b.enabled) continue;
                            var hx = freqToX(b.frequency, width);
                            var hy = dbToY(b.gainDb + root.preampDb, height);

                            ctx.beginPath();
                            ctx.arc(hx, hy, 6, 0, Math.PI * 2);
                            ctx.fillStyle = Theme.accentHover;
                            ctx.fill();
                            ctx.strokeStyle = Theme.background;
                            ctx.lineWidth = 2;
                            ctx.stroke();

                            // Number label inside dot
                            ctx.fillStyle = Theme.background;
                            ctx.font = "bold 8px sans-serif";
                            ctx.textAlign = "center";
                            ctx.textBaseline = "middle";
                            ctx.fillText(String(m + 1), hx, hy);
                        }
                    }
                }
            }

            // Interactive dragging of PEQ control points
            MouseArea {
                anchors.fill: parent
                enabled: root.eqMode === 1 && root.eqEnabled
                hoverEnabled: true

                onPressed: function(mouse) {
                    if (!root.peqBands) return;
                    for (var i = 0; i < root.peqBands.length; ++i) {
                        var b = root.peqBands[i];
                        if (!b.enabled) continue;
                        var hx = eqCurveCanvas.freqToX(b.frequency, width);
                        var hy = eqCurveCanvas.dbToY(b.gainDb + root.preampDb, height);
                        if (Math.hypot(mouse.x - hx, mouse.y - hy) <= 12) {
                            root.draggingBandIndex = i;
                            break;
                        }
                    }
                }

                onPositionChanged: function(mouse) {
                    if (root.draggingBandIndex >= 0 && root.draggingBandIndex < root.peqBands.length) {
                        var newFreq = Math.round(eqCurveCanvas.xToFreq(mouse.x, width));
                        newFreq = Math.max(20, Math.min(20000, newFreq));
                        var newGain = Math.round((eqCurveCanvas.yToDb(mouse.y, height) - root.preampDb) * 2) / 2;
                        newGain = Math.max(-18.0, Math.min(18.0, newGain));

                        var cur = root.peqBands[root.draggingBandIndex];
                        if (bridge) {
                            bridge.setPeqBand(root.draggingBandIndex, cur.type, newFreq, newGain, cur.q, cur.enabled);
                        }
                        eqCurveCanvas.requestPaint();
                    }
                }

                onReleased: {
                    root.draggingBandIndex = -1;
                }
            }

            // dB reference label
            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 6
                text: "0 dB REFERENCE"
                font.pixelSize: 8
                font.bold: true
                color: Theme.textMuted
            }

            // Scale readouts
            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                text: "+18 dB"
                font.pixelSize: 8
                color: Theme.textMuted
            }
            Text {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 6
                text: "-18 dB"
                font.pixelSize: 8
                color: Theme.textMuted
            }
        }

        // ================= PEQ / DSP SUB-TOOLBAR =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: 8

                // AutoEq Lookup Button
                Button {
                    id: autoEqBtn
                    implicitHeight: 28
                    implicitWidth: autoEqRow.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: {
                        if (bridge && (!root.autoEqResults || root.autoEqResults.length === 0)) {
                            root.autoEqResults = bridge.searchAutoEq("");
                        }
                        root.showAutoEqModal = !root.showAutoEqModal;
                        root.showImportModal = false;
                        root.showAttributionModal = false;
                    }

                    contentItem: RowLayout {
                        id: autoEqRow
                        spacing: 6
                        VectorIcon {
                            name: "equalizer"
                            width: 14
                            height: 14
                            color: root.showAutoEqModal ? Theme.accent : (autoEqBtn.hovered ? Theme.accentHover : Theme.accent)
                        }
                        Text {
                            text: "Find Headphone / IEM (AutoEq)..."
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            font.bold: true
                            color: autoEqBtn.hovered ? Theme.accentHover : Theme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    background: Rectangle {
                        color: autoEqBtn.hovered ? Theme.surfaceElevated : (root.showAutoEqModal ? Theme.selection : Theme.surface)
                        border.color: (autoEqBtn.hovered || root.showAutoEqModal) ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                // Import EQ Button
                Button {
                    id: importBtn
                    implicitHeight: 28
                    implicitWidth: importRow.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: {
                        root.showImportModal = !root.showImportModal;
                        root.showAutoEqModal = false;
                        root.showAttributionModal = false;
                    }

                    contentItem: RowLayout {
                        id: importRow
                        spacing: 6
                        VectorIcon {
                            name: "folder"
                            width: 14
                            height: 14
                            color: root.showImportModal ? Theme.accent : (importBtn.hovered ? Theme.accentHover : Theme.textSecondary)
                        }
                        Text {
                            text: "Import..."
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: importBtn.hovered ? Theme.accentHover : Theme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    background: Rectangle {
                        color: importBtn.hovered ? Theme.surfaceElevated : (root.showImportModal ? Theme.selection : Theme.surface)
                        border.color: (importBtn.hovered || root.showImportModal) ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                // Export EQ Button
                Button {
                    id: exportBtn
                    implicitHeight: 28
                    implicitWidth: exportRow.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: exportFileDialog.open()

                    contentItem: RowLayout {
                        id: exportRow
                        spacing: 6
                        VectorIcon {
                            name: "arrow_down"
                            width: 14
                            height: 14
                            color: exportBtn.hovered ? Theme.accentHover : Theme.textSecondary
                        }
                        Text {
                            text: "Export..."
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: exportBtn.hovered ? Theme.accentHover : Theme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    background: Rectangle {
                        color: exportBtn.hovered ? Theme.surfaceElevated : Theme.surface
                        border.color: exportBtn.hovered ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                // Attribution Button
                Button {
                    id: creditsBtn
                    implicitHeight: 28
                    implicitWidth: creditsRow.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: {
                        root.showAttributionModal = !root.showAttributionModal;
                        root.showAutoEqModal = false;
                        root.showImportModal = false;
                    }

                    contentItem: RowLayout {
                        id: creditsRow
                        spacing: 6
                        VectorIcon {
                            name: "info"
                            width: 14
                            height: 14
                            color: root.showAttributionModal ? Theme.accent : (creditsBtn.hovered ? Theme.accentHover : Theme.textSecondary)
                        }
                        Text {
                            text: "Credits"
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: creditsBtn.hovered ? Theme.accentHover : Theme.textSecondary
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    background: Rectangle {
                        color: creditsBtn.hovered ? Theme.surfaceElevated : (root.showAttributionModal ? Theme.selection : Theme.surface)
                        border.color: (creditsBtn.hovered || root.showAttributionModal) ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // Master Preamp Control
                RowLayout {
                    spacing: 6

                    Text {
                        text: "Preamp:"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    StyledSlider {
                        id: topPreampSlider
                        Layout.preferredWidth: 100
                        from: -15.0
                        to: 15.0
                        stepSize: 0.5
                        value: root.preampDb
                        onMoved: {
                            if (bridge) bridge.setEqPreampDb(value);
                            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                        }
                    }

                    Text {
                        text: (root.preampDb >= 0 ? "+" : "") + root.preampDb.toFixed(1) + " dB"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Math.abs(root.preampDb) > 0.05 ? Theme.accentHover : Theme.textMuted
                        Layout.preferredWidth: 50
                    }
                }
            }
        }

        // ================= BODY: GRAPHIC VS PARAMETRIC VIEW =================
        StackLayout {
            id: mainViewStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.eqMode

            // ================= PAGE 0: GRAPHIC EQ VIEW =================
            ColumnLayout {
                spacing: 0

                // Graphic Toolbar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.surface
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.panelPadding
                        anchors.rightMargin: Theme.panelPadding
                        spacing: 12

                        Text {
                            text: "Bands:"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }

                        StyledComboBox {
                            Layout.preferredWidth: 110
                            Layout.preferredHeight: 26
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
                                root.refreshState();
                            }
                        }

                        Text {
                            text: "Preset:"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }

                        StyledComboBox {
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 26
                            model: ["Flat", "Rock", "Pop", "Jazz", "Classical", "Vocal", "Bass Boost", "Treble Boost", "Reference", "Custom"]
                            currentIndex: Math.max(0, model.indexOf(bridge ? bridge.eqPreset : "Flat"))
                            onActivated: function(index) {
                                if (bridge && model[index] !== "Custom") {
                                    bridge.setEqPreset(model[index]);
                                    root.refreshState();
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                // Graphic Sliders Area
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Theme.panelPadding
                    spacing: 4

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

                            StyledSlider {
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

            // ================= PAGE 1: PARAMETRIC (PEQ) VIEW =================
            ColumnLayout {
                spacing: 0

                // Filter Bands Table Header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.surface
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.panelPadding
                        anchors.rightMargin: Theme.panelPadding
                        spacing: 8

                        Text { text: "#"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 32 }
                        Text { text: "TYPE"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 140 }
                        Text { text: "FREQUENCY"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 110 }
                        Text { text: "GAIN (dB)"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.fillWidth: true }
                        Text { text: "Q FACTOR"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 90 }
                        Text { text: "ON"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 36; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "DEL"; font.bold: true; font.pixelSize: 10; color: Theme.textMuted; Layout.preferredWidth: 32; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // Scrollable Filters List
                ScrollView {
                    id: peqScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ListView {
                        id: peqListView
                        width: peqScrollView.availableWidth
                        model: root.peqBands ? root.peqBands.length : 0
                        spacing: 4

                        delegate: Rectangle {
                            width: peqListView.width
                            height: 38
                            color: index % 2 === 0 ? Theme.surface : Theme.background
                            border.color: Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.panelPadding
                                anchors.rightMargin: Theme.panelPadding
                                spacing: 8

                                // Filter index badge
                                Rectangle {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 22
                                    radius: Theme.cornerRadiusSmall
                                    color: Theme.selection

                                    Text {
                                        anchors.centerIn: parent
                                        text: String(index + 1)
                                        font.bold: true
                                        font.pixelSize: 10
                                        color: Theme.accentHover
                                    }
                                }

                                // Filter Type Dropdown
                                StyledComboBox {
                                    Layout.preferredWidth: 140
                                    Layout.preferredHeight: 26
                                    model: root.filterTypeNames
                                    currentIndex: {
                                        var b = root.peqBands[index];
                                        return (b && b.type !== undefined) ? Math.min(b.type, root.filterTypeNames.length - 1) : 0;
                                    }
                                    onActivated: function(typeIndex) {
                                        var b = root.peqBands[index];
                                        if (bridge && b) {
                                            bridge.setPeqBand(index, typeIndex, b.frequency, b.gainDb, b.q, b.enabled);
                                            root.refreshState();
                                        }
                                    }
                                }

                                // Frequency SpinBox / Field
                                RowLayout {
                                    Layout.preferredWidth: 110
                                    spacing: 4

                                    TextField {
                                        id: freqInput
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 26
                                        text: {
                                            var b = root.peqBands[index];
                                            return b ? String(Math.round(b.frequency)) : "1000";
                                        }
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSmall
                                        background: Rectangle {
                                            color: Theme.background
                                            border.color: freqInput.activeFocus ? Theme.accent : Theme.panelBorder
                                            radius: Theme.cornerRadiusSmall
                                        }
                                        onEditingFinished: {
                                            var val = parseFloat(text);
                                            if (!isNaN(val) && val >= 10 && val <= 22000) {
                                                var b = root.peqBands[index];
                                                if (bridge && b) {
                                                    bridge.setPeqBand(index, b.type, val, b.gainDb, b.q, b.enabled);
                                                    root.refreshState();
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        text: "Hz"
                                        font.pixelSize: 10
                                        color: Theme.textSecondary
                                    }
                                }

                                // Gain Slider + Readout
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    StyledSlider {
                                        id: filterGainSlider
                                        Layout.fillWidth: true
                                        from: -18.0
                                        to: 18.0
                                        stepSize: 0.5
                                        value: {
                                            var b = root.peqBands[index];
                                            return b ? b.gainDb : 0.0;
                                        }
                                        onMoved: {
                                            var b = root.peqBands[index];
                                            if (bridge && b) {
                                                bridge.setPeqBand(index, b.type, b.frequency, value, b.q, b.enabled);
                                            }
                                            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                                        }
                                    }

                                    Text {
                                        text: {
                                            var b = root.peqBands[index];
                                            var g = b ? b.gainDb : 0.0;
                                            return (g >= 0 ? "+" : "") + g.toFixed(1) + " dB";
                                        }
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: {
                                            var b = root.peqBands[index];
                                            var g = b ? b.gainDb : 0.0;
                                            return Math.abs(g) > 0.05 ? Theme.accentHover : Theme.textMuted;
                                        }
                                        Layout.preferredWidth: 54
                                    }
                                }

                                // Q Factor Field
                                RowLayout {
                                    Layout.preferredWidth: 90
                                    spacing: 4

                                    TextField {
                                        id: qInput
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 26
                                        text: {
                                            var b = root.peqBands[index];
                                            return b ? b.q.toFixed(2) : "1.41";
                                        }
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSmall
                                        background: Rectangle {
                                            color: Theme.background
                                            border.color: qInput.activeFocus ? Theme.accent : Theme.panelBorder
                                            radius: Theme.cornerRadiusSmall
                                        }
                                        onEditingFinished: {
                                            var val = parseFloat(text);
                                            if (!isNaN(val) && val >= 0.05 && val <= 30.0) {
                                                var b = root.peqBands[index];
                                                if (bridge && b) {
                                                    bridge.setPeqBand(index, b.type, b.frequency, b.gainDb, val, b.enabled);
                                                    root.refreshState();
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        text: "Q"
                                        font.pixelSize: 10
                                        color: Theme.textSecondary
                                    }
                                }

                                // Enabled Toggle Checkbox
                                StyledCheckBox {
                                    id: filterEnabledCheck
                                    Layout.preferredWidth: 36
                                    checked: {
                                        var b = root.peqBands[index];
                                        return b ? b.enabled : true;
                                    }
                                    onToggled: {
                                        var b = root.peqBands[index];
                                        if (bridge && b) {
                                            bridge.setPeqBand(index, b.type, b.frequency, b.gainDb, b.q, checked);
                                            root.refreshState();
                                        }
                                    }
                                }

                                // Delete Filter Button
                                Rectangle {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 26
                                    radius: Theme.cornerRadiusSmall
                                    color: delMouse.containsMouse ? Theme.selection : "transparent"
                                    border.color: delMouse.containsMouse ? Theme.accent : "transparent"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✕"
                                        font.pixelSize: 11
                                        color: delMouse.containsMouse ? Theme.accentHover : Theme.textMuted
                                    }

                                    MouseArea {
                                        id: delMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (bridge) {
                                                bridge.removePeqBand(index);
                                                root.refreshState();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // PEQ Action Bar (Add Band / Reset PEQ)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.surfaceElevated
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.panelPadding
                        anchors.rightMargin: Theme.panelPadding
                        spacing: 12

                        Button {
                            id: addBandBtn
                            implicitHeight: 28
                            implicitWidth: addBandRow.implicitWidth + 20
                            hoverEnabled: true
                            contentItem: RowLayout {
                                id: addBandRow
                                spacing: 4
                                VectorIcon {
                                    name: "plus"
                                    width: 12
                                    height: 12
                                    color: addBandBtn.hovered ? Theme.accentHover : Theme.accent
                                }
                                Text {
                                    text: "Add Filter Band"
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                    color: addBandBtn.hovered ? Theme.accentHover : Theme.accent
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            background: Rectangle {
                                color: addBandBtn.hovered ? Theme.surfaceElevated : Theme.surface
                                border.color: addBandBtn.hovered ? Theme.accentHover : Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                            }
                            onClicked: {
                                if (bridge) {
                                    bridge.addPeqBand(0, 1000.0, 0.0, 1.414);
                                    root.refreshState();
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.NoButton
                            }
                        }

                        Button {
                            id: flatResetBtn
                            text: "Flat Reset (5 Bands)"
                            implicitHeight: 28
                            implicitWidth: flatResetText.implicitWidth + 24
                            hoverEnabled: true
                            contentItem: Text {
                                id: flatResetText
                                text: flatResetBtn.text
                                font.pixelSize: Theme.fontSizeSmall
                                font.family: Theme.fontFamily
                                color: flatResetBtn.hovered ? Theme.accentHover : Theme.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: flatResetBtn.hovered ? Theme.surfaceElevated : Theme.surface
                                border.color: flatResetBtn.hovered ? Theme.accent : Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                            }
                            onClicked: {
                                if (bridge) {
                                    bridge.resetPeq();
                                    root.refreshState();
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.NoButton
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "Active PEQ Filters: " + (root.peqBands ? root.peqBands.length : 0) + " / 32"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textMuted
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
                anchors.rightMargin: 40 // Safe margin for ResizeGrip
                spacing: 12

                VectorIcon {
                    name: "equalizer"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: {
                        var modeStr = root.eqMode === 1 ? "Parametric (PEQ)" : "Graphic (" + (bridge ? bridge.eqBandCount : 10) + " Bands)";
                        var enStr = root.eqEnabled ? "Active" : "Bypassed";
                        var preStr = (root.preampDb >= 0 ? "+" : "") + root.preampDb.toFixed(1) + " dB";
                        var pr = bridge ? bridge.eqPreset : "Custom";
                        return "Equalizer: " + enStr + " • Mode: " + modeStr + " • Preamp: " + preStr + " • Profile: " + pr;
                    }
                    font.pixelSize: Theme.fontSizeSmall - 1
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Button {
                    id: doneBtn
                    text: "Done"
                    implicitHeight: 32
                    implicitWidth: 90
                    hoverEnabled: true
                    contentItem: Text {
                        text: doneBtn.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        font.bold: true
                        color: doneBtn.hovered ? Theme.accentHover : Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: doneBtn.hovered ? Theme.surfaceElevated : Theme.selection
                        border.color: doneBtn.hovered ? Theme.accentHover : Theme.accent
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
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

    // ================= MODAL: AUTOEQ LOOKUP DRAWER =================
    Rectangle {
        id: autoEqDrawer
        anchors.fill: parent
        anchors.margins: 16
        radius: Theme.cornerRadiusMedium
        color: Theme.surfaceElevated
        border.color: Theme.accent
        border.width: 1
        visible: root.showAutoEqModal
        z: 100

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.panelPadding
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                VectorIcon {
                    name: "favorite"
                    width: 18
                    height: 18
                    color: Theme.accent
                }

                Text {
                    text: "AutoEq Headphone & IEM Reference Database"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 26
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: closeAutoEqMouse.containsMouse ? Theme.selection : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: closeAutoEqMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.showAutoEqModal = false
                    }
                }
            }

            // Search Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                color: Theme.background
                border.color: searchInput.activeFocus ? Theme.accent : Theme.panelBorder
                border.width: 1
                radius: Theme.cornerRadiusSmall

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    VectorIcon {
                        name: "search"
                        width: 16
                        height: 16
                        color: Theme.textSecondary
                    }

                    TextField {
                        id: searchInput
                        Layout.fillWidth: true
                        placeholderText: "Type headphone or IEM model (e.g. Blessing 2, HD 600, Zero 2, Aria, Sundara)..."
                        font.pixelSize: Theme.fontSizeBase
                        color: Theme.textPrimary
                        background: Item {}
                        onTextChanged: {
                            if (bridge) {
                                root.autoEqResults = bridge.searchAutoEq(text);
                            }
                        }
                    }

                    Button {
                        id: clearSearchBtn
                        text: "Clear"
                        implicitHeight: 24
                        implicitWidth: 48
                        visible: searchInput.text.length > 0
                        hoverEnabled: true
                        contentItem: Text {
                            text: clearSearchBtn.text
                            font.pixelSize: Theme.fontSizeSmall - 1
                            font.family: Theme.fontFamily
                            color: clearSearchBtn.hovered ? Theme.accentHover : Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: clearSearchBtn.hovered ? Theme.surfaceElevated : Theme.surface
                            border.color: Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        onClicked: {
                            searchInput.text = "";
                            if (bridge) root.autoEqResults = bridge.searchAutoEq("");
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }

            // Search Results List
            ScrollView {
                id: autoEqScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Item {
                    anchors.fill: parent
                    visible: !root.autoEqResults || root.autoEqResults.length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        VectorIcon {
                            name: "search"
                            width: 28
                            height: 28
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: searchInput.text.trim().length > 0
                                ? "No matching headphone or IEM models found for \"" + searchInput.text + "\""
                                : "Type in the search bar above to find reference headphone & IEM target models"
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                ListView {
                    id: autoEqListView
                    width: autoEqScrollView.availableWidth
                    model: root.autoEqResults || []
                    spacing: 6

                    delegate: Rectangle {
                        id: profileRow
                        width: autoEqListView.width
                        height: 54
                        radius: Theme.cornerRadiusSmall
                        color: rowMouse.containsMouse ? Theme.surfaceElevated : Theme.surface
                        border.color: rowMouse.containsMouse ? Theme.accent : Theme.panelBorder
                        border.width: 1

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (bridge && modelData && modelData.id) {
                                    bridge.applyAutoEqProfile(modelData.id);
                                    root.refreshState();
                                    root.showAutoEqModal = false;
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            VectorIcon {
                                name: "music"
                                width: 18
                                height: 18
                                color: rowMouse.containsMouse ? Theme.accent : Theme.textMuted
                                Layout.alignment: Qt.AlignVCenter
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: (modelData && modelData.model) ? modelData.model : ""
                                    font.bold: true
                                    font.pixelSize: Theme.fontSizeBase
                                    font.family: Theme.fontFamily
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    spacing: 8
                                    Layout.fillWidth: true

                                    Rectangle {
                                        visible: !!(modelData && modelData.source)
                                        Layout.preferredHeight: 18
                                        Layout.preferredWidth: sourceLabel.contentWidth + 12
                                        radius: Theme.cornerRadiusSmall
                                        color: Theme.background
                                        border.color: Theme.panelBorder
                                        border.width: 1

                                        Text {
                                            id: sourceLabel
                                            anchors.centerIn: parent
                                            text: (modelData && modelData.source) ? modelData.source : ""
                                            font.pixelSize: 10
                                            font.bold: true
                                            font.family: Theme.fontFamily
                                            color: Theme.accent
                                        }
                                    }

                                    Text {
                                        text: {
                                            var parts = [];
                                            if (modelData && modelData.target) parts.push(modelData.target);
                                            if (modelData && modelData.filterCount !== undefined) parts.push(modelData.filterCount + " Filters");
                                            if (modelData && modelData.preampDb !== undefined) parts.push("Preamp: " + Number(modelData.preampDb).toFixed(1) + " dB");
                                            return parts.join(" • ");
                                        }
                                        font.pixelSize: Theme.fontSizeSmall - 1
                                        font.family: Theme.fontFamily
                                        color: Theme.textSecondary
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            Button {
                                id: applyProfileBtn
                                text: "Apply Profile"
                                implicitHeight: 28
                                implicitWidth: 100
                                hoverEnabled: true
                                contentItem: Text {
                                    text: applyProfileBtn.text
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                    color: applyProfileBtn.hovered ? Theme.accentHover : Theme.textPrimary
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: applyProfileBtn.hovered ? Theme.surfaceElevated : Theme.selection
                                    border.color: applyProfileBtn.hovered ? Theme.accentHover : Theme.accent
                                    border.width: 1
                                    radius: Theme.cornerRadiusSmall
                                }
                                onClicked: {
                                    if (bridge && modelData && modelData.id) {
                                        bridge.applyAutoEqProfile(modelData.id);
                                        root.refreshState();
                                        root.showAutoEqModal = false;
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                        }
                    }
                }
            }

            // Attribution Notice Footer in AutoEq Modal
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.background
                radius: Theme.cornerRadiusSmall

                Text {
                    anchors.centerIn: parent
                    text: "AutoEq data by Jaakko Pasanen (MIT License) • Measurements: Oratory1990, Crinacle, Rtings, Super* Review"
                    font.pixelSize: 9
                    color: Theme.textMuted
                }
            }
        }
    }

    // ================= MODAL: IMPORT / PASTE DRAWER =================
    Rectangle {
        id: importDrawer
        anchors.fill: parent
        anchors.margins: 20
        radius: Theme.cornerRadiusMedium
        color: Theme.surfaceElevated
        border.color: Theme.accent
        border.width: 1
        visible: root.showImportModal
        z: 100

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.panelPadding
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                VectorIcon {
                    name: "folder"
                    width: 18
                    height: 18
                    color: Theme.accent
                }

                Text {
                    text: "Import Equalizer Configuration"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 26
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: closeImportMouse.containsMouse ? Theme.selection : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: closeImportMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.showImportModal = false
                    }
                }
            }

            Text {
                text: "Load a file from disk or paste configuration text (supports Equalizer APO, Peace GUI, and Squiglink / Wavelet GraphicEQ formats):"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                id: browseFileBtn
                text: "Browse File from Disk (.txt, .peace)..."
                implicitHeight: 32
                hoverEnabled: true
                contentItem: Text {
                    text: browseFileBtn.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    color: browseFileBtn.hovered ? Theme.accentHover : Theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: browseFileBtn.hovered ? Theme.surfaceElevated : Theme.background
                    border.color: browseFileBtn.hovered ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                }
                onClicked: importFileDialog.open()
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.NoButton
                }
            }

            ScrollView {
                id: pasteScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: pasteTextArea
                    width: pasteScrollView.availableWidth
                    placeholderText: "Paste text from Equalizer APO (Filter 1: ON PK Fc 105 Hz...) or Squiglink (GraphicEQ: 20 0; 25 1.5;...)..."
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: "monospace"
                    color: Theme.textPrimary
                    background: Rectangle {
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    id: pasteBtn
                    text: "Paste from Clipboard"
                    implicitHeight: 30
                    implicitWidth: 150
                    hoverEnabled: true
                    contentItem: Text {
                        text: pasteBtn.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: pasteBtn.hovered ? Theme.accentHover : Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: pasteBtn.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: pasteBtn.hovered ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                    onClicked: pasteTextArea.paste()
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                Button {
                    id: clearPasteBtn
                    text: "Clear"
                    implicitHeight: 30
                    implicitWidth: 60
                    hoverEnabled: true
                    contentItem: Text {
                        text: clearPasteBtn.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: clearPasteBtn.hovered ? Theme.accentHover : Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: clearPasteBtn.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: clearPasteBtn.hovered ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                    onClicked: pasteTextArea.text = ""
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: applyImportBtn
                    text: "Apply Configuration"
                    implicitHeight: 32
                    implicitWidth: 160
                    hoverEnabled: true
                    contentItem: Text {
                        text: applyImportBtn.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        font.bold: true
                        color: applyImportBtn.hovered ? Theme.accentHover : Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: applyImportBtn.hovered ? Theme.surfaceElevated : Theme.selection
                        border.color: applyImportBtn.hovered ? Theme.accentHover : Theme.accent
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                    onClicked: {
                        if (bridge && pasteTextArea.text.trim().length > 0) {
                            if (bridge.importEqText(pasteTextArea.text)) {
                                root.refreshState();
                                root.showImportModal = false;
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }
    }

    // ================= MODAL: ATTRIBUTION CREDITS DRAWER =================
    Rectangle {
        id: attributionDrawer
        anchors.fill: parent
        anchors.margins: 30
        radius: Theme.cornerRadiusMedium
        color: Theme.surfaceElevated
        border.color: Theme.accent
        border.width: 1
        visible: root.showAttributionModal
        z: 100

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                VectorIcon {
                    name: "info"
                    width: 18
                    height: 18
                    color: Theme.accent
                }

                Text {
                    text: "Acoustic DSP & AutoEq Attributions"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 26
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: closeAttrMouse.containsMouse ? Theme.selection : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: closeAttrMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.showAttributionModal = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.background
                radius: Theme.cornerRadiusSmall
                border.color: Theme.panelBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Text {
                        text: "• AutoEq: Headphone & IEM Frequency Response Database by Jaakko Pasanen (MIT License).\n" +
                              "• Measurements: Gras 43AG / 45CA acoustic rig measurements by Oratory1990, Crinacle (In-Ear, 711 & 4620), Rtings, and Super* Review.\n" +
                              "• Target Compensation: Harman Over-Ear (2018), Harman In-Ear (2019 v2), USound 1v1, and Optimum HiFi target curves.\n" +
                              "• DSP Biquad Mathematics: Direct Form II Transposed IIR filter formulations derived from Robert Bristow-Johnson's Audio EQ Cookbook.\n" +
                              "• Formats & Conventions: Equalizer APO, Peace GUI, Wavelet, and Squiglink compatible export and import standards."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textPrimary
                        wrapMode: Text.WordWrap
                        lineHeight: 1.4
                        Layout.fillWidth: true
                    }
                }
            }

            Button {
                id: closeAttrBtn
                text: "Close"
                Layout.alignment: Qt.AlignRight
                implicitHeight: 30
                implicitWidth: 80
                hoverEnabled: true
                contentItem: Text {
                    text: closeAttrBtn.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    color: closeAttrBtn.hovered ? Theme.accentHover : Theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: closeAttrBtn.hovered ? Theme.surfaceElevated : Theme.surface
                    border.color: closeAttrBtn.hovered ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                }
                onClicked: root.showAttributionModal = false
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.NoButton
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 740
        minHeight: 460
    }
}
