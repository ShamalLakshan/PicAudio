import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Dialog {
    id: viewConfigDialog
    title: qsTr("Customize Workspace View")
    modal: true
    width: Math.min(840, parent ? parent.width - 48 : 840)
    height: Math.min(640, parent ? parent.height - 48 : 640)
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside

    property string targetViewId: bridge.tabModel.activeViewId
    property var viewDef: bridge.viewRegistry.getView(targetViewId)
    property var allViews: bridge.viewRegistry.getAllViews()

    function refreshViewData() {
        allViews = bridge.viewRegistry.getAllViews()
        viewDef = bridge.viewRegistry.getView(targetViewId)
    }

    Connections {
        target: bridge.viewRegistry
        function onViewsChanged() {
            viewConfigDialog.refreshViewData()
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

            Text {
                text: qsTr("View Settings: %1").arg(viewConfigDialog.viewDef.name || targetViewId)
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Save as New Custom View...")
                onClicked: {
                    var newId = bridge.viewRegistry.duplicateView(targetViewId, qsTr("Custom %1").arg(viewConfigDialog.viewDef.name))
                    if (newId.length > 0) {
                        targetViewId = newId
                        bridge.tabModel.updateActiveView(newId)
                        viewConfigDialog.refreshViewData()
                    }
                }
            }
        }
    }

    contentItem: ScrollView {
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: viewConfigDialog.availableWidth
            spacing: Math.round(Theme.spacingMedium * Theme.uiScale)

            // Switch Active View
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(50 * Theme.uiScale)
                color: Theme.background
                border.color: Theme.panelBorder
                radius: Math.round(4 * Theme.uiScale)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(8 * Theme.uiScale)

                    Text {
                        text: qsTr("Switch View Template:")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    ComboBox {
                        Layout.preferredWidth: Math.round(220 * Theme.uiScale)
                        textRole: "name"
                        valueRole: "viewId"
                        model: viewConfigDialog.allViews
                        currentIndex: {
                            for (var i = 0; i < model.length; ++i) {
                                if (model[i].viewId === viewConfigDialog.targetViewId) return i
                            }
                            return 0
                        }
                        onActivated: function(index) {
                            var selId = model[index].viewId
                            viewConfigDialog.targetViewId = selId
                            bridge.tabModel.updateActiveView(selId)
                            viewConfigDialog.refreshViewData()
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("Delete Custom View")
                        visible: !viewConfigDialog.viewDef.isBuiltin
                        onClicked: {
                            bridge.viewRegistry.deleteCustomView(viewConfigDialog.targetViewId)
                            targetViewId = "tracks_default"
                            bridge.tabModel.updateActiveView("tracks_default")
                            viewConfigDialog.refreshViewData()
                        }
                    }
                }
            }

            // Section: View Configuration
            Rectangle {
                Layout.fillWidth: true
                color: Theme.background
                border.color: Theme.panelBorder
                radius: Math.round(4 * Theme.uiScale)
                Layout.preferredHeight: configColumn.implicitHeight + Math.round(24 * Theme.uiScale)

                ColumnLayout {
                    id: configColumn
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * Theme.uiScale)
                    spacing: Math.round(10 * Theme.uiScale)

                    Text {
                        text: qsTr("Grouping & Layout")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Bold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Group Tracks By:"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSmall }
                        ComboBox {
                            id: groupByCombo
                            Layout.preferredWidth: Math.round(200 * Theme.uiScale)
                            model: ["album_artist", "album", "year", "genre", "none"]
                            currentIndex: model.indexOf(viewConfigDialog.viewDef.groupBy || "album_artist")
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Artwork / Thumbnail Size: %1 px").arg(thumbSlider.value); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSmall }
                        Slider {
                            id: thumbSlider
                            Layout.fillWidth: true
                            from: 80
                            to: 320
                            stepSize: 10
                            value: viewConfigDialog.viewDef.thumbnailSize || 160
                        }
                    }
                }
            }

            // Section: Panel Layout Overrides
            Rectangle {
                Layout.fillWidth: true
                color: Theme.background
                border.color: Theme.panelBorder
                radius: Math.round(4 * Theme.uiScale)
                Layout.preferredHeight: overridesCol.implicitHeight + Math.round(24 * Theme.uiScale)

                ColumnLayout {
                    id: overridesCol
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * Theme.uiScale)
                    spacing: Math.round(8 * Theme.uiScale)

                    Text {
                        text: qsTr("Panel Layout Overrides (For this View)")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Bold
                    }

                    Text {
                        text: qsTr("These window panel settings apply dynamically whenever a tab using this view is focused:")
                        color: Theme.textSecondary
                        font.pixelSize: Math.round(11 * Theme.uiScale)
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: Math.round(6 * Theme.uiScale)
                        columnSpacing: Math.round(16 * Theme.uiScale)

                        CheckBox {
                            id: chkLeftSidebarHover
                            text: qsTr("Left Sidebar: Auto-Open on Edge Hover")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.leftSidebar === "auto_open")
                        }

                        CheckBox {
                            id: chkRightSidebarHover
                            text: qsTr("Right Sidebar: Auto-Open on Edge Hover")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.rightSidebar === "auto_open")
                        }

                        CheckBox {
                            id: chkHideLeftMain
                            text: qsTr("Collapse Left Main Panel")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.leftMainPanel === "hidden")
                        }

                        CheckBox {
                            id: chkHideRightMain
                            text: qsTr("Collapse Right Main Panel")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.rightMainPanel === "hidden")
                        }

                        CheckBox {
                            id: chkHideTopPanel
                            text: qsTr("Collapse Top Panel")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.topPanel === "hidden")
                        }

                        CheckBox {
                            id: chkHideColumnBrowser
                            text: qsTr("Hide In-Tab Column Browser")
                            checked: (viewConfigDialog.viewDef.panelOverrides && viewConfigDialog.viewDef.panelOverrides.columnBrowser === "hidden")
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
            text: qsTr("Apply & Close")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                var ov = {}
                if (chkLeftSidebarHover.checked) ov["leftSidebar"] = "auto_open"
                if (chkRightSidebarHover.checked) ov["rightSidebar"] = "auto_open"
                if (chkHideLeftMain.checked) ov["leftMainPanel"] = "hidden"
                if (chkHideRightMain.checked) ov["rightMainPanel"] = "hidden"
                if (chkHideTopPanel.checked) ov["topPanel"] = "hidden"
                if (chkHideColumnBrowser.checked) ov["columnBrowser"] = "hidden"

                if (!viewConfigDialog.viewDef.isBuiltin) {
                    bridge.viewRegistry.updateCustomView(
                        targetViewId,
                        viewConfigDialog.viewDef.name,
                        viewConfigDialog.viewDef.displayedColumns,
                        groupByCombo.currentText,
                        thumbSlider.value,
                        ov
                    )
                }

                // Apply overrides immediately to active layout
                bridge.layoutModel.applyLayoutOverrides(ov)
                viewConfigDialog.close()
            }
        }
    }
}
