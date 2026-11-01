import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 600)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 500)) - height) / 2)
    width: Math.min(520, Overlay.overlay ? Overlay.overlay.width - 32 : 520)
    height: Math.min(480, Overlay.overlay ? Overlay.overlay.height - 32 : 480)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var targetTrack: null
    property var targetTracks: []
    property bool isBatch: targetTracks && targetTracks.length > 1

    signal tagsSaved()

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 600);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 500);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        if (isBatch) {
            titleInput.text = "";
            artistInput.text = "";
            albumInput.text = "";
            genreInput.text = "";
            yearInput.text = "";
            trackNumberInput.text = "";
        } else if (targetTrack) {
            titleInput.text = targetTrack.title || "";
            artistInput.text = targetTrack.artist || "";
            albumInput.text = targetTrack.album || "";
            genreInput.text = targetTrack.genre || "";
            yearInput.text = targetTrack.year > 0 ? ("" + targetTrack.year) : "";
            trackNumberInput.text = targetTrack.trackNumber > 0 ? ("" + targetTrack.trackNumber) : "";
        }
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Dialog Header
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
                    name: "music"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: root.isBatch ? ("Edit Tags (" + root.targetTracks.length + " Tracks)") : "Edit Track Tags"
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

        // Form Fields Container
        ScrollView {
            id: formScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            leftPadding: Theme.panelPadding
            rightPadding: Theme.panelPadding
            topPadding: Theme.spacingMedium
            bottomPadding: Theme.spacingMedium
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: formScrollView.availableWidth
                spacing: Theme.spacingLarge

                Text {
                    visible: root.isBatch
                    text: "Leave fields blank to preserve their existing track values."
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.accentHover
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Title
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall
                    opacity: root.isBatch ? 0.4 : 1.0

                    Text {
                        text: "Track Title"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textSecondary
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: Theme.background
                        border.color: titleInput.activeFocus ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                        clip: true

                        TextInput {
                            id: titleInput
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBase
                            selectByMouse: true
                            enabled: !root.isBatch
                        }
                    }
                }

                // Artist
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Text {
                        text: "Artist"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textSecondary
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: Theme.background
                        border.color: artistInput.activeFocus ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                        clip: true

                        TextInput {
                            id: artistInput
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBase
                            selectByMouse: true
                        }
                    }
                }

                // Album
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Text {
                        text: "Album"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textSecondary
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: Theme.background
                        border.color: albumInput.activeFocus ? Theme.accent : Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                        clip: true

                        TextInput {
                            id: albumInput
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBase
                            selectByMouse: true
                        }
                    }
                }

                // Genre & Year Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMedium

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingSmall

                        Text {
                            text: "Genre"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textSecondary
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: Theme.background
                            border.color: genreInput.activeFocus ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                            clip: true

                            TextInput {
                                id: genreInput
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacingMedium
                                anchors.rightMargin: Theme.spacingMedium
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 90
                        spacing: Theme.spacingSmall

                        Text {
                            text: "Year"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textSecondary
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: Theme.background
                            border.color: yearInput.activeFocus ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                            clip: true

                            TextInput {
                                id: yearInput
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacingMedium
                                anchors.rightMargin: Theme.spacingMedium
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 90
                        spacing: Theme.spacingSmall
                        opacity: root.isBatch ? 0.4 : 1.0

                        Text {
                            text: "Track #"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textSecondary
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: Theme.background
                            border.color: trackNumberInput.activeFocus ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                            clip: true

                            TextInput {
                                id: trackNumberInput
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacingMedium
                                anchors.rightMargin: Theme.spacingMedium
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                                enabled: !root.isBatch
                            }
                        }
                    }
                }
            }
        }

        // Footer Actions Row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: cancelMouse.containsMouse ? Theme.surface : "transparent"
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
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: saveMouse.containsMouse ? Theme.accentHover : Theme.accent
                    clip: true

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSmall
                        VectorIcon {
                            name: "check"
                            width: 11
                            height: 11
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "Save Tags"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textPrimary
                        }
                    }

                    MouseArea {
                        id: saveMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var tags = {};
                            if (!root.isBatch && titleInput.text.trim().length > 0) {
                                tags["title"] = titleInput.text.trim();
                            }
                            if (artistInput.text.trim().length > 0) {
                                tags["artist"] = artistInput.text.trim();
                            }
                            if (albumInput.text.trim().length > 0) {
                                tags["album"] = albumInput.text.trim();
                            }
                            if (genreInput.text.trim().length > 0) {
                                tags["genre"] = genreInput.text.trim();
                            }
                            var y = parseInt(yearInput.text.trim());
                            if (!isNaN(y) && y > 0) {
                                tags["year"] = y;
                            }
                            var trk = parseInt(trackNumberInput.text.trim());
                            if (!isNaN(trk) && trk > 0) {
                                tags["trackNumber"] = trk;
                            }

                            if (root.isBatch) {
                                var ids = [];
                                for (var i = 0; i < root.targetTracks.length; ++i) {
                                    if (root.targetTracks[i].id) {
                                        ids.push(root.targetTracks[i].id);
                                    }
                                }
                                bridge.updateMultipleTrackTags(ids, tags);
                            } else if (root.targetTrack && root.targetTrack.id) {
                                bridge.updateTrackTags(root.targetTrack.id, tags);
                            }

                            root.tagsSaved();
                            root.close();
                        }
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 460
        minHeight: 380
    }
}
}
