import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Dialog {
    id: arrangePanelsDialog
    title: qsTr("Panels Configuration (\"Arrange Panels\")")
    modal: true
    width: Math.min(1040, parent ? Math.max(820, parent.width - 48) : 1040)
    height: Math.min(700, parent ? Math.max(540, parent.height - 48) : 700)
    anchors.centerIn: parent
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside

    property string selectedZoneId: "leftSidebar"
    property var zonesList: bridge.layoutModel ? bridge.layoutModel.getZones() : []
    property var currentZoneElements: bridge.layoutModel ? bridge.layoutModel.getElementsForZone(selectedZoneId) : []
    property var availableElements: bridge.layoutModel ? bridge.layoutModel.getAllAvailableElements() : []

    readonly property string selectedZoneName: {
        if (!zonesList) return selectedZoneId;
        for (var i = 0; i < zonesList.length; ++i) {
            if (zonesList[i].zoneId === selectedZoneId) {
                return zonesList[i].name;
            }
        }
        return selectedZoneId;
    }

    function isModeConfigurable(zoneId) {
        return zoneId !== "headerBar" && zoneId !== "mainPanel" && zoneId !== "floatingWindow";
    }

    function refreshData() {
        if (!bridge || !bridge.layoutModel) return;
        zonesList = bridge.layoutModel.getZones();
        currentZoneElements = bridge.layoutModel.getElementsForZone(selectedZoneId);
        availableElements = bridge.layoutModel.getAllAvailableElements();
    }

    Connections {
        target: bridge.layoutModel
        function onLayoutChanged() {
            arrangePanelsDialog.refreshData();
        }
    }

    component StyledButton: Button {
        id: btnControl
        implicitHeight: Math.round(30 * Theme.uiScale)
        leftPadding: Math.round(Theme.spacingMedium * Theme.uiScale)
        rightPadding: Math.round(Theme.spacingMedium * Theme.uiScale)
        topPadding: Math.round(Theme.spacingSmall * Theme.uiScale)
        bottomPadding: Math.round(Theme.spacingSmall * Theme.uiScale)
        hoverEnabled: true

        contentItem: Text {
            text: btnControl.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.bold: btnControl.highlighted
            color: btnControl.highlighted ? "#ffffff" : (btnControl.hovered ? Theme.accentHover : Theme.textPrimary)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: btnControl.highlighted ? Theme.accent : (btnControl.hovered ? Theme.surfaceElevated : Theme.background)
            border.color: btnControl.highlighted ? Theme.accentHover : (btnControl.hovered ? Theme.accent : Theme.panelBorder)
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.NoButton
        }
    }

    component StyledCheckBox: CheckBox {
        id: cbControl
        implicitHeight: Math.round(26 * Theme.uiScale)
        hoverEnabled: true

        indicator: Rectangle {
            implicitWidth: Math.round(16 * Theme.uiScale)
            implicitHeight: Math.round(16 * Theme.uiScale)
            x: cbControl.leftPadding
            y: Math.round((cbControl.height - height) / 2)
            radius: Theme.cornerRadiusSmall
            color: cbControl.checked ? Theme.accent : Theme.surfaceElevated
            border.color: cbControl.checked ? Theme.accent : (cbControl.hovered ? Theme.accentHover : Theme.panelBorder)
            border.width: 1

            Rectangle {
                width: Math.round(8 * Theme.uiScale)
                height: Math.round(8 * Theme.uiScale)
                anchors.centerIn: parent
                radius: 1
                color: "#ffffff"
                visible: cbControl.checked
            }
        }

        contentItem: Text {
            text: cbControl.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            color: cbControl.hovered ? Theme.textPrimary : Theme.textSecondary
            verticalAlignment: Text.AlignVCenter
            leftPadding: cbControl.indicator.width + Math.round(Theme.spacingSmall * Theme.uiScale)
            elide: Text.ElideRight
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.NoButton
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cardCornerRadius
    }

    header: Rectangle {
        color: Theme.surface
        height: Math.round(52 * Theme.uiScale)
        border.color: Theme.panelBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Math.round(Theme.panelPadding * Theme.uiScale)
            anchors.rightMargin: Math.round(Theme.panelPadding * Theme.uiScale)
            spacing: Math.round(Theme.spacingMedium * Theme.uiScale)

            Text {
                text: qsTr("Arrange Panels & Docking Zones")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                elide: Text.ElideRight
            }

            Text {
                text: qsTr("Layout Preset:")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSmall
            }

            StyledComboBox {
                id: presetCombo
                Layout.preferredWidth: Math.round(180 * Theme.uiScale)
                Layout.preferredHeight: Math.round(30 * Theme.uiScale)
                textRole: "name"
                model: bridge.layoutModel ? bridge.layoutModel.getAvailablePresets() : []
                currentIndex: {
                    var presets = model;
                    var cur = bridge.layoutModel ? bridge.layoutModel.currentPreset : "studio";
                    for (var i = 0; i < presets.length; ++i) {
                        if (presets[i].id === cur) return i;
                    }
                    return 0;
                }
                onActivated: function(index) {
                    var p = model[index];
                    if (p && bridge) {
                        bridge.applyLayoutPreset(p.id);
                        arrangePanelsDialog.refreshData();
                    }
                }
            }

            StyledButton {
                text: qsTr("Reset Layout")
                implicitHeight: Math.round(30 * Theme.uiScale)
                onClicked: {
                    if (bridge && bridge.layoutModel) {
                        bridge.layoutModel.resetToDefaultLayout();
                        arrangePanelsDialog.refreshData();
                    }
                }
            }
        }
    }

    contentItem: Item {
        clip: true

        RowLayout {
            anchors.fill: parent
            anchors.margins: Math.round(Theme.spacingMedium * Theme.uiScale)
            spacing: Math.round(Theme.spacingMedium * Theme.uiScale)

            // Column 1: Zones Selector (Left)
            Rectangle {
                Layout.preferredWidth: Math.round(210 * Theme.uiScale)
                Layout.minimumWidth: Math.round(180 * Theme.uiScale)
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(8 * Theme.uiScale)
                    spacing: Math.round(6 * Theme.uiScale)

                    Text {
                        text: qsTr("Structural Docking Zones")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Bold
                    }

                    ListView {
                        id: zonesListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: arrangePanelsDialog.zonesList
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            id: zoneItem
                            required property var modelData
                            width: zonesListView.width
                            height: Math.round(38 * Theme.uiScale)
                            radius: Math.round(4 * Theme.uiScale)
                            color: arrangePanelsDialog.selectedZoneId === modelData.zoneId ?
                                   Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15) :
                                   (zoneHover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                            border.color: arrangePanelsDialog.selectedZoneId === modelData.zoneId ? Theme.accent : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(8 * Theme.uiScale)
                                anchors.rightMargin: Math.round(8 * Theme.uiScale)
                                spacing: Math.round(6 * Theme.uiScale)

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: zoneItem.modelData.name
                                    color: arrangePanelsDialog.selectedZoneId === zoneItem.modelData.zoneId ? Theme.accent : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.weight: arrangePanelsDialog.selectedZoneId === zoneItem.modelData.zoneId ? Font.DemiBold : Font.Normal
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    Layout.preferredWidth: Math.round(22 * Theme.uiScale)
                                    Layout.preferredHeight: Math.round(18 * Theme.uiScale)
                                    radius: Math.round(9 * Theme.uiScale)
                                    color: arrangePanelsDialog.selectedZoneId === zoneItem.modelData.zoneId ? Theme.accent : Qt.rgba(1, 1, 1, 0.08)

                                    Text {
                                        anchors.centerIn: parent
                                        text: zoneItem.modelData.elementCount
                                        color: arrangePanelsDialog.selectedZoneId === zoneItem.modelData.zoneId ? "#ffffff" : Theme.textSecondary
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                    }
                                }
                            }

                            HoverHandler { id: zoneHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: {
                                    arrangePanelsDialog.selectedZoneId = zoneItem.modelData.zoneId;
                                    arrangePanelsDialog.refreshData();
                                }
                            }
                        }
                    }
                }
            }

            // Column 2: Selected Zone Details & Assigned Elements (Center)
            Rectangle {
                Layout.fillWidth: true
                Layout.minimumWidth: Math.round(340 * Theme.uiScale)
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * Theme.uiScale)
                    spacing: Math.round(10 * Theme.uiScale)

                    // Zone Header & Mode controls
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Math.round(Theme.spacingSmall * Theme.uiScale)

                        Text {
                            text: qsTr("Zone: %1").arg(arrangePanelsDialog.selectedZoneName)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBase
                            font.bold: true
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            spacing: Math.round(Theme.spacingSmall * Theme.uiScale)
                            visible: arrangePanelsDialog.isModeConfigurable(arrangePanelsDialog.selectedZoneId)

                            Text {
                                text: qsTr("Display Mode:")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                            }

                            StyledComboBox {
                                id: zoneModeCombo
                                Layout.preferredWidth: Math.round(130 * Theme.uiScale)
                                Layout.preferredHeight: Math.round(28 * Theme.uiScale)
                                textRole: "text"
                                model: (arrangePanelsDialog.selectedZoneId.indexOf("Sidebar") !== -1) ?
                                       [
                                           { text: qsTr("Permanent"), value: "permanent" },
                                           { text: qsTr("Auto-Open"), value: "auto_open" },
                                           { text: qsTr("Hidden"), value: "hidden" }
                                       ] :
                                       [
                                           { text: qsTr("Visible"), value: "visible" },
                                           { text: qsTr("Hidden"), value: "hidden" }
                                       ]
                                currentIndex: {
                                    var curMode = bridge.layoutModel ? bridge.layoutModel.getZoneMode(arrangePanelsDialog.selectedZoneId) : "";
                                    for (var i = 0; i < model.length; ++i) {
                                        if (model[i].value === curMode) return i;
                                    }
                                    return 0;
                                }
                                onActivated: function(index) {
                                    if (bridge && bridge.layoutModel) {
                                        bridge.layoutModel.setZoneMode(arrangePanelsDialog.selectedZoneId, model[index].value);
                                        arrangePanelsDialog.refreshData();
                                    }
                                }
                            }
                        }
                    }

                    // Assigned Elements List Header
                    Text {
                        text: qsTr("Assigned UI Elements:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.DemiBold
                    }

                    // Container for Assigned List or Empty Placeholder
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: assignedListView
                            anchors.fill: parent
                            clip: true
                            model: arrangePanelsDialog.currentZoneElements
                            spacing: Math.round(6 * Theme.uiScale)
                            visible: arrangePanelsDialog.currentZoneElements && arrangePanelsDialog.currentZoneElements.length > 0
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: Rectangle {
                                id: assignedItem
                                required property var modelData
                                required property int index
                                width: assignedListView.width
                                implicitHeight: itemColLayout.implicitHeight + Math.round(14 * Theme.uiScale)
                                color: Theme.surface
                                border.color: Theme.panelBorder
                                border.width: 1
                                radius: Math.round(4 * Theme.uiScale)
                                clip: true

                                ColumnLayout {
                                    id: itemColLayout
                                    anchors.fill: parent
                                    anchors.margins: Math.round(7 * Theme.uiScale)
                                    spacing: Math.round(4 * Theme.uiScale)

                                    // Top Row: Title + Remove Button
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Math.round(8 * Theme.uiScale)

                                        Text {
                                            text: assignedItem.modelData.name
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeSmall
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                        }

                                        StyledButton {
                                            text: "✕"
                                            implicitWidth: Math.round(24 * Theme.uiScale)
                                            implicitHeight: Math.round(24 * Theme.uiScale)
                                            leftPadding: 0
                                            rightPadding: 0
                                            topPadding: 0
                                            bottomPadding: 0
                                            ToolTip.visible: hovered
                                            ToolTip.text: qsTr("Remove from zone")
                                            onClicked: {
                                                if (bridge && bridge.layoutModel) {
                                                    bridge.layoutModel.removeElement(assignedItem.modelData.elementId);
                                                    arrangePanelsDialog.refreshData();
                                                }
                                            }
                                        }
                                    }

                                    // Bottom Row: Stack controls
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Math.round(8 * Theme.uiScale)

                                        StyledCheckBox {
                                            id: stackCb
                                            text: qsTr("Stacked")
                                            checked: !!assignedItem.modelData.isStacked
                                            onToggled: {
                                                if (bridge && bridge.layoutModel) {
                                                    bridge.layoutModel.setElementStacked(assignedItem.modelData.elementId, checked, "sub_tabs");
                                                    arrangePanelsDialog.refreshData();
                                                }
                                            }
                                        }

                                        RowLayout {
                                            spacing: Math.round(6 * Theme.uiScale)
                                            visible: stackCb.checked
                                            Layout.fillWidth: true

                                            Text {
                                                text: qsTr("Mode:")
                                                color: Theme.textSecondary
                                                font.pixelSize: Theme.fontSizeSmall
                                            }

                                            StyledComboBox {
                                                Layout.fillWidth: true
                                                Layout.maximumWidth: Math.round(130 * Theme.uiScale)
                                                Layout.preferredHeight: Math.round(26 * Theme.uiScale)
                                                textRole: "text"
                                                model: [
                                                    { text: qsTr("Sub-Tabs"), value: "sub_tabs" },
                                                    { text: qsTr("Header Cycle"), value: "header_cycle" }
                                                ]
                                                currentIndex: assignedItem.modelData.stackMode === "header_cycle" ? 1 : 0
                                                onActivated: function(idx) {
                                                    if (bridge && bridge.layoutModel) {
                                                        bridge.layoutModel.setElementStacked(assignedItem.modelData.elementId, true, model[idx].value);
                                                        arrangePanelsDialog.refreshData();
                                                    }
                                                }
                                            }
                                        }

                                        Item { Layout.fillWidth: true }
                                    }
                                }
                            }
                        }

                        // Empty State Placeholder
                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: !arrangePanelsDialog.currentZoneElements || arrangePanelsDialog.currentZoneElements.length === 0
                            spacing: Math.round(6 * Theme.uiScale)

                            Text {
                                text: qsTr("No elements assigned to this zone")
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSizeSmall
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Text {
                                text: qsTr("Click '+' on an available widget to assign it")
                                color: Theme.textMuted
                                font.pixelSize: Math.round(11 * Theme.uiScale)
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            // Column 3: Available Modular Elements to Add (Right)
            Rectangle {
                Layout.preferredWidth: Math.round(250 * Theme.uiScale)
                Layout.minimumWidth: Math.round(200 * Theme.uiScale)
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(8 * Theme.uiScale)
                    spacing: Math.round(6 * Theme.uiScale)

                    Text {
                        text: qsTr("Available Widgets")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Bold
                    }

                    ListView {
                        id: availableListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: arrangePanelsDialog.availableElements
                        spacing: Math.round(4 * Theme.uiScale)
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            id: availItem
                            required property var modelData
                            width: availableListView.width
                            height: Math.round(36 * Theme.uiScale)
                            radius: Math.round(4 * Theme.uiScale)
                            color: Theme.surface
                            border.color: Theme.panelBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(8 * Theme.uiScale)
                                anchors.rightMargin: Math.round(8 * Theme.uiScale)
                                spacing: Math.round(6 * Theme.uiScale)

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: availItem.modelData.name
                                    color: Theme.textPrimary
                                    font.pixelSize: Math.round(11 * Theme.uiScale)
                                    elide: Text.ElideRight
                                }

                                StyledButton {
                                    text: "+"
                                    implicitWidth: Math.round(24 * Theme.uiScale)
                                    implicitHeight: Math.round(24 * Theme.uiScale)
                                    leftPadding: 0
                                    rightPadding: 0
                                    topPadding: 0
                                    bottomPadding: 0
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Add to %1").arg(arrangePanelsDialog.selectedZoneName)
                                    onClicked: {
                                        if (bridge && bridge.layoutModel) {
                                            bridge.layoutModel.assignElement(availItem.modelData.elementId, arrangePanelsDialog.selectedZoneId);
                                            arrangePanelsDialog.refreshData();
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

    footer: Rectangle {
        color: Theme.surface
        height: Math.round(52 * Theme.uiScale)
        border.color: Theme.panelBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Math.round(Theme.panelPadding * Theme.uiScale)
            anchors.rightMargin: Math.round(36 * Theme.uiScale)
            spacing: Math.round(Theme.spacingMedium * Theme.uiScale)

            Item { Layout.fillWidth: true }

            StyledButton {
                text: qsTr("Done")
                highlighted: true
                implicitWidth: Math.round(100 * Theme.uiScale)
                implicitHeight: Math.round(32 * Theme.uiScale)
                onClicked: {
                    if (bridge && bridge.layoutModel) {
                        bridge.layoutModel.saveLayoutProfile("default");
                    }
                    arrangePanelsDialog.close();
                }
            }
        }
    }
}
