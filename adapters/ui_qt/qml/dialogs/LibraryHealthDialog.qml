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
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 820)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 620)) - height) / 2)
    width: Math.min(840, Overlay.overlay ? Math.max(680, Overlay.overlay.width - 48) : 840)
    height: Math.min(640, Overlay.overlay ? Math.max(520, Overlay.overlay.height - 48) : 640)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var health: bridge.checkLibraryHealth()
    property var missingTracks: []
    property int currentTab: 0 // 0: Missing Tracks, 1: Batch Relocation

    function refreshHealth() {
        health = bridge.checkLibraryHealth();
        missingTracks = bridge.scanMissingTracks();
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 820);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 620);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
        refreshHealth();
    }

    FolderDialog {
        id: oldFolderPicker
        title: "Select Old Folder Prefix"
        onAccepted: {
            oldPrefixInput.text = selectedFolder.toString().replace(/^file:\/\//, "");
        }
    }

    FolderDialog {
        id: newFolderPicker
        title: "Select New Folder Destination"
        onAccepted: {
            newPrefixInput.text = selectedFolder.toString().replace(/^file:\/\//, "");
        }
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
                Layout.preferredHeight: 46
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
                        name: "settings"
                        width: 14
                        height: 14
                        color: Theme.accent
                    }

                    Text {
                        text: "Library Health & Missing Files Inspector"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        width: 26
                        height: 26
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

            // Stats Cards Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 74
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    // Card 1: Total Tracks
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        radius: Theme.cornerRadiusSmall
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: "" + (root.health.totalTracks || 0); font.pixelSize: Theme.fontSizeLarge; font.bold: true; color: Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "TOTAL TRACKS"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }

                    // Card 2: Missing Files
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: (root.health.missingTracksCount > 0) ? Theme.surfaceElevated : Theme.surface
                        border.color: (root.health.missingTracksCount > 0) ? Theme.accent : Theme.panelBorder
                        border.width: (root.health.missingTracksCount > 0) ? 1.5 : 1
                        radius: Theme.cornerRadiusSmall
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: "" + (root.health.missingTracksCount || 0); font.pixelSize: Theme.fontSizeLarge; font.bold: true; color: (root.health.missingTracksCount > 0) ? Theme.accent : Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "MISSING FILES"; font.pixelSize: 9; font.bold: true; color: (root.health.missingTracksCount > 0) ? Theme.accent : Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }

                    // Card 3: Duplicates
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        radius: Theme.cornerRadiusSmall
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: "" + (root.health.duplicateTracksCount || 0); font.pixelSize: Theme.fontSizeLarge; font.bold: true; color: Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "DUPLICATES"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }

                    // Card 4: Untagged
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        radius: Theme.cornerRadiusSmall
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: "" + (root.health.untaggedTracksCount || 0); font.pixelSize: Theme.fontSizeLarge; font.bold: true; color: Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "UNTAGGED"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }

                    // Card 5: DB Size
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        radius: Theme.cornerRadiusSmall
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: root.health.databaseSizeFormatted || "0 KB"; font.pixelSize: Theme.fontSizeBase; font.bold: true; color: Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "SQLITE DATABASE"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
            }

            // Tab bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: Theme.surface
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.preferredWidth: 180
                        Layout.fillHeight: true
                        color: root.currentTab === 0 ? Theme.selection : "transparent"
                        border.color: root.currentTab === 0 ? Theme.accent : "transparent"
                        border.width: root.currentTab === 0 ? 1 : 0
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "Missing Tracks (" + root.missingTracks.length + ")"; font.pixelSize: Theme.fontSizeSmall; font.bold: root.currentTab === 0; color: root.currentTab === 0 ? Theme.textPrimary : Theme.textMuted }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentTab = 0
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 180
                        Layout.fillHeight: true
                        color: root.currentTab === 1 ? Theme.selection : "transparent"
                        border.color: root.currentTab === 1 ? Theme.accent : "transparent"
                        border.width: root.currentTab === 1 ? 1 : 0
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "Batch Relocate Folder"; font.pixelSize: Theme.fontSizeSmall; font.bold: root.currentTab === 1; color: root.currentTab === 1 ? Theme.textPrimary : Theme.textMuted }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentTab = 1
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // Tab 0: Missing Tracks Table
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.currentTab === 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Table Header
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            Text { Layout.preferredWidth: 160; text: "TITLE"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 120; text: "ARTIST"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 120; text: "ALBUM"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; text: "BROKEN PATH"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted; elide: Text.ElideRight }
                        }
                    }

                    ScrollView {
                        id: missingScrollView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        clip: true

                        ListView {
                            id: missingListView
                            width: missingScrollView.availableWidth
                            model: root.missingTracks
                            clip: true

                            delegate: Rectangle {
                                width: missingListView.width
                                height: 30
                                color: rowMouse.containsMouse ? Theme.surfaceElevated : (index % 2 === 0 ? Theme.surface : Theme.background)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8
                                    Text { Layout.preferredWidth: 160; text: modelData.title || "—"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textPrimary; elide: Text.ElideRight }
                                    Text { Layout.preferredWidth: 120; text: modelData.artist || "—"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary; elide: Text.ElideRight }
                                    Text { Layout.preferredWidth: 120; text: modelData.album || "—"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textMuted; elide: Text.ElideRight }
                                    Text { Layout.fillWidth: true; text: modelData.filePath || ""; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.accentHover; elide: Text.ElideMiddle }
                                }

                                MouseArea {
                                    id: rowMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }
                        }
                    }

                    // Empty state
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: root.missingTracks.length === 0
                        color: Theme.background
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            VectorIcon { name: "music"; width: 24; height: 24; color: Theme.accent; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "No broken file paths found"; font.pixelSize: Theme.fontSizeBase; font.bold: true; color: Theme.textPrimary; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "All tracks in your library exist at their recorded locations on disk."; font.pixelSize: Theme.fontSizeSmall; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
            }

            // Tab 1: Batch Relocate Folder
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.currentTab === 1

                ScrollView {
                    id: relocateScrollView
                    anchors.fill: parent
                    anchors.margins: Theme.panelPadding
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: relocateScrollView.availableWidth
                        spacing: 16

                        Text {
                            text: "Batch Relocate Missing Drive or Folder"
                            font.pixelSize: Theme.fontSizeBase
                            font.bold: true
                            color: Theme.textPrimary
                        }

                        Text {
                            text: "If you moved your music directory or changed external drive letters, enter the previous path prefix and the new destination prefix below to instantly update your library without re-scanning or losing play counts, history, or ratings."
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // Old Prefix
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text { text: "Old Folder Path Prefix (to replace):"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    color: Theme.background
                                    border.color: oldPrefixInput.activeFocus ? Theme.accent : Theme.panelBorder
                                    border.width: 1
                                    radius: Theme.cornerRadiusSmall
                                    clip: true
                                    TextInput {
                                        id: oldPrefixInput
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeBase
                                        selectByMouse: true
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 84
                                    Layout.preferredHeight: 32
                                    radius: Theme.cornerRadiusSmall
                                    color: oldBrowseMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                    border.color: Theme.panelBorder
                                    Text { anchors.centerIn: parent; text: "Browse..."; font.pixelSize: Theme.fontSizeSmall; color: Theme.textPrimary }
                                    MouseArea {
                                        id: oldBrowseMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: oldFolderPicker.open()
                                    }
                                }
                            }
                        }

                        // New Prefix
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text { text: "New Folder Path Destination:"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    color: Theme.background
                                    border.color: newPrefixInput.activeFocus ? Theme.accent : Theme.panelBorder
                                    border.width: 1
                                    radius: Theme.cornerRadiusSmall
                                    clip: true
                                    TextInput {
                                        id: newPrefixInput
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        verticalAlignment: TextInput.AlignVCenter
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeBase
                                        selectByMouse: true
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 84
                                    Layout.preferredHeight: 32
                                    radius: Theme.cornerRadiusSmall
                                    color: newBrowseMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                    border.color: Theme.panelBorder
                                    Text { anchors.centerIn: parent; text: "Browse..."; font.pixelSize: Theme.fontSizeSmall; color: Theme.textPrimary }
                                    MouseArea {
                                        id: newBrowseMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: newFolderPicker.open()
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 34
                            radius: Theme.cornerRadiusSmall
                            color: relocateMouse.containsMouse ? Theme.accentHover : Theme.accent
                            Text { anchors.centerIn: parent; text: "Relocate Files Now"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary }
                            MouseArea {
                                id: relocateMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (oldPrefixInput.text.trim().length > 0 && newPrefixInput.text.trim().length > 0) {
                                        var count = bridge.relocateMissingFolder(oldPrefixInput.text.trim(), newPrefixInput.text.trim());
                                        root.refreshHealth();
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // Bottom Actions Bar
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
                    spacing: 8

                    Rectangle {
                        Layout.preferredWidth: purgeText.implicitWidth + 24
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: purgeMouse.containsMouse ? Theme.selection : "transparent"
                        border.color: Theme.panelBorder
                        Text { id: purgeText; anchors.centerIn: parent; text: "Purge Dead Tracks"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                        MouseArea {
                            id: purgeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                bridge.purgeMissingTracks();
                                root.refreshHealth();
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: dedupText.implicitWidth + 24
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: dedupMouse.containsMouse ? Theme.selection : "transparent"
                        border.color: Theme.panelBorder
                        Text { id: dedupText; anchors.centerIn: parent; text: "Deduplicate Library"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                        MouseArea {
                            id: dedupMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                bridge.deduplicateTracks();
                                root.refreshHealth();
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: optText.implicitWidth + 24
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: optMouse.containsMouse ? Theme.selection : "transparent"
                        border.color: Theme.panelBorder
                        Text { id: optText; anchors.centerIn: parent; text: "Optimize SQLite DB"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                        MouseArea {
                            id: optMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                bridge.optimizeDatabase();
                                root.refreshHealth();
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: rescanText.implicitWidth + 24
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: rescanMouse.containsMouse ? Theme.selection : Theme.surface
                        border.color: Theme.panelBorder
                        Text { id: rescanText; anchors.centerIn: parent; text: "Refresh Scan"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textPrimary }
                        MouseArea {
                            id: rescanMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.refreshHealth()
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: doneMouse.containsMouse ? Theme.accentHover : Theme.accent
                        Text { anchors.centerIn: parent; text: "Done"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary }
                        MouseArea {
                            id: doneMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
            }
        }
    }
}
