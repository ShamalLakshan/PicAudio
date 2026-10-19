import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property int mainViewMode: 1
    property bool showLeftPanel: true
    property bool showRightPanel: true

    property alias isSearchActiveFocus: searchTextInput.activeFocus
    property alias searchText: searchTextInput.text

    signal requestSwitchViewMode(int mode)
    signal requestSwitchToExpandedView()
    signal requestSwitchToNowPlaying()
    signal requestOpenFolderDialog()
    signal requestPreferences()
    signal toggleLeftPanel()
    signal toggleRightPanel()

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

        // Navigation Tabs
        RowLayout {
            spacing: 2

            Repeater {
                model: [
                    { name: "ALL TRACKS", icon: "table", mode: 1 },
                    { name: "ALBUMS", icon: "grid", mode: 0 },
                    { name: "EXPANDED", icon: "album_tracks", mode: 2 },
                    { name: "NOW PLAYING", icon: "disc", mode: 4 }
                ]

                delegate: Rectangle {
                    property bool isTabActive: root.mainViewMode === modelData.mode

                    Layout.preferredWidth: tabRowLayout.implicitWidth + 16
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignVCenter
                    radius: 3
                    color: isTabActive ? Theme.selection : (tabMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                    border.color: isTabActive ? Theme.panelBorder : "transparent"
                    border.width: 1
                    clip: true

                    RowLayout {
                        id: tabRowLayout
                        anchors.centerIn: parent
                        spacing: 6

                        VectorIcon {
                            name: modelData.icon
                            width: 12
                            height: 12
                            color: isTabActive ? Theme.accent : Theme.textMuted
                        }

                        Text {
                            text: modelData.name
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8
                            color: isTabActive ? Theme.textPrimary : Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.mode === 1) {
                                root.requestSwitchViewMode(1);
                            } else if (modelData.mode === 0) {
                                root.requestSwitchViewMode(0);
                            } else if (modelData.mode === 2) {
                                root.requestSwitchToExpandedView();
                            } else if (modelData.mode === 4) {
                                root.requestSwitchToNowPlaying();
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 20; Layout.alignment: Qt.AlignVCenter; color: Theme.panelBorder }

        // Quick Playback Actions
        RowLayout {
            spacing: 4

            Rectangle {
                Layout.preferredWidth: playAllText.implicitWidth + 18
                Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                radius: 3
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
                Layout.preferredWidth: shuffleAllText.implicitWidth + 18
                Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                radius: 3
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
            radius: 3
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
            Layout.preferredWidth: scanBtnLayout.implicitWidth + 18
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignVCenter
            radius: 3
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
            spacing: 2
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: 3
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
                radius: 3
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

            // Settings Button (Ctrl+,)
            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: 3
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
