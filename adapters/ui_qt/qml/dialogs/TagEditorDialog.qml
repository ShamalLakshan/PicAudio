import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 680)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 600)) - height) / 2)
    width: Math.min(720, Overlay.overlay ? Math.max(560, Overlay.overlay.width - 48) : 720)
    height: Math.min(620, Overlay.overlay ? Math.max(480, Overlay.overlay.height - 48) : 620)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var targetTrack: null
    property var targetTracks: []
    property bool isBatch: targetTracks && targetTracks.length > 1
    property var sharedData: ({})
    property string artPreviewHash: ""

    // Dirty flags to only save fields edited by user
    property bool titleModified: false
    property bool artistModified: false
    property bool albumModified: false
    property bool albumArtistModified: false
    property bool genreModified: false
    property bool yearModified: false
    property bool discNumberModified: false
    property bool trackNumberModified: false
    property bool commentModified: false

    signal tagsSaved()

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    function getTargetIds() {
        var ids = [];
        if (root.isBatch) {
            for (var i = 0; i < root.targetTracks.length; ++i) {
                if (root.targetTracks[i].id) ids.push(root.targetTracks[i].id);
            }
        } else if (root.targetTrack && root.targetTrack.id) {
            ids.push(root.targetTrack.id);
        }
        return ids;
    }

    function resetDirtyFlags() {
        titleModified = false;
        artistModified = false;
        albumModified = false;
        albumArtistModified = false;
        genreModified = false;
        yearModified = false;
        discNumberModified = false;
        trackNumberModified = false;
        commentModified = false;
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 680);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 600);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        resetDirtyFlags();
        var ids = getTargetIds();
        sharedData = bridge.getSharedTrackTags(ids);
        artPreviewHash = sharedData.artHash || "";

        if (root.isBatch) {
            titleInput.text = "";
            titleInput.placeholderText = "<keep individual track titles>";

            artistInput.text = sharedData.sameArtist ? (sharedData.artist || "") : "";
            artistInput.placeholderText = sharedData.sameArtist ? "" : "<multiple values>";

            albumInput.text = sharedData.sameAlbum ? (sharedData.album || "") : "";
            albumInput.placeholderText = sharedData.sameAlbum ? "" : "<multiple values>";

            albumArtistInput.text = sharedData.sameAlbumArtist ? (sharedData.albumArtist || "") : "";
            albumArtistInput.placeholderText = sharedData.sameAlbumArtist ? "" : "<multiple values>";

            genreInput.text = sharedData.sameGenre ? (sharedData.genre || "") : "";
            genreInput.placeholderText = sharedData.sameGenre ? "" : "<multiple values>";

            yearInput.text = sharedData.sameYear ? (sharedData.year || "") : "";
            yearInput.placeholderText = sharedData.sameYear ? "" : "<multiple values>";

            discNumberInput.text = "";
            discNumberInput.placeholderText = "<keep>";

            trackNumberInput.text = "";
            trackNumberInput.placeholderText = "<keep>";

            commentInput.text = sharedData.sameComment ? (sharedData.comment || "") : "";
            commentInput.placeholderText = sharedData.sameComment ? "" : "<multiple values>";
        } else if (root.targetTrack) {
            titleInput.text = sharedData.title || root.targetTrack.title || "";
            titleInput.placeholderText = "";

            artistInput.text = sharedData.artist || root.targetTrack.artist || "";
            artistInput.placeholderText = "";

            albumInput.text = sharedData.album || root.targetTrack.album || "";
            albumInput.placeholderText = "";

            albumArtistInput.text = sharedData.albumArtist || root.targetTrack.albumArtist || "";
            albumArtistInput.placeholderText = "";

            genreInput.text = sharedData.genre || root.targetTrack.genre || "";
            genreInput.placeholderText = "";

            yearInput.text = sharedData.year || (root.targetTrack.year > 0 ? ("" + root.targetTrack.year) : "");
            yearInput.placeholderText = "";

            discNumberInput.text = sharedData.discNumber || (root.targetTrack.discNumber > 0 ? ("" + root.targetTrack.discNumber) : "");
            discNumberInput.placeholderText = "";

            trackNumberInput.text = sharedData.trackNumber || (root.targetTrack.trackNumber > 0 ? ("" + root.targetTrack.trackNumber) : "");
            trackNumberInput.placeholderText = "";

            commentInput.text = sharedData.comment || root.targetTrack.comment || "";
            commentInput.placeholderText = "";
        }
    }

    FileDialog {
        id: replaceArtDialog
        title: "Select Artwork Image"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp)", "All files (*)"]
        onAccepted: {
            var ids = root.getTargetIds();
            if (ids.length > 0) {
                var path = selectedFile.toString();
                bridge.replaceCoverArtFromFile(ids, path);
                root.sharedData = bridge.getSharedTrackTags(ids);
                root.artPreviewHash = root.sharedData.artHash || "";
            }
        }
    }

    FileDialog {
        id: exportArtDialog
        title: "Export Cover Artwork"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JPEG Image (*.jpg)", "PNG Image (*.png)", "All files (*)"]
        onAccepted: {
            var ids = root.getTargetIds();
            if (ids.length > 0) {
                bridge.extractCoverArtToFile(ids[0], selectedFile.toString());
            }
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
                        text: root.isBatch ? ("Edit Tags (" + root.targetTracks.length + " Tracks Selected)") : "Edit Track Tags & Artwork"
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
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: formScrollView.availableWidth
                    spacing: 12

                    // Cover Art Card Row
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: 90
                                Layout.preferredHeight: 90
                                color: Theme.background
                                border.color: Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: root.artPreviewHash ? ("image://art/" + root.artPreviewHash) : "qrc:/icons/picaudio.png"
                                    fillMode: Image.PreserveAspectCrop
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    text: "Cover Artwork"
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: true
                                    color: Theme.textPrimary
                                }

                                RowLayout {
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: repArtText.implicitWidth + 20
                                        Layout.preferredHeight: 28
                                        radius: Theme.cornerRadiusSmall
                                        color: repArtMouse.containsMouse ? Theme.selection : Theme.surface
                                        border.color: Theme.panelBorder
                                        Text { id: repArtText; anchors.centerIn: parent; text: "Replace Art..."; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textPrimary }
                                        MouseArea {
                                            id: repArtMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: replaceArtDialog.open()
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: expArtText.implicitWidth + 20
                                        Layout.preferredHeight: 28
                                        radius: Theme.cornerRadiusSmall
                                        color: expArtMouse.containsMouse ? Theme.selection : Theme.surface
                                        border.color: Theme.panelBorder
                                        Text { id: expArtText; anchors.centerIn: parent; text: "Export Art..."; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textSecondary }
                                        MouseArea {
                                            id: expArtMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: exportArtDialog.open()
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: remArtText.implicitWidth + 20
                                        Layout.preferredHeight: 28
                                        radius: Theme.cornerRadiusSmall
                                        color: remArtMouse.containsMouse ? Theme.selection : Theme.surface
                                        border.color: Theme.panelBorder
                                        Text { id: remArtText; anchors.centerIn: parent; text: "Remove Art"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.accent }
                                        MouseArea {
                                            id: remArtMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                var ids = root.getTargetIds();
                                                bridge.removeCoverArtFromTracks(ids);
                                                root.artPreviewHash = "";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        visible: root.isBatch
                        text: "Fields displaying <multiple values> vary across selected tracks. Only edited fields will be updated."
                        font.pixelSize: Theme.fontSizeSmall - 1
                        color: Theme.accentHover
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    // Title
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        opacity: root.isBatch ? 0.45 : 1.0

                        Text { text: "Track Title"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                anchors.margins: 6
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                                enabled: !root.isBatch
                                onTextChanged: if (activeFocus) root.titleModified = true
                            }
                        }
                    }

                    // Artist & Album Artist Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text { text: "Track Artist"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.artistModified = true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text { text: "Album Artist"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                color: Theme.background
                                border.color: albumArtistInput.activeFocus ? Theme.accent : Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                                clip: true

                                TextInput {
                                    id: albumArtistInput
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.albumArtistModified = true
                                }
                            }
                        }
                    }

                    // Album & Genre Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text { text: "Album"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.albumModified = true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 160
                            spacing: 3
                            Text { text: "Genre"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.genreModified = true
                                }
                            }
                        }
                    }

                    // Year, Track #, Disc #
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.preferredWidth: 90
                            spacing: 3
                            Text { text: "Year"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.yearModified = true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 90
                            spacing: 3
                            opacity: root.isBatch ? 0.45 : 1.0
                            Text { text: "Track #"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
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
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    enabled: !root.isBatch
                                    onTextChanged: if (activeFocus) root.trackNumberModified = true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 90
                            spacing: 3
                            Text { text: "Disc #"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                color: Theme.background
                                border.color: discNumberInput.activeFocus ? Theme.accent : Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                                clip: true

                                TextInput {
                                    id: discNumberInput
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeBase
                                    selectByMouse: true
                                    onTextChanged: if (activeFocus) root.discNumberModified = true
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Comment
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: "Comment / Description"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: Theme.background
                            border.color: commentInput.activeFocus ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                            clip: true

                            TextInput {
                                id: commentInput
                                anchors.fill: parent
                                anchors.margins: 6
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                                onTextChanged: if (activeFocus) root.commentModified = true
                            }
                        }
                    }
                }
            }

            // Footer Actions Row
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: 36 // Safe margin for ResizeGrip
                    spacing: Theme.spacingMedium

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: cancelMouse.containsMouse ? Theme.surface : "transparent"
                        border.color: Theme.panelBorder

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
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: saveMouse.containsMouse ? Theme.accentHover : Theme.accent

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
                                if (!root.isBatch && root.titleModified) {
                                    tags["title"] = titleInput.text.trim();
                                }
                                if (root.artistModified) {
                                    tags["artist"] = artistInput.text.trim();
                                }
                                if (root.albumModified) {
                                    tags["album"] = albumInput.text.trim();
                                }
                                if (root.albumArtistModified) {
                                    tags["albumArtist"] = albumArtistInput.text.trim();
                                }
                                if (root.genreModified) {
                                    tags["genre"] = genreInput.text.trim();
                                }
                                if (root.yearModified) {
                                    var y = parseInt(yearInput.text.trim());
                                    tags["year"] = isNaN(y) ? 0 : y;
                                }
                                if (root.discNumberModified) {
                                    var d = parseInt(discNumberInput.text.trim());
                                    tags["discNumber"] = isNaN(d) ? 0 : d;
                                }
                                if (!root.isBatch && root.trackNumberModified) {
                                    var trk = parseInt(trackNumberInput.text.trim());
                                    tags["trackNumber"] = isNaN(trk) ? 0 : trk;
                                }
                                if (root.commentModified) {
                                    tags["comment"] = commentInput.text.trim();
                                }

                                var ids = root.getTargetIds();
                                if (ids.length > 1) {
                                    bridge.updateMultipleTrackTags(ids, tags);
                                } else if (ids.length === 1) {
                                    bridge.updateTrackTags(ids[0], tags);
                                }

                                root.tagsSaved();
                                root.close();
                            }
                        }
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 520
        minHeight: 420
    }
}
