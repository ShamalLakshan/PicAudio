import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Dialog {
    id: arrangePanelsDialog
    title: qsTr("Panels Configuration (\"Arrange Panels\")")
    modal: true
    width: Math.min(940, parent ? parent.width - 48 : 940)
    height: Math.min(680, parent ? parent.height - 48 : 680)
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside

    property string selectedZoneId: "leftSidebar"
    property var zonesList: bridge.layoutModel.getZones()
    property var currentZoneElements: bridge.layoutModel.getElementsForZone(selectedZoneId)
    property var availableElements: bridge.layoutModel.getAllAvailableElements()

    function refreshData() {
        zonesList = bridge.layoutModel.getZones()
        currentZoneElements = bridge.layoutModel.getElementsForZone(selectedZoneId)
        availableElements = bridge.layoutModel.getAllAvailableElements()
    }

    Connections {
        target: bridge.layoutModel
        function onLayoutChanged() {
            arrangePanelsDialog.refreshData()
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
            }

            Item { Layout.fillWidth: true }

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

            Button {
                text: qsTr("Reset Layout")
                onClicked: {
                    bridge.layoutModel.resetToDefaultLayout()
                    arrangePanelsDialog.refreshData()
                }
            }
        }
    }

    contentItem: Item {
        clip: true

        RowLayout {
            anchors.fill: parent
            anchors.margins: Math.round(Theme.spacingMedium * Theme.uiScale)
            spacing: Math.round(Theme.spacingLarge * Theme.uiScale)

            // Column 1: Zones Selector (Left)
            Rectangle {
                Layout.preferredWidth: Math.round(240 * Theme.uiScale)
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)

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

                        delegate: Rectangle {
                            id: zoneItem
                            required property var modelData
                            width: zonesListView.width
                            height: Math.round(42 * Theme.uiScale)
                            radius: Math.round(4 * Theme.uiScale)
                            color: arrangePanelsDialog.selectedZoneId === modelData.zoneId ? Qt.rgba(0.2, 0.6, 1.0, 0.15) : (zoneHover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                            border.color: arrangePanelsDialog.selectedZoneId === modelData.zoneId ? Theme.accent : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(8 * Theme.uiScale)
                                anchors.rightMargin: Math.round(8 * Theme.uiScale)

                                Text {
                                    Layout.fillWidth: true
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
                                    color: Qt.rgba(1, 1, 1, 0.08)

                                    Text {
                                        anchors.centerIn: parent
                                        text: zoneItem.modelData.elementCount
                                        color: Theme.textSecondary
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                    }
                                }
                            }

                            HoverHandler { id: zoneHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: {
                                    arrangePanelsDialog.selectedZoneId = zoneItem.modelData.zoneId
                                    arrangePanelsDialog.refreshData()
                                }
                            }
                        }
                    }
                }
            }

            // Column 2: Selected Zone Details & Assigned Elements (Center)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * Theme.uiScale)
                    spacing: Math.round(10 * Theme.uiScale)

                    // Zone Header & Mode controls
                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: qsTr("Zone: %1").arg(arrangePanelsDialog.selectedZoneId)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBase
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        // Mode Selector (Permanent vs Auto-Open Edge Hover vs Hidden)
                        Text {
                            text: qsTr("Display Mode:")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                            visible: arrangePanelsDialog.selectedZoneId.indexOf("Sidebar") !== -1 || arrangePanelsDialog.selectedZoneId.indexOf("Panel") !== -1
                        }

                        ComboBox {
                            id: zoneModeCombo
                            Layout.preferredWidth: Math.round(140 * Theme.uiScale)
                            model: (arrangePanelsDialog.selectedZoneId.indexOf("Sidebar") !== -1) ?
                                   ["permanent", "auto_open", "hidden"] : ["visible", "hidden"]
                            currentIndex: {
                                var m = bridge.layoutModel.getZoneMode(arrangePanelsDialog.selectedZoneId)
                                var idx = model.indexOf(m)
                                return idx >= 0 ? idx : 0
                            }
                            onActivated: function(index) {
                                bridge.layoutModel.setZoneMode(arrangePanelsDialog.selectedZoneId, model[index])
                                arrangePanelsDialog.refreshData()
                            }
                            visible: arrangePanelsDialog.selectedZoneId !== "headerBar" && arrangePanelsDialog.selectedZoneId !== "mainPanel"
                        }
                    }

                    // Assigned Elements List
                    Text {
                        text: qsTr("Assigned UI Elements:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        id: assignedListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: arrangePanelsDialog.currentZoneElements
                        spacing: Math.round(6 * Theme.uiScale)

                        delegate: Rectangle {
                            id: assignedItem
                            required property var modelData
                            required property int index
                            width: assignedListView.width
                            height: Math.round(48 * Theme.uiScale)
                            color: Theme.surface
                            border.color: Theme.panelBorder
                            border.width: 1
                            radius: Math.round(4 * Theme.uiScale)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(10 * Theme.uiScale)
                                anchors.rightMargin: Math.round(10 * Theme.uiScale)
                                spacing: Math.round(8 * Theme.uiScale)

                                Text {
                                    Layout.fillWidth: true
                                    text: assignedItem.modelData.name
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    elide: Text.ElideRight
                                }

                                // Stacking Checkbox
                                CheckBox {
                                    text: qsTr("Stacked")
                                    checked: assignedItem.modelData.isStacked
                                    onToggled: {
                                        bridge.layoutModel.setElementStacked(assignedItem.modelData.elementId, checked, "sub_tabs")
                                        arrangePanelsDialog.refreshData()
                                    }
                                }

                                // Stack Mode ComboBox (Sub-Tabs vs Header Cycle)
                                ComboBox {
                                    Layout.preferredWidth: Math.round(120 * Theme.uiScale)
                                    visible: assignedItem.modelData.isStacked
                                    model: ["sub_tabs", "header_cycle"]
                                    currentIndex: assignedItem.modelData.stackMode === "header_cycle" ? 1 : 0
                                    onActivated: function(idx) {
                                        bridge.layoutModel.setElementStacked(assignedItem.modelData.elementId, true, model[idx])
                                        arrangePanelsDialog.refreshData()
                                    }
                                }

                                // Remove Button
                                Button {
                                    text: "✕"
                                    Layout.preferredWidth: Math.round(30 * Theme.uiScale)
                                    onClicked: {
                                        bridge.layoutModel.removeElement(assignedItem.modelData.elementId)
                                        arrangePanelsDialog.refreshData()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Column 3: Available Modular Elements to Add (Right)
            Rectangle {
                Layout.preferredWidth: Math.round(240 * Theme.uiScale)
                Layout.fillHeight: true
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                radius: Math.round(4 * Theme.uiScale)

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

                                Text {
                                    Layout.fillWidth: true
                                    text: availItem.modelData.name
                                    color: Theme.textPrimary
                                    font.pixelSize: Math.round(11 * Theme.uiScale)
                                    elide: Text.ElideRight
                                }

                                Button {
                                    text: "+"
                                    Layout.preferredWidth: Math.round(24 * Theme.uiScale)
                                    Layout.preferredHeight: Math.round(24 * Theme.uiScale)
                                    onClicked: {
                                        bridge.layoutModel.assignElement(availItem.modelData.elementId, arrangePanelsDialog.selectedZoneId)
                                        arrangePanelsDialog.refreshData()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: Rectangle { color: Theme.surface; border.color: Theme.panelBorder; border.width: 1 }

        Button {
            text: qsTr("Done")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                bridge.layoutModel.saveLayoutProfile("default")
                arrangePanelsDialog.close()
            }
        }
    }
}
