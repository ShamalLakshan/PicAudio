import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: tabsBarRoot
    color: Theme.surface
    height: Math.round(36 * Theme.uiScale)
    implicitHeight: Math.round(36 * Theme.uiScale)

    signal tabClicked(int index)
    signal newTabRequested()

    // Keyboard shortcuts
    Shortcut {
        sequences: ["Ctrl+T"]
        onActivated: bridge.tabModel.openTab("library://music", "Music")
    }
    Shortcut {
        sequences: ["Ctrl+W"]
        onActivated: {
            if (bridge.tabModel.tabCount > 1) {
                bridge.tabModel.closeTab(bridge.tabModel.activeTabIndex, false)
            }
        }
    }
    Shortcut {
        sequences: ["Ctrl+Shift+T"]
        onActivated: {
            if (bridge.tabModel.canReopenClosedTab) {
                bridge.tabModel.reopenLastClosedTab()
            }
        }
    }
    Shortcut { sequence: "Ctrl+1"; onActivated: bridge.tabModel.setActiveTabIndex(0) }
    Shortcut { sequence: "Ctrl+2"; onActivated: bridge.tabModel.setActiveTabIndex(1) }
    Shortcut { sequence: "Ctrl+3"; onActivated: bridge.tabModel.setActiveTabIndex(2) }
    Shortcut { sequence: "Ctrl+4"; onActivated: bridge.tabModel.setActiveTabIndex(3) }
    Shortcut { sequence: "Ctrl+5"; onActivated: bridge.tabModel.setActiveTabIndex(4) }
    Shortcut { sequence: "Ctrl+6"; onActivated: bridge.tabModel.setActiveTabIndex(5) }
    Shortcut { sequence: "Ctrl+7"; onActivated: bridge.tabModel.setActiveTabIndex(6) }
    Shortcut { sequence: "Ctrl+8"; onActivated: bridge.tabModel.setActiveTabIndex(7) }
    Shortcut { sequence: "Ctrl+9"; onActivated: bridge.tabModel.setActiveTabIndex(8) }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Scrollable Tab Strip
        ScrollView {
            id: tabScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Row {
                id: tabRow
                height: tabsBarRoot.height
                spacing: 1

                Repeater {
                    id: tabRepeater
                    model: bridge.tabModel

                    delegate: Rectangle {
                        id: tabItem
                        required property int index
                        required property string tabId
                        required property string title
                        required property string sourceId
                        required property string viewId
                        required property int lockMode
                        required property bool isPinned
                        required property bool isActive

                        width: isPinned ? Math.round(54 * Theme.uiScale) : Math.max(Math.round(120 * Theme.uiScale), Math.min(Math.round(220 * Theme.uiScale), tabTitleText.implicitWidth + Math.round(68 * Theme.uiScale)))
                        height: tabsBarRoot.height
                        color: isActive ? Theme.background : (tabHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
                        border.color: isActive ? Theme.panelBorder : "transparent"
                        border.width: 1

                        // Top accent line for active tab
                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 2
                            color: tabItem.isActive ? Theme.accent : "transparent"
                            visible: tabItem.isActive
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Math.round(8 * Theme.uiScale)
                            anchors.rightMargin: Math.round(6 * Theme.uiScale)
                            spacing: Math.round(4 * Theme.uiScale)

                            // Lock / Pin Status Icon
                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                visible: tabItem.isPinned || tabItem.lockMode > 0
                                text: tabItem.isPinned ? "📌" : (tabItem.lockMode === 2 ? "🔒" : "🔒")
                                font.pixelSize: Math.round(11 * Theme.uiScale)
                                color: Theme.accent
                            }

                            // Tab Title
                            Text {
                                id: tabTitleText
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: tabItem.title
                                color: tabItem.isActive ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: tabItem.isActive ? Font.DemiBold : Font.Normal
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            // Close Button (hidden if pinned or locked)
                            Rectangle {
                                id: closeBtn
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: Math.round(18 * Theme.uiScale)
                                Layout.preferredHeight: Math.round(18 * Theme.uiScale)
                                radius: Math.round(9 * Theme.uiScale)
                                color: closeMouse.hovered ? Qt.rgba(1, 0.3, 0.3, 0.2) : "transparent"
                                visible: !tabItem.isPinned && tabItem.lockMode === 0 && bridge.tabModel.tabCount > 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    color: closeMouse.hovered ? "#ff6b6b" : Theme.textSecondary
                                }

                                HoverHandler { id: closeMouse; cursorShape: Qt.PointingHandCursor }

                                TapHandler {
                                    onTapped: bridge.tabModel.closeTab(tabItem.index, false)
                                }
                            }
                        }

                        HoverHandler {
                            id: tabHover
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                            onTapped: function(eventPoint, button) {
                                if (button === Qt.MiddleButton) {
                                    if (!tabItem.isPinned && tabItem.lockMode === 0 && bridge.tabModel.tabCount > 1) {
                                        bridge.tabModel.closeTab(tabItem.index, false)
                                    }
                                } else {
                                    bridge.tabModel.setActiveTabIndex(tabItem.index)
                                    tabsBarRoot.tabClicked(tabItem.index)
                                }
                            }
                        }

                        // Right-Click Context Menu
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: tabContextMenu.open()
                        }

                        Menu {
                            id: tabContextMenu
                            MenuItem {
                                text: "Duplicate Tab"
                                onTriggered: bridge.tabModel.duplicateTab(tabItem.index)
                            }
                            MenuItem {
                                text: tabItem.lockMode === 0 ? "Lock Tab" : "Unlock Tab"
                                onTriggered: {
                                    if (tabItem.lockMode === 0) {
                                        bridge.tabModel.setTabLockMode(tabItem.index, 1)
                                    } else {
                                        bridge.tabModel.unlockTab(tabItem.index)
                                    }
                                }
                            }
                            MenuItem {
                                text: tabItem.isPinned ? "Unpin Tab" : "Pin Tab"
                                onTriggered: bridge.tabModel.setTabPinned(tabItem.index, !tabItem.isPinned)
                            }
                            MenuItem {
                                text: "Set as Root Lock..."
                                onTriggered: bridge.tabModel.setTabRootLock(tabItem.index, tabItem.sourceId)
                            }
                            MenuSeparator {}
                            MenuItem {
                                text: "Close Tab\tCtrl+W"
                                enabled: !tabItem.isPinned && tabItem.lockMode === 0 && bridge.tabModel.tabCount > 1
                                onTriggered: bridge.tabModel.closeTab(tabItem.index, false)
                            }
                            MenuItem {
                                text: "Close Other Tabs"
                                enabled: bridge.tabModel.tabCount > 1
                                onTriggered: bridge.tabModel.closeOtherTabs(tabItem.index)
                            }
                            MenuItem {
                                text: "Reopen Closed Tab\tCtrl+Shift+T"
                                enabled: bridge.tabModel.canReopenClosedTab
                                onTriggered: bridge.tabModel.reopenLastClosedTab()
                            }
                        }
                    }
                }
            }
        }

        // New Tab (+) Button
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Math.round(30 * Theme.uiScale)
            Layout.preferredHeight: Math.round(30 * Theme.uiScale)
            color: newTabHover.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
            radius: Math.round(4 * Theme.uiScale)

            Text {
                anchors.centerIn: parent
                text: "+"
                font.pixelSize: Math.round(18 * Theme.uiScale)
                font.bold: true
                color: Theme.textSecondary
            }

            HoverHandler { id: newTabHover; cursorShape: Qt.PointingHandCursor }

            TapHandler {
                onTapped: {
                    bridge.tabModel.openTab("library://music", "Music")
                    tabsBarRoot.newTabRequested()
                }
            }
        }

        // Configure Layout Button
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Math.round(30 * Theme.uiScale)
            Layout.preferredHeight: Math.round(30 * Theme.uiScale)
            Layout.rightMargin: Math.round(4 * Theme.uiScale)
            color: configLayoutHover.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
            radius: Math.round(4 * Theme.uiScale)

            Text {
                anchors.centerIn: parent
                text: "☵"
                font.pixelSize: Math.round(15 * Theme.uiScale)
                color: Theme.textSecondary
            }

            HoverHandler { id: configLayoutHover; cursorShape: Qt.PointingHandCursor }

            TapHandler {
                onTapped: {
                    if (arrangePanelsDialogInstance) {
                        arrangePanelsDialogInstance.open()
                    }
                }
            }
        }
    }
}
