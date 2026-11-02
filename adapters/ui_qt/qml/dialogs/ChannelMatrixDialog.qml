import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 880)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 680)) - height) / 2)
    width: Math.min(880, Overlay.overlay ? Math.max(760, Overlay.overlay.width - 48) : 880)
    height: Math.min(680, Overlay.overlay ? Math.max(540, Overlay.overlay.height - 48) : 680)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var matrixData: (typeof bridge !== "undefined" && bridge) ? bridge.channelMatrix : []
    property var levels: (typeof bridge !== "undefined" && bridge) ? bridge.channelLevels : []
    property int selectedOutCh: -1
    property int selectedInCh: -1

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 880);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 680);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
        root.matrixData = bridge.channelMatrix;
        root.levels = bridge.channelLevels;
    }

    Timer {
        interval: 50
        running: root.visible
        repeat: true
        onTriggered: {
            root.levels = bridge.channelLevels;
        }
    }

    Connections {
        target: bridge
        function onChannelMatrixChanged() {
            root.matrixData = bridge.channelMatrix;
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Movable Header Bar
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
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    VectorIcon {
                        name: "equalizer"
                        width: 18
                        height: 18
                        color: Theme.accentHover
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.fillWidth: true

                        Text {
                            text: "Channel Matrix & Surround Processor (Mono to 7.1)"
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "Audiophile matrix routing, downmix (ITU-R BS.775), and upmixing for Mono, Stereo, 5.1, and 7.1"
                            font.pixelSize: Theme.fontSizeSmall - 1
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        width: 30
                        height: 30
                        radius: Theme.cornerRadiusSmall
                        color: closeArea.containsMouse ? Theme.selection : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            font.pixelSize: Theme.fontSizeSmall
                            color: closeArea.containsMouse ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            id: closeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            // Toolbar / Preset selection
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                color: Theme.surfaceElevated

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 10

                    Text {
                        text: "Preset:"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    ComboBox {
                        id: presetCombo
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        model: [
                            "Stereo Passthrough",
                            "Downmix Surround to Stereo",
                            "Mono Downmix",
                            "Upmix Stereo to 5.1",
                            "Cinema Surround Upmix (7.1)",
                            "Custom Channel Matrix"
                        ]
                        currentIndex: {
                            var idx = model.indexOf(bridge.channelProcessing);
                            return idx >= 0 ? idx : 0;
                        }
                        onActivated: function(index) {
                            bridge.setChannelProcessing(model[index]);
                        }
                        background: Rectangle {
                            implicitWidth: 220
                            implicitHeight: 30
                            color: presetCombo.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: presetCombo.activeFocus ? Theme.accent : (presetCombo.hovered ? Theme.accentHover : Theme.panelBorder)
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        contentItem: Text {
                            leftPadding: Theme.spacingMedium + 2
                            rightPadding: 24
                            text: presetCombo.displayText
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        indicator: VectorIcon {
                            x: presetCombo.width - width - Theme.spacingMedium - 2
                            y: Math.round((presetCombo.height - height) / 2)
                            width: 10
                            height: 10
                            name: "arrow_down"
                            color: presetCombo.hovered ? Theme.accentHover : Theme.textSecondary
                        }
                        popup: Popup {
                            y: presetCombo.height + 2
                            width: Math.max(presetCombo.width, 220)
                            implicitHeight: Math.min(260, Math.max(60, (contentItem.contentHeight || 0) + topPadding + bottomPadding + 8))
                            padding: 4
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: presetCombo.delegateModel
                                currentIndex: presetCombo.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                            }
                            background: Rectangle {
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                            }
                        }
                        delegate: ItemDelegate {
                            width: ListView.view ? ListView.view.width : presetCombo.width
                            implicitHeight: 30
                            highlighted: presetCombo.highlightedIndex === index
                            contentItem: Text {
                                text: modelData
                                color: highlighted ? Theme.accentHover : Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSmall
                                font.family: Theme.fontFamily
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: highlighted ? Theme.selection : "transparent"
                                radius: Theme.cornerRadiusSmall
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Text {
                        text: "Output Channels:"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textPrimary
                        Layout.leftMargin: 8
                    }

                    ComboBox {
                        id: channelsCombo
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        model: ["1 (Mono)", "2 (Stereo)", "6 (5.1 Surround)", "8 (7.1 Surround)"]
                        currentIndex: {
                            var ch = bridge.outputChannels;
                            if (ch === 1) return 0;
                            if (ch === 2) return 1;
                            if (ch === 6) return 2;
                            if (ch === 8) return 3;
                            return 1;
                        }
                        onActivated: function(index) {
                            var map = [1, 2, 6, 8];
                            bridge.setOutputChannels(map[index]);
                        }
                        background: Rectangle {
                            implicitWidth: 160
                            implicitHeight: 30
                            color: channelsCombo.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: channelsCombo.activeFocus ? Theme.accent : (channelsCombo.hovered ? Theme.accentHover : Theme.panelBorder)
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        contentItem: Text {
                            leftPadding: Theme.spacingMedium + 2
                            rightPadding: 24
                            text: channelsCombo.displayText
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        indicator: VectorIcon {
                            x: channelsCombo.width - width - Theme.spacingMedium - 2
                            y: Math.round((channelsCombo.height - height) / 2)
                            width: 10
                            height: 10
                            name: "arrow_down"
                            color: channelsCombo.hovered ? Theme.accentHover : Theme.textSecondary
                        }
                        popup: Popup {
                            y: channelsCombo.height + 2
                            width: Math.max(channelsCombo.width, 160)
                            implicitHeight: Math.min(260, Math.max(60, (contentItem.contentHeight || 0) + topPadding + bottomPadding + 8))
                            padding: 4
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: channelsCombo.delegateModel
                                currentIndex: channelsCombo.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                            }
                            background: Rectangle {
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                            }
                        }
                        delegate: ItemDelegate {
                            width: ListView.view ? ListView.view.width : channelsCombo.width
                            implicitHeight: 30
                            highlighted: channelsCombo.highlightedIndex === index
                            contentItem: Text {
                                text: modelData
                                color: highlighted ? Theme.accentHover : Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSmall
                                font.family: Theme.fontFamily
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: highlighted ? Theme.selection : "transparent"
                                radius: Theme.cornerRadiusSmall
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Normalize"
                        implicitHeight: 30
                        implicitWidth: 84
                        ToolTip.visible: hovered
                        ToolTip.text: "Scales coefficients to prevent clipping"
                        onClicked: bridge.normalizeChannelMatrix()
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: parent.hovered ? Theme.accentHover : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Button {
                        text: "Reset"
                        implicitHeight: 30
                        implicitWidth: 84
                        ToolTip.visible: hovered
                        ToolTip.text: "Reset matrix to direct 1:1 identity"
                        onClicked: bridge.resetChannelMatrixToIdentity()
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: parent.hovered ? Theme.accentHover : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            // Main Matrix Grid Area
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Item {
                    width: Math.max(parent.width, 680)
                    height: 400

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        // Column headers: Output channels (FL, FR, FC, LFE, RL, RR, SL, SR)
                        Row {
                            spacing: 4
                            // Top-left corner spacer
                            Rectangle {
                                width: 72
                                height: 38
                                color: "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "IN \\ OUT"
                                    font.pixelSize: Theme.fontSizeSmall - 2
                                    font.bold: true
                                    color: Theme.textMuted
                                }
                            }

                            Repeater {
                                model: 8
                                Rectangle {
                                    width: 66
                                    height: 38
                                    radius: Theme.cornerRadiusSmall
                                    color: Theme.background
                                    border.color: Theme.panelBorder
                                    border.width: 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 2
                                        spacing: 1

                                        Text {
                                            text: bridge.channelName(index)
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: true
                                            color: Theme.accentHover
                                            Layout.alignment: Qt.AlignHCenter
                                        }

                                        // Mini output VU level meter bar
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 4
                                            radius: 2
                                            color: Theme.surfaceElevated

                                            Rectangle {
                                                width: parent.width * Math.min(1.0, (root.levels && root.levels.length > index ? root.levels[index] : 0.0))
                                                height: parent.height
                                                radius: 2
                                                color: width > parent.width * 0.9 ? "#EF4444" : Theme.accent
                                            }
                                        }
                                    }

                                    ToolTip.visible: colHover.containsMouse
                                    ToolTip.text: "Output: " + bridge.channelFullName(index)

                                    MouseArea {
                                        id: colHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                    }
                                }
                            }
                        }

                        // Matrix Rows: Input channels 0..7
                        Repeater {
                            model: 8
                            delegate: Row {
                                property int inIdx: index
                                spacing: 4

                                // Row header: Input channel
                                Rectangle {
                                    width: 72
                                    height: 38
                                    radius: Theme.cornerRadiusSmall
                                    color: Theme.background
                                    border.color: Theme.panelBorder
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 4

                                        Text {
                                            text: bridge.channelName(inIdx)
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: true
                                            color: Theme.textPrimary
                                        }

                                        // Mini input VU bar
                                        Rectangle {
                                            Layout.preferredWidth: 4
                                            Layout.fillHeight: true
                                            radius: 2
                                            color: Theme.surfaceElevated

                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                width: parent.width
                                                height: parent.height * Math.min(1.0, (root.levels && root.levels.length > inIdx ? root.levels[inIdx] : 0.0))
                                                radius: 2
                                                color: height > parent.height * 0.9 ? "#EF4444" : Theme.accent
                                            }
                                        }
                                    }

                                    ToolTip.visible: rowHover.containsMouse
                                    ToolTip.text: "Input: " + bridge.channelFullName(inIdx)

                                    MouseArea {
                                        id: rowHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                    }
                                }

                                // Interactive Cells for each output channel (0..7)
                                Repeater {
                                    model: 8
                                    delegate: Rectangle {
                                        property int outIdx: index
                                        property real gainVal: {
                                            if (root.matrixData && root.matrixData.length > outIdx && root.matrixData[outIdx].length > inIdx) {
                                                return root.matrixData[outIdx][inIdx];
                                            }
                                            return 0.0;
                                        }

                                        width: 66
                                        height: 38
                                        radius: Theme.cornerRadiusSmall
                                        color: {
                                            if (gainVal > 0.01) {
                                                return Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Math.min(0.85, 0.15 + gainVal * 0.45));
                                            }
                                            return cellArea.containsMouse ? Theme.surfaceElevated : Theme.background;
                                        }
                                        border.color: cellArea.containsMouse ? Theme.accent : Theme.panelBorder
                                        border.width: cellArea.containsMouse ? 1.5 : 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: gainVal > 0.001 ? gainVal.toFixed(2) : "—"
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: gainVal > 0.001
                                            color: gainVal > 0.001 ? Theme.textPrimary : Theme.textMuted
                                        }

                                        ToolTip.visible: cellArea.containsMouse
                                        ToolTip.text: bridge.channelName(inIdx) + " → " + bridge.channelName(outIdx) +
                                                      " (Gain: " + gainVal.toFixed(2) + ")\nClick to toggle | Right-click to edit"

                                        MouseArea {
                                            id: cellArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                                            cursorShape: Qt.PointingHandCursor

                                            onClicked: function(mouse) {
                                                if (mouse.button === Qt.LeftButton) {
                                                    // Cycle preset gains: 0.0 -> 0.71 -> 1.0 -> 0.0
                                                    var current = gainVal;
                                                    var next = 0.0;
                                                    if (current < 0.05) next = 0.7071;
                                                    else if (current < 0.85) next = 1.0;
                                                    else next = 0.0;
                                                    bridge.setChannelMatrixCell(outIdx, inIdx, next);
                                                } else if (mouse.button === Qt.RightButton) {
                                                    root.selectedOutCh = outIdx;
                                                    root.selectedInCh = inIdx;
                                                    gainSliderPopup.targetOut = outIdx;
                                                    gainSliderPopup.targetIn = inIdx;
                                                    gainSliderPopup.currentVal = gainVal;
                                                    gainSliderPopup.open();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            // Footer note
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 36
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    spacing: 12

                    VectorIcon {
                        name: "info"
                        width: 14
                        height: 14
                        color: Theme.accentHover
                    }

                    Text {
                        text: "ITU-R BS.775 surround downmix & psychoacoustic matrix. Click cells to step gain (0.0 / 0.71 / 1.0) or right-click for precision slider."
                        font.pixelSize: Theme.fontSizeSmall - 1
                        color: Theme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Done"
                        implicitHeight: 32
                        implicitWidth: 84
                        onClicked: root.close()
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceElevated : Theme.selection
                            border.color: parent.hovered ? Theme.accentHover : Theme.accent
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
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

        // Inline gain slider popup for right-click precision editing
        Popup {
            id: gainSliderPopup
            property int targetOut: -1
            property int targetIn: -1
            property real currentVal: 0.0

            modal: true
            focus: true
            width: 220
            height: 120
            x: Math.round((parent.width - width) / 2)
            y: Math.round((parent.height - height) / 2)

            background: Rectangle {
                color: Theme.surfaceElevated
                border.color: Theme.accent
                border.width: 1.5
                radius: Theme.cornerRadiusMedium
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: (gainSliderPopup.targetIn >= 0 && gainSliderPopup.targetOut >= 0)
                          ? bridge.channelName(gainSliderPopup.targetIn) + " → " + bridge.channelName(gainSliderPopup.targetOut) + " Gain"
                          : "Gain"
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textPrimary
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Slider {
                        id: precisionSlider
                        Layout.fillWidth: true
                        from: 0.0
                        to: 2.0
                        stepSize: 0.01
                        value: gainSliderPopup.currentVal
                        onMoved: {
                            bridge.setChannelMatrixCell(gainSliderPopup.targetOut, gainSliderPopup.targetIn, value);
                        }
                    }

                    Text {
                        text: precisionSlider.value.toFixed(2)
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.accentHover
                        Layout.preferredWidth: 35
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: "Mute (0)"
                        Layout.fillWidth: true
                        onClicked: {
                            bridge.setChannelMatrixCell(gainSliderPopup.targetOut, gainSliderPopup.targetIn, 0.0);
                            gainSliderPopup.close();
                        }
                    }
                    Button {
                        text: "-3dB (0.71)"
                        Layout.fillWidth: true
                        onClicked: {
                            bridge.setChannelMatrixCell(gainSliderPopup.targetOut, gainSliderPopup.targetIn, 0.7071);
                            gainSliderPopup.close();
                        }
                    }
                    Button {
                        text: "Unity (1.0)"
                        Layout.fillWidth: true
                        onClicked: {
                            bridge.setChannelMatrixCell(gainSliderPopup.targetOut, gainSliderPopup.targetIn, 1.0);
                            gainSliderPopup.close();
                        }
                    }
                }
            }
        }

        ResizeGrip {
            target: root
            minWidth: 720
            minHeight: 500
        }
    }
}
