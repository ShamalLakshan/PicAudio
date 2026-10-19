import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, Overlay.overlay ? Overlay.overlay.width - 32 : 420)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string oldName: ""

    signal playlistRenamed(string oldName, string newName)

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    contentItem: ColumnLayout {
        spacing: 0
        clip: true

        // Dialog Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                VectorIcon {
                    name: "queue"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "Rename Playlist"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: closeMouse.containsMouse ? Theme.selection : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "clear"
                        width: 10
                        height: 10
                        color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        // Body Form
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.panelPadding
            spacing: Theme.spacingMedium

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    text: "Playlist Name"
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.background
                    border.color: renameInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    clip: true

                    TextInput {
                        id: renameInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBase
                        selectByMouse: true
                        onAccepted: {
                            if (canRename) {
                                executeRename();
                            }
                        }
                    }
                }
            }

            // Footer Actions
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingSmall
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: cancelMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    border.color: Theme.panelBorder
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }

                Rectangle {
                    id: renameBtn
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: canRename
                           ? (renameMouse.containsMouse ? Theme.accentHover : Theme.accent)
                           : Theme.surfaceElevated
                    border.color: canRename ? "transparent" : Theme.panelBorder
                    opacity: canRename ? 1.0 : 0.5
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: "Rename"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: canRename ? Theme.textPrimary : Theme.textMuted
                    }

                    MouseArea {
                        id: renameMouse
                        anchors.fill: parent
                        enabled: root.canRename
                        hoverEnabled: true
                        cursorShape: root.canRename ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.executeRename()
                    }
                }
            }
        }
    }

    property bool canRename: renameInput.text.trim().length > 0 && renameInput.text.trim() !== root.oldName

    function executeRename() {
        var newName = renameInput.text.trim();
        bridge.renamePlaylist(root.oldName, newName);
        root.playlistRenamed(root.oldName, newName);
        root.close();
    }

    onAboutToShow: {
        renameInput.text = oldName;
        renameInput.forceActiveFocus();
    }
}
