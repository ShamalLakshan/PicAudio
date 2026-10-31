import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root

    DropArea {
        id: queueDropArea
        anchors.fill: parent
        onEntered: (drag) => {
            if (drag.hasUrls) {
                drag.acceptProposedAction();
            }
        }
        onDropped: (drop) => {
            if (drop.hasUrls) {
                bridge.handleDropOnQueue(drop.urls);
                drop.acceptProposedAction();
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
        border.color: Theme.accent
        border.width: 2
        radius: Theme.cornerRadiusMedium
        visible: queueDropArea.containsDrag
        z: 999
        clip: true

        RowLayout {
            anchors.centerIn: parent
            spacing: Theme.spacingMedium

            VectorIcon {
                name: "playlist"
                width: 24
                height: 24
                color: Theme.accent
            }

            Text {
                text: "Drop tracks or folders to enqueue"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeBase
                font.family: Theme.fontFamily
                font.bold: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
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
                spacing: Theme.spacingMedium

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: "UPCOMING QUEUE"
                        font.pixelSize: Theme.fontSizeSmall - 1
                        font.bold: true
                        font.letterSpacing: 1.0
                        color: Theme.textMuted
                    }
                    Text {
                        text: bridge.queueRemainingCount + " tracks • " + bridge.queueRemainingDurationStr
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.accent
                    }
                }

                Item { Layout.fillWidth: true }

                // Shuffle button
                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: shufBtnMouse.containsMouse ? Theme.selection : "transparent"
                    border.color: Theme.panelBorder
                    VectorIcon {
                        anchors.centerIn: parent
                        name: "shuffle"
                        width: 11
                        height: 11
                        color: Theme.textSecondary
                    }
                    MouseArea {
                        id: shufBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.shuffleRemainingQueue()
                    }
                    ToolTip.visible: shufBtnMouse.containsMouse
                    ToolTip.text: "Shuffle Remaining Tracks"
                }

                // Clear button
                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: clearBtnMouse.containsMouse ? Theme.selection : "transparent"
                    border.color: Theme.panelBorder
                    VectorIcon {
                        anchors.centerIn: parent
                        name: "trash"
                        width: 11
                        height: 11
                        color: Theme.textSecondary
                    }
                    MouseArea {
                        id: clearBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.clearQueue()
                    }
                    ToolTip.visible: clearBtnMouse.containsMouse
                    ToolTip.text: "Clear Upcoming Queue"
                }
            }
        }

        // Empty state
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            visible: bridge.queueRemainingCount === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium

                VectorIcon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "queue"
                    width: 32
                    height: 32
                    color: Theme.textMuted
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Queue is empty"
                    font.pixelSize: Theme.fontSizeBase
                    font.bold: true
                    color: Theme.textMuted
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Play a track or right-click to add songs"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textMuted
                }
            }
        }

        // Queue list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: bridge.queueRemainingCount > 0
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: queueListView
                anchors.fill: parent
                model: bridge.queueTrackModel
                clip: true

                delegate: Rectangle {
                    id: queueItemDelegate
                    width: queueListView.width
                    height: 36
                    color: qRowMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    border.color: qRowMouse.containsMouse ? Theme.panelBorder : "transparent"
                    border.width: 1

                    property bool isUpNextItem: index < bridge.upNextCount

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 6

                        // Track number
                        Rectangle {
                            Layout.preferredWidth: queueItemDelegate.isUpNextItem ? 20 : 16
                            Layout.preferredHeight: 16
                            radius: 2
                            color: queueItemDelegate.isUpNextItem ? Theme.selection : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: queueItemDelegate.isUpNextItem ? "★" : (index + 1)
                                font.pixelSize: Theme.fontSizeSmall - 1
                                font.bold: queueItemDelegate.isUpNextItem
                                color: queueItemDelegate.isUpNextItem ? Theme.accent : Theme.textMuted
                            }
                        }

                        // Track info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: model.title
                                font.pixelSize: Theme.fontSizeSmall + 1
                                font.bold: true
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                spacing: 4
                                Layout.fillWidth: true
                                clip: true

                                Text {
                                    text: model.artist
                                    font.pixelSize: Theme.fontSizeSmall - 1
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: model.codec
                                    font.pixelSize: Theme.fontSizeSmall - 2
                                    font.weight: Font.Medium
                                    color: Theme.textMuted
                                }
                            }
                        }

                        // Duration
                        Text {
                            text: model.durationFormatted
                            font.pixelSize: Theme.fontSizeSmall - 1
                            color: Theme.textMuted
                        }

                        // Reorder buttons (Move Up / Down)
                        RowLayout {
                            spacing: 1
                            visible: qRowMouse.containsMouse
                            z: 2

                            Rectangle {
                                width: 16
                                height: 16
                                radius: 2
                                color: upMouse.containsMouse ? Theme.accent : "transparent"
                                visible: index > 0
                                VectorIcon { anchors.centerIn: parent; name: "arrow_up"; width: 8; height: 8; color: Theme.textPrimary }
                                MouseArea {
                                    id: upMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: bridge.moveQueueItem(index, index - 1)
                                }
                            }

                            Rectangle {
                                width: 16
                                height: 16
                                radius: 2
                                color: dnMouse.containsMouse ? Theme.accent : "transparent"
                                visible: index < bridge.queueRemainingCount - 1
                                VectorIcon { anchors.centerIn: parent; name: "arrow_down"; width: 8; height: 8; color: Theme.textPrimary }
                                MouseArea {
                                    id: dnMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: bridge.moveQueueItem(index, index + 1)
                                }
                            }

                            Rectangle {
                                width: 16
                                height: 16
                                radius: 2
                                color: rmMouse.containsMouse ? Theme.error : "transparent"
                                VectorIcon { anchors.centerIn: parent; name: "close"; width: 8; height: 8; color: Theme.textPrimary }
                                MouseArea {
                                    id: rmMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: bridge.removeFromQueue(index)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: qRowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            queueListView.currentIndex = index;
                            if (mouse.button === Qt.RightButton) {
                                queueContextMenu.targetQueueIndex = index;
                                queueContextMenu.popup();
                            }
                        }
                        onDoubleClicked: (mouse) => {
                            if (mouse.button === Qt.LeftButton) {
                                bridge.playQueueTrack(index);
                            }
                        }
                    }
                }
            }
        }
    }

    // Keyboard Shortcuts for Queue Navigation and Management (Issue #12)
    Shortcut {
        sequence: "Alt+Up"
        enabled: root.visible && queueListView.currentIndex > 0
        onActivated: {
            var idx = queueListView.currentIndex;
            bridge.moveQueueItem(idx, idx - 1);
            queueListView.currentIndex = idx - 1;
        }
    }

    Shortcut {
        sequence: "Alt+Down"
        enabled: root.visible && queueListView.currentIndex >= 0 && queueListView.currentIndex < bridge.queueRemainingCount - 1
        onActivated: {
            var idx = queueListView.currentIndex;
            bridge.moveQueueItem(idx, idx + 1);
            queueListView.currentIndex = idx + 1;
        }
    }

    Shortcut {
        sequence: "Delete"
        enabled: root.visible && queueListView.currentIndex >= 0
        onActivated: {
            var idx = queueListView.currentIndex;
            bridge.removeFromQueue(idx);
            if (queueListView.currentIndex >= bridge.queueRemainingCount) {
                queueListView.currentIndex = bridge.queueRemainingCount - 1;
            }
        }
    }

    // Queue Context Menu (Issue #12)
    ContextMenu {
        id: queueContextMenu
        menuType: "queue"
        onSaveQueueAsPlaylistRequested: saveQueueDialog.open()
    }

    // Dialog for Saving Current Queue as a Static Playlist
    Dialog {
        id: saveQueueDialog
        modal: true
        focus: true
        x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500)) - width) / 2)
        y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400)) - height) / 2)
        width: 380
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        header: Rectangle {
            height: 38
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            MovableHeaderArea {
                target: saveQueueDialog
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                VectorIcon {
                    name: "queue"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "Save Queue as Playlist"
                    font.pixelSize: Theme.fontSizeBase
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }
            }
        }

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusMedium

            ResizeGrip {
                target: saveQueueDialog
                minWidth: 340
                minHeight: 200
            }
        }

        contentItem: ColumnLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 16

            Text {
                text: "Create a new static playlist containing all remaining tracks in the queue."
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            TextField {
                id: queuePlaylistNameInput
                Layout.fillWidth: true
                placeholderText: "Enter playlist name"
                color: Theme.textPrimary
                background: Rectangle {
                    color: Theme.surfaceElevated
                    border.color: queuePlaylistNameInput.activeFocus ? Theme.accent : Theme.panelBorder
                    radius: Theme.cornerRadiusSmall
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                Button {
                    text: "Cancel"
                    onClicked: saveQueueDialog.close()
                }

                Button {
                    text: "Save Playlist"
                    enabled: queuePlaylistNameInput.text.trim().length > 0
                    onClicked: {
                        bridge.saveQueueAsPlaylist(queuePlaylistNameInput.text.trim());
                        saveQueueDialog.close();
                    }
                }
            }
        }

        onAboutToShow: {
            var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
            var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400);
            if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
                x = Math.max(16, Math.round((ovW - width) / 2));
                y = Math.max(16, Math.round((ovH - height) / 2));
            }

            queuePlaylistNameInput.text = "";
            queuePlaylistNameInput.forceActiveFocus();
        }
    }
}
