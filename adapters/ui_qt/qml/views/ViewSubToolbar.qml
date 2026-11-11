import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property bool showColumnBrowser: false
    property string activeLetterFilter: "All"

    signal toggleColumnBrowser()
    signal letterFilterSelected(string letter)
    signal requestCustomizeView()

    Layout.fillWidth: true
    Layout.preferredHeight: 34
    color: Theme.surfaceElevated
    border.color: Theme.panelBorder
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        // In-Tab View Mode Switcher
        RowLayout {
            spacing: 2
            Layout.alignment: Qt.AlignVCenter

            Repeater {
                model: [
                    { id: "tracks_default", name: "Tracks Table", icon: "table", tooltip: "Tracks Table View" },
                    { id: "covers_default", name: "Album Covers", icon: "grid", tooltip: "Album Covers Grid View" },
                    { id: "album_and_tracks_default", name: "Album & Tracks", icon: "album_tracks", tooltip: "Album & Tracks View" },
                    { id: "artists_default", name: "Artists", icon: "library", tooltip: "Artists Grid View" },
                    { id: "now_playing_default", name: "Now Playing", icon: "disc", tooltip: "Now Playing View" },
                    { id: "jukebox_default", name: "Jukebox", icon: "volume", tooltip: "Jukebox Presentation View" },
                    { id: "history_default", name: "History", icon: "history", tooltip: "Listening History View" }
                ]

                delegate: Rectangle {
                    id: vmBtn
                    property bool isCurrent: bridge.tabModel && (bridge.tabModel.activeViewId === modelData.id || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === bridge.viewRegistry.getBaseRendererType(modelData.id)))

                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    Layout.leftMargin: 1
                    Layout.rightMargin: 1
                    width: 28
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: isCurrent ? Theme.selection : (vmMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                    border.color: isCurrent ? Theme.accent : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: modelData.icon
                        width: 12
                        height: 12
                        color: vmBtn.isCurrent ? Theme.accent : Theme.textMuted
                    }

                    MouseArea {
                        id: vmMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (bridge.tabModel) {
                                bridge.tabModel.updateActiveView(modelData.id);
                            }
                        }
                    }

                    ToolTip.visible: vmMouse.containsMouse
                    ToolTip.text: modelData.tooltip
                }
            }

            // View modes dropdown (custom views + config)
            Rectangle {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 24
                radius: Theme.cornerRadiusSmall
                color: moreViewsMouse.containsMouse ? Theme.surfaceElevated : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "▾"
                    font.pixelSize: 10
                    color: Theme.textMuted
                }

                MouseArea {
                    id: moreViewsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: viewsMenu.popup()
                }

                ToolTip.visible: moreViewsMouse.containsMouse
                ToolTip.text: qsTr("More Views & Custom Views...")

                Menu {
                    id: viewsMenu
                    Instantiator {
                        model: bridge.viewRegistry ? bridge.viewRegistry.getAllViews() : []
                        onObjectAdded: (index, object) => viewsMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => viewsMenu.removeItem(object)
                        delegate: MenuItem {
                            text: modelData.name
                            checkable: true
                            checked: bridge.tabModel && bridge.tabModel.activeViewId === modelData.viewId
                            onTriggered: {
                                if (bridge.tabModel) {
                                    bridge.tabModel.updateActiveView(modelData.viewId);
                                }
                            }
                        }
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Customize Workspace View...")
                        onTriggered: root.requestCustomizeView()
                    }
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; width: 1; height: 16; color: Theme.panelBorder }

        // Toggle Category Filters Button
        Rectangle {
            Layout.preferredWidth: colBtnLayout.implicitWidth + 18
            Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 4
            Layout.rightMargin: 2
            width: Layout.preferredWidth
            height: 24
            radius: Theme.cornerRadiusSmall
            color: root.showColumnBrowser ? Theme.selection : (cbMouse.containsMouse ? Theme.surfaceElevated : "transparent")
            border.color: root.showColumnBrowser ? Theme.accent : Theme.panelBorder

            RowLayout {
                id: colBtnLayout
                anchors.centerIn: parent
                spacing: 4

                VectorIcon {
                    name: "filter"
                    width: 10
                    height: 10
                    color: root.showColumnBrowser ? Theme.accent : Theme.textMuted
                }

                Text {
                    text: "Filters"
                    font.pixelSize: 10
                    font.bold: root.showColumnBrowser
                    color: root.showColumnBrowser ? Theme.accentHover : Theme.textSecondary
                }

                Text {
                    text: root.showColumnBrowser ? "▲" : "▼"
                    font.pixelSize: 8
                    color: root.showColumnBrowser ? Theme.accent : Theme.textMuted
                }
            }

            MouseArea {
                id: cbMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.toggleColumnBrowser()
            }

            ToolTip.visible: cbMouse.containsMouse
            ToolTip.text: (root.showColumnBrowser ? "Collapse" : "Expand") + " Category Filters (Genre, Artist, Album) (Ctrl+2)"
        }

        // Toggle Hi-Res Audio Filter Button
        Rectangle {
            Layout.preferredWidth: hiresLayout.implicitWidth + 16
            Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 2
            Layout.rightMargin: 4
            width: Layout.preferredWidth
            height: 24
            radius: Theme.cornerRadiusSmall
            color: bridge.hiResOnly ? Theme.accent : (hiresMouse.containsMouse ? Theme.surfaceElevated : "transparent")
            border.color: bridge.hiResOnly ? Theme.accent : Theme.panelBorder

            RowLayout {
                id: hiresLayout
                anchors.centerIn: parent
                spacing: 4

                VectorIcon {
                    name: "volume"
                    width: 10
                    height: 10
                    color: bridge.hiResOnly ? Theme.textPrimary : Theme.textMuted
                }

                Text {
                    text: "HI-RES"
                    font.pixelSize: 10
                    font.bold: true
                    color: bridge.hiResOnly ? Theme.textPrimary : Theme.textSecondary
                }
            }

            MouseArea {
                id: hiresMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bridge.toggleHiResFilter()
            }

            ToolTip.visible: hiresMouse.containsMouse
            ToolTip.text: bridge.hiResOnly ? "Showing Hi-Res files only (24-bit, 88.2kHz+ or DSD). Click to show all." : "Filter tracks & albums to Hi-Res (24-bit, 88.2kHz+ or DSD)"
        }

        // Alphabet jump ribbon
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            contentWidth: alphaRow.implicitWidth
            clip: true

            RowLayout {
                id: alphaRow
                spacing: 2

                Repeater {
                    model: ["All", "#", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]

                    delegate: Rectangle {
                        Layout.preferredWidth: Math.max(22, alphaText.implicitWidth + 10)
                        Layout.preferredHeight: 20
                        Layout.leftMargin: 1
                        Layout.rightMargin: 1
                        width: Layout.preferredWidth
                        height: 20
                        radius: Theme.cornerRadiusSmall
                        color: root.activeLetterFilter === modelData ? Theme.accent : (alphaMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        Text {
                            id: alphaText
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 10
                            font.bold: root.activeLetterFilter === modelData
                            color: root.activeLetterFilter === modelData ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            id: alphaMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.activeLetterFilter === modelData && modelData !== "All") {
                                    root.activeLetterFilter = "All";
                                    root.letterFilterSelected("All");
                                } else {
                                    root.activeLetterFilter = modelData;
                                    root.letterFilterSelected(modelData);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Item counter
        Text {
            text: {
                var suffix = bridge.hiResOnly ? " (Hi-Res)" : "";
                var vId = bridge.tabModel ? bridge.tabModel.activeViewId : "";
                var baseType = bridge.viewRegistry ? bridge.viewRegistry.getBaseRendererType(vId) : "tracks";
                if (baseType === "covers") {
                    return (bridge.albumModel ? bridge.albumModel.rowCount() : 0) + " albums" + suffix;
                } else if (baseType === "album_and_tracks") {
                    return (bridge.selectedAlbumTrackCount > 0 ? (bridge.selectedAlbumTrackCount + " tracks • " + bridge.selectedAlbumDuration) : "Album Details");
                } else if (baseType === "history") {
                    return (bridge.historyTrackModel ? bridge.historyTrackModel.rowCount() : 0) + " played";
                }
                return (bridge.trackModel ? bridge.trackModel.rowCount() : 0) + " tracks" + suffix;
            }
            font.pixelSize: 10
            color: bridge.hiResOnly ? Theme.accent : Theme.textMuted
        }
    }
}
