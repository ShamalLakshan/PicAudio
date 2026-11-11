import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property bool showLeftPanel: true
    property bool showRightPanel: true

    property alias isSearchActiveFocus: searchTextInput.activeFocus
    property alias searchText: searchTextInput.text

    signal requestOpenFolderDialog()
    signal requestPreferences()
    signal toggleLeftPanel()
    signal toggleRightPanel()
    signal requestArrangePanels()
    signal requestCustomizeView()

    function focusSearch() {
        searchTextInput.forceActiveFocus();
        searchTextInput.selectAll();
    }

    function clearSearch() {
        searchTextInput.text = "";
        bridge.search("");
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 42
    color: Theme.surface

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        // Active Workspace / Tab Breadcrumb Badge
        Rectangle {
            Layout.preferredHeight: 28
            Layout.preferredWidth: tabBadgeLayout.implicitWidth + 20
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 2
            Layout.rightMargin: 2
            radius: Theme.cornerRadiusSmall
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1
            clip: true

            RowLayout {
                id: tabBadgeLayout
                anchors.centerIn: parent
                spacing: 6

                VectorIcon {
                    name: {
                        var src = bridge.tabModel ? bridge.tabModel.activeSourceId : "";
                        if (src.indexOf("folder://") === 0) return "folder";
                        if (src.indexOf("playlist://") === 0) return "disc";
                        if (src.indexOf("history") !== -1) return "history";
                        if (src.indexOf("favorites") !== -1) return "heart";
                        return "library";
                    }
                    width: 12
                    height: 12
                    color: Theme.accent
                }

                Text {
                    text: bridge.tabModel ? bridge.tabModel.activeTitle : "Music"
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.maximumWidth: 160
                }

                Text {
                    visible: bridge.tabModel && bridge.tabModel.activeIsPinned
                    text: "📌"
                    font.pixelSize: 9
                }

                Text {
                    visible: bridge.tabModel && bridge.tabModel.activeLockMode > 0
                    text: bridge.tabModel.activeLockMode === 2 ? "🔒R" : "🔒"
                    font.pixelSize: 9
                    color: Theme.accent
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 20; Layout.alignment: Qt.AlignVCenter; color: Theme.panelBorder }

        // Quick Playback Actions
        RowLayout {
            spacing: 4

            Rectangle {
                Layout.preferredWidth: playAllText.implicitWidth + 22
                Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 2
                Layout.rightMargin: 2
                radius: Theme.cornerRadiusSmall
                color: playAllMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                border.color: Theme.panelBorder
                clip: true

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    VectorIcon { name: "play"; width: 10; height: 10; color: Theme.textPrimary }
                    Text { id: playAllText; text: "Play All"; font.pixelSize: 11; font.bold: true; color: Theme.textPrimary }
                }

                MouseArea {
                    id: playAllMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: bridge.playAll()
                }
            }

            Rectangle {
                Layout.preferredWidth: shuffleAllText.implicitWidth + 22
                Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 2
                Layout.rightMargin: 2
                radius: Theme.cornerRadiusSmall
                color: shuffleAllMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                border.color: Theme.panelBorder
                clip: true

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    VectorIcon { name: "shuffle"; width: 11; height: 11; color: Theme.textSecondary }
                    Text { id: shuffleAllText; text: "Shuffle"; font.pixelSize: 11; color: Theme.textSecondary }
                }

                MouseArea {
                    id: shuffleAllMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: bridge.shuffleAll()
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Search input with focus handling
        Rectangle {
            Layout.preferredWidth: 250
            Layout.preferredHeight: 28
            Layout.minimumWidth: 140
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            radius: Theme.cornerRadiusSmall
            color: Theme.background
            border.color: searchTextInput.activeFocus ? Theme.accent : Theme.panelBorder
            border.width: 1
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                VectorIcon {
                    name: "search"
                    width: 11
                    height: 11
                    color: Theme.textMuted
                }

                FocusScope {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    focus: true

                    TextInput {
                        id: searchTextInput
                        anchors.fill: parent
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        clip: true
                        focus: true
                        onTextChanged: bridge.search(text)

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Search title, artist, album..."
                            color: Theme.textMuted
                            font.pixelSize: 11
                            visible: !searchTextInput.text && !searchTextInput.activeFocus
                        }
                    }
                }

                VectorIcon {
                    visible: searchTextInput.text.length > 0
                    name: "clear"
                    width: 10
                    height: 10
                    color: Theme.textMuted
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.clearSearch();
                        }
                    }
                }
            }
        }

        // Scan Directory Action Button
        Rectangle {
            Layout.preferredWidth: scanBtnLayout.implicitWidth + 22
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            radius: Theme.cornerRadiusSmall
            color: scanMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
            border.color: Theme.panelBorder
            clip: true

            RowLayout {
                id: scanBtnLayout
                anchors.centerIn: parent
                spacing: 6

                VectorIcon {
                    name: "folder"
                    width: 12
                    height: 12
                    color: Theme.accent
                }

                Text {
                    text: "Scan Directory..."
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.accentHover
                }
            }

            MouseArea {
                id: scanMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.requestOpenFolderDialog()
            }
        }

        // Panel Toggle Quick Buttons
        RowLayout {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                radius: Theme.cornerRadiusSmall
                color: root.showLeftPanel ? Theme.selection : (pnlLeftMouse.containsMouse ? Theme.surfaceElevated : Theme.surface)
                border.color: Theme.panelBorder
                VectorIcon { anchors.centerIn: parent; name: "panel_left"; width: 13; height: 13; color: root.showLeftPanel ? Theme.textPrimary : Theme.textMuted }
                MouseArea {
                    id: pnlLeftMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleLeftPanel()
                }
            }

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                radius: Theme.cornerRadiusSmall
                color: root.showRightPanel ? Theme.selection : (pnlRightMouse.containsMouse ? Theme.surfaceElevated : Theme.surface)
                border.color: Theme.panelBorder
                VectorIcon { anchors.centerIn: parent; name: "panel_right"; width: 13; height: 13; color: root.showRightPanel ? Theme.textPrimary : Theme.textMuted }
                MouseArea {
                    id: pnlRightMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleRightPanel()
                }
            }

            // Arrange Panels Button
            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                radius: Theme.cornerRadiusSmall
                color: arrangePanelsMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: Theme.panelBorder
                Text {
                    anchors.centerIn: parent
                    text: "☵"
                    font.pixelSize: 14
                    color: Theme.textSecondary
                }
                MouseArea {
                    id: arrangePanelsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestArrangePanels()
                }
                ToolTip.visible: arrangePanelsMouse.containsMouse
                ToolTip.text: qsTr("Panels Configuration (\"Arrange Panels\")")
            }

            // Customize View Button
            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                radius: Theme.cornerRadiusSmall
                color: viewConfigMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: Theme.panelBorder
                Text {
                    anchors.centerIn: parent
                    text: "⚙"
                    font.pixelSize: 14
                    color: Theme.textSecondary
                }
                MouseArea {
                    id: viewConfigMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestCustomizeView()
                }
                ToolTip.visible: viewConfigMouse.containsMouse
                ToolTip.text: qsTr("Customize Workspace View...")
            }

            // Settings Button (Ctrl+,)
            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                radius: Theme.cornerRadiusSmall
                color: settingsBtnMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: Theme.panelBorder
                VectorIcon { anchors.centerIn: parent; name: "settings"; width: 13; height: 13; color: Theme.textSecondary }
                MouseArea {
                    id: settingsBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestPreferences()
                }
                ToolTip.visible: settingsBtnMouse.containsMouse
                ToolTip.text: "Preferences & Settings (Ctrl+,)"
            }
        }
    }
}
