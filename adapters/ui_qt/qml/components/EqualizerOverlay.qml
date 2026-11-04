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
    width: Math.min(960, Overlay.overlay ? Math.max(680, Overlay.overlay.width - 48) : 960)
    height: Math.min(540, Overlay.overlay ? Math.max(400, Overlay.overlay.height - 48) : 540)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool eqEnabled: bridge ? bridge.eqEnabled : true
    property real preampDb: bridge ? bridge.eqPreampDb : 0.0
    property var bandGains: bridge ? bridge.eqBandGains : [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    readonly property var defaultBandFreqs: ["31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz"]
    property var bandFreqs: (bridge && bridge.eqBandLabels && bridge.eqBandLabels.length > 0) ? bridge.eqBandLabels : defaultBandFreqs
    property int currentTab: 0 // 0: Equalizer, 1: DSP Routing & Chain
    property string dspSearchText: ""

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    function getFilteredDspPlugins() {
        var all = bridge ? bridge.discoveredDspPlugins : [];
        if (!all || all.length === 0) return [];
        var q = dspSearchText.trim().toLowerCase();
        if (q === "") return all;
        var res = [];
        for (var i = 0; i < all.length; ++i) {
            var item = all[i];
            var name = (item.name || "").toLowerCase();
            var vendor = (item.vendor || "").toLowerCase();
            var desc = (item.description || "").toLowerCase();
            var id = (item.id || "").toLowerCase();
            var path = (item.libraryPath || "").toLowerCase();
            if (name.indexOf(q) !== -1 || vendor.indexOf(q) !== -1 ||
                desc.indexOf(q) !== -1 || id.indexOf(q) !== -1 || path.indexOf(q) !== -1) {
                res.push(item);
            }
        }
        return res;
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 960);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 540);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
    }

    function setPreset(name) {
        if (bridge) {
            bridge.setEqPreset(name);
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
                    text: "Parametric DSP Equalizer"
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
                    Layout.preferredWidth: 74
                    Layout.preferredHeight: 26
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

                // Mode Tabs (EQ vs DSP Chain & Routing)
                RowLayout {
                    spacing: 4
                    Rectangle {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 26
                        radius: Theme.cornerRadiusSmall
                        color: root.currentTab === 0 ? Theme.surface : "transparent"
                        border.color: root.currentTab === 0 ? Theme.accent : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: (bridge ? bridge.eqBandCount : 10) + "-Band EQ"
                            font.pixelSize: Theme.fontSizeSmall - 1
                            font.bold: root.currentTab === 0
                            color: root.currentTab === 0 ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentTab = 0
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 130
                        Layout.preferredHeight: 26
                        radius: Theme.cornerRadiusSmall
                        color: root.currentTab === 1 ? Theme.surface : "transparent"
                        border.color: root.currentTab === 1 ? Theme.accent : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "DSP Chain & Routing"
                            font.pixelSize: Theme.fontSizeSmall - 1
                            font.bold: root.currentTab === 1
                            color: root.currentTab === 1 ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentTab = 1
                        }
                    }
                }

                // Mode & Preset selector (visible on EQ tab)
                RowLayout {
                    visible: root.currentTab === 0
                    spacing: 8

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
                            if (eqCurveCanvas) eqCurveCanvas.requestPaint();
                        }
                    }

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

        // ================= TAB 0: EQUALIZER VIEW =================
        ColumnLayout {
            visible: root.currentTab === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Frequency Curve Canvas
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 92
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
                    Layout.preferredWidth: (root.bandFreqs && root.bandFreqs.length > 15) ? 42 : 56
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

        // ================= TAB 1: DSP ROUTING & CHAIN VIEW =================
        ScrollView {
            visible: root.currentTab === 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.panelPadding + 4
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 12

                Text {
                    text: "DSP Pipeline Routing & Active Processing Chain"
                    font.pixelSize: Theme.fontSizeBase
                    font.bold: true
                    color: Theme.textPrimary
                }

                Text {
                    text: "Configure the execution sequence between the built-in parametric equalizer and external plug-and-play DSP plugins (such as LSP Expanders or Dynamics processors)."
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textMuted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                // Routing Order Selector
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: "Processing Sequence Order"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "Choose whether equalization occurs prior to dynamic expansion or downstream."
                            font.pixelSize: Theme.fontSizeSmall - 1
                            color: Theme.textMuted
                        }
                    }

                    StyledComboBox {
                        Layout.preferredWidth: 320
                        Layout.preferredHeight: 30
                        model: [
                            "Equalizer → External DSP Plugins",
                            "External DSP Plugins → Equalizer"
                        ]
                        currentIndex: bridge ? bridge.dspRoutingOrder : 0
                        onActivated: function(index) {
                            if (bridge) bridge.setDspRoutingOrder(index)
                        }
                    }
                }

                // Master Toggles Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 24

                    CheckBox {
                        text: "Enable Parametric Equalizer"
                        checked: bridge ? bridge.eqEnabled : true
                        onToggled: if (bridge) bridge.setEqEnabled(checked)
                    }

                    CheckBox {
                        text: "Enable External DSP Chain (.clap plugins)"
                        checked: bridge ? bridge.dspMasterEnabled : true
                        onToggled: if (bridge) bridge.setDspMasterEnabled(checked)
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                // External DSP Plugins Header & Actions
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Active External DSP Plugins"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    // Count Badge
                    Rectangle {
                        visible: bridge && bridge.discoveredDspPlugins && bridge.discoveredDspPlugins.length > 0
                        implicitWidth: countText.implicitWidth + 12
                        implicitHeight: 20
                        radius: Theme.cornerRadiusSmall
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        border.width: 1

                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: {
                                var filtered = root.getFilteredDspPlugins();
                                var total = bridge ? bridge.discoveredDspPlugins.length : 0;
                                if (root.dspSearchText.trim() !== "") {
                                    return filtered.length + " of " + total;
                                }
                                return total + " plugins";
                            }
                            font.pixelSize: Theme.fontSizeSmall - 2
                            font.bold: true
                            color: Theme.accent
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Select All"
                        implicitHeight: 28
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        enabled: bridge && bridge.discoveredDspPlugins && bridge.discoveredDspPlugins.length > 0
                        onClicked: {
                            if (!bridge) return;
                            if (root.dspSearchText.trim() === "") {
                                bridge.setAllDspPluginsEnabled(true);
                            } else {
                                var filtered = root.getFilteredDspPlugins();
                                for (var i = 0; i < filtered.length; ++i) {
                                    bridge.setDspPluginEnabled(filtered[i].id, true);
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Button {
                        text: "Unselect All"
                        implicitHeight: 28
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        enabled: bridge && bridge.discoveredDspPlugins && bridge.discoveredDspPlugins.length > 0
                        onClicked: {
                            if (!bridge) return;
                            if (root.dspSearchText.trim() === "") {
                                bridge.setAllDspPluginsEnabled(false);
                            } else {
                                var filtered = root.getFilteredDspPlugins();
                                for (var i = 0; i < filtered.length; ++i) {
                                    bridge.setDspPluginEnabled(filtered[i].id, false);
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Button {
                        text: "Open DSP Folder"
                        implicitHeight: 28
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        onClicked: if (bridge) bridge.openDspDirectory()
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Button {
                        text: "Rescan"
                        implicitHeight: 28
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        onClicked: if (bridge) bridge.scanDspPlugins()
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }

                // Search Bar
                Rectangle {
                    visible: bridge && bridge.discoveredDspPlugins && bridge.discoveredDspPlugins.length > 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    radius: Theme.cornerRadiusSmall
                    color: Theme.background
                    border.color: dspSearchInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 6

                        VectorIcon {
                            name: "search"
                            width: 14
                            height: 14
                            color: Theme.textMuted
                        }

                        TextInput {
                            id: dspSearchInput
                            Layout.fillWidth: true
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textPrimary
                            clip: true
                            text: root.dspSearchText
                            onTextChanged: root.dspSearchText = text

                            Text {
                                anchors.fill: parent
                                text: "Search plugins by name, vendor, description, or id..."
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                                visible: !dspSearchInput.text && !dspSearchInput.activeFocus
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            visible: dspSearchInput.text !== ""
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            radius: 9
                            color: clearSearchMouse.containsMouse ? Theme.surfaceElevated : "transparent"

                            VectorIcon {
                                anchors.centerIn: parent
                                name: "clear"
                                width: 9
                                height: 9
                                color: Theme.textMuted
                            }

                            MouseArea {
                                id: clearSearchMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    dspSearchInput.text = "";
                                    root.dspSearchText = "";
                                }
                            }
                        }
                    }
                }

                // Discovered Plugins Cards
                Repeater {
                    model: root.getFilteredDspPlugins()
                    delegate: Rectangle {
                        id: pluginCard
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: Math.max(62, cardRow.implicitHeight + 16)
                        radius: Theme.cornerRadiusSmall
                        color: modelData.enabled ? Theme.surfaceElevated : Theme.surface
                        border.color: modelData.enabled ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        clip: true

                        RowLayout {
                            id: cardRow
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            anchors.topMargin: 8
                            anchors.bottomMargin: 8
                            spacing: 12

                            CheckBox {
                                Layout.alignment: Qt.AlignVCenter
                                checked: modelData.enabled
                                onToggled: if (bridge) bridge.setDspPluginEnabled(modelData.id, checked)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                Layout.minimumWidth: 100
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name + (modelData.vendor ? " — " + modelData.vendor : "")
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: true
                                    color: modelData.enabled ? Theme.accentHover : Theme.textPrimary
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.description ? modelData.description : modelData.libraryPath
                                    font.pixelSize: Theme.fontSizeSmall - 1
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }
                            }

                            // Open UI button: always shown for CLAP plugins!
                            Button {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 30
                                text: (bridge && bridge.isDspPluginWindowVisible(modelData.id)) ? "Close UI" : "Open UI"
                                onClicked: {
                                    if (!bridge) return;
                                    if (bridge.isDspPluginWindowVisible(modelData.id)) {
                                        bridge.hideDspPluginWindow(modelData.id);
                                    } else {
                                        bridge.showDspPluginWindow(modelData.id);
                                    }
                                }
                            }
                        }
                    }
                }

                // Empty state if no external DSP plugins discovered
                Rectangle {
                    visible: !bridge || !bridge.discoveredDspPlugins || bridge.discoveredDspPlugins.length === 0
                    Layout.fillWidth: true
                    implicitHeight: 72
                    radius: Theme.cornerRadiusSmall
                    color: Theme.background
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        VectorIcon {
                            name: "equalizer"
                            width: 20
                            height: 20
                            color: Theme.textMuted
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "No external .clap DSP plugins loaded yet."
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textSecondary
                            }
                            Text {
                                text: "Drop your downloaded native .clap plugins (e.g. LSP Expander) into the DSP folder and click 'Rescan Plugins'."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
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
                        return "Parametric EQ: " + en + " • " + bc + " Bands • Preset: " + pr + (root.currentTab === 1 ? " • Routing: " + (bridge && bridge.dspRoutingOrder === 1 ? "DSP → EQ" : "EQ → DSP") : "");
                    }
                    font.pixelSize: Theme.fontSizeSmall - 1
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Button {
                    text: "Reset Flat"
                    implicitHeight: 32
                    implicitWidth: 96
                    padding: Theme.spacingSmall
                    leftPadding: Theme.spacingMedium
                    rightPadding: Theme.spacingMedium
                    hoverEnabled: true
                    onClicked: {
                        if (bridge) bridge.resetEq();
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
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
