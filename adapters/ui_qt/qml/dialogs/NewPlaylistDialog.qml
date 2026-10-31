import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400)) - height) / 2)
    width: Math.min(460, Overlay.overlay ? Overlay.overlay.width - 32 : 460)
    height: Math.min(isSmart ? 340 : 260, Overlay.overlay ? Overlay.overlay.height - 32 : (isSmart ? 340 : 260))
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool isSmart: false
    property bool isEdit: false

    function openSmart() {
        isEdit = false;
        isSmart = true;
        open();
    }

    function openSmartEdit(name, rules) {
        isEdit = true;
        isSmart = true;
        open();
        playlistNameInput.text = name;
        smartCriteriaInput.text = rules;
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        if (!isEdit) {
            playlistNameInput.text = "";
            smartCriteriaInput.text = "";
        }
        playlistNameInput.forceActiveFocus();
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

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                MovableHeaderArea {
                    target: root
                }

                RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                VectorIcon {
                    name: root.isSmart ? "settings" : "queue"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: root.isSmart ? "Create Smart Playlist" : "Create New Playlist"
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
            Layout.fillHeight: true
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
                    border.color: playlistNameInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    clip: true

                    TextInput {
                        id: playlistNameInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBase
                        selectByMouse: true

                        Text {
                            text: root.isSmart ? "e.g. 5-Star Favorites, Hi-Res FLAC" : "e.g. Road Trip, Late Night Chill"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeBase
                            visible: !playlistNameInput.text && !playlistNameInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Keys.onReturnPressed: {
                            if (playlistNameInput.text.trim().length > 0) {
                                submit();
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                visible: root.isSmart

                Text {
                    text: "Dynamic Filter Rule"
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.background
                    border.color: smartCriteriaInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    clip: true

                    TextInput {
                        id: smartCriteriaInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBase
                        selectByMouse: true

                        Text {
                            text: "e.g. genre:Jazz, rating:>=4, year:>=2000"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeBase
                            visible: !smartCriteriaInput.text && !smartCriteriaInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Keys.onReturnPressed: {
                            if (playlistNameInput.text.trim().length > 0) {
                                submit();
                            }
                        }
                    }
                }

                Text {
                    text: "Tracks matching this rule will automatically populate into this smart playlist."
                    font.pixelSize: 10
                    color: Theme.textMuted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Item { Layout.fillHeight: true }
        }

        // Footer Actions
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    implicitHeight: 28
                    implicitWidth: 80
                    onClicked: root.close()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }

                Button {
                    text: root.isSmart ? "Create Smart Playlist" : "Create Playlist"
                    enabled: playlistNameInput.text.trim().length > 0
                    implicitHeight: 28
                    onClicked: submit()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: parent.enabled ? Theme.accentHover : Theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? Theme.selection : Theme.background) : Theme.background
                        border.color: parent.enabled ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 380
        minHeight: 240
    }
}

    function submit() {
        var name = playlistNameInput.text.trim();
        if (!name) return;
        if (root.isSmart) {
            if (root.isEdit) {
                bridge.updateSmartPlaylistRules(name, smartCriteriaInput.text.trim());
            } else {
                bridge.createSmartPlaylist(name, smartCriteriaInput.text.trim());
            }
        } else {
            bridge.createPlaylist(name);
        }
        isEdit = false;
        root.close();
    }
}
