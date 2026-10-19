import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root

    property bool isPlaylistView: false
    property string activePlaylistName: ""
    property alias currentIndex: trackTableListView.currentIndex

    signal requestColumnHeaderContextMenu(string colKey, string colTitle)
    signal requestViewportContextMenu()
    signal requestTrackContextMenu(var targetTracks, var targetTrack, string targetFilePath, string targetTitle, string targetArtist, bool isPlaylistView, string currentPlaylistName)
    signal requestExpandAlbum(string album, string artist)

    DropArea {
        id: tableDropArea
        anchors.fill: parent
        onEntered: (drag) => {
            if (drag.hasUrls) {
                drag.acceptProposedAction();
            }
        }
        onDropped: (drop) => {
            if (drop.hasUrls) {
                bridge.handleDropOnTracks(drop.urls, true);
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
        visible: tableDropArea.containsDrag
        z: 999
        clip: true

        RowLayout {
            anchors.centerIn: parent
            spacing: Theme.spacingMedium

            VectorIcon {
                name: "play"
                width: 24
                height: 24
                color: Theme.accent
            }

            Text {
                text: "Drop audio files or folders to play immediately"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeMedium
                font.family: Theme.fontFamily
                font.bold: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        component TableHeaderCell: Item {
            id: cellRoot
            property string colKey: ""
            property string colTitle: ""
            property string headerText: ""
            property int colIndex: 0
            property bool isFillWidth: false

            visible: bridge.isColumnVisible(colKey)
            Layout.preferredWidth: bridge.getColumnWidth(colKey)
            Layout.minimumWidth: isFillWidth ? 100 : 30
            Layout.fillWidth: isFillWidth
            Layout.fillHeight: true
            clip: true

            Rectangle {
                anchors.fill: parent
                color: hCellMouse.containsMouse ? Theme.surfaceHighlight : "transparent"
                radius: 2
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 8
                spacing: 4

                Text {
                    text: cellRoot.headerText
                    font.pixelSize: 9
                    font.bold: true
                    color: (bridge.trackModel.sortColumn === cellRoot.colKey) ? Theme.accent : Theme.textMuted
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillWidth: true
                }

                Text {
                    visible: bridge.trackModel.sortColumn === cellRoot.colKey
                    text: bridge.trackModel.sortAscending ? "▲" : "▼"
                    font.pixelSize: 8
                    color: Theme.accent
                    verticalAlignment: Text.AlignVCenter
                }
            }

            MouseArea {
                id: hCellMouse
                anchors.fill: parent
                anchors.rightMargin: 6
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property int dragStartX: 0
                property bool isDragging: false

                onPressed: (mouse) => {
                    dragStartX = mouse.x;
                    isDragging = false;
                }

                onPositionChanged: (mouse) => {
                    if (pressed && (mouse.buttons & Qt.LeftButton)) {
                        var dx = mouse.x - dragStartX;
                        if (Math.abs(dx) > 10) {
                            isDragging = true;
                        }
                        var cellW = Math.max(30, cellRoot.width);
                        if (dx > cellW && cellRoot.colIndex < bridge.tableColumnKeys.length - 1) {
                            bridge.moveColumn(cellRoot.colIndex, cellRoot.colIndex + 1);
                            dragStartX = mouse.x;
                        } else if (dx < -cellW && cellRoot.colIndex > 0) {
                            bridge.moveColumn(cellRoot.colIndex, cellRoot.colIndex - 1);
                            dragStartX = mouse.x;
                        }
                    }
                }

                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        root.requestColumnHeaderContextMenu(cellRoot.colKey, cellRoot.colTitle);
                    } else if (!isDragging) {
                        bridge.trackModel.toggleSort(cellRoot.colKey);
                    }
                }
                onDoubleClicked: bridge.autoSizeColumn(cellRoot.colKey)
            }

            // Right-edge resize handle
            Item {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 6
                z: 10

                Rectangle {
                    anchors.centerIn: parent
                    width: 1
                    height: parent.height
                    color: resizeMouse.containsMouse ? Theme.accent : Theme.panelBorder
                }

                MouseArea {
                    id: resizeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    property int startX: 0
                    property int startW: 0
                    onPressed: (mouse) => {
                        startX = mouse.x;
                        startW = bridge.getColumnWidth(cellRoot.colKey);
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            var newW = Math.max(25, startW + (mouse.x - startX));
                            bridge.setColumnWidth(cellRoot.colKey, newW);
                        }
                    }
                    onDoubleClicked: bridge.autoSizeColumn(cellRoot.colKey)
                }
            }
        }

        // Table Header Row
        Rectangle {
            Layout.fillWidth: true
            height: 26
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            MouseArea {
                anchors.fill: parent
                z: -1
                acceptedButtons: Qt.RightButton
                onClicked: {
                    root.requestColumnHeaderContextMenu("", "");
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Repeater {
                    model: bridge.tableColumnKeys
                    delegate: TableHeaderCell {
                        colKey: modelData
                        colIndex: index
                        colTitle: bridge.tableColumns[modelData] ? bridge.tableColumns[modelData].title : ""
                        headerText: bridge.tableColumns[modelData] ? bridge.tableColumns[modelData].header : ""
                        isFillWidth: modelData === "title"
                    }
                }
            }
        }

        // Table Rows ScrollView
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: trackTableListView
                anchors.fill: parent
                model: bridge.trackModel
                clip: true

                MouseArea {
                    id: emptyViewportMouse
                    anchors.fill: parent
                    z: -1
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            root.requestViewportContextMenu();
                        } else {
                            bridge.trackModel.clearSelection();
                        }
                    }
                }

                delegate: Rectangle {
                    id: rowRect
                    width: trackTableListView.width
                    height: bridge.tableRowHeight > 0 ? bridge.tableRowHeight : 25
                    color: model.isSelected ? Theme.selection : ((bridge.currentFilePath === model.filePath) ? Theme.surfaceElevated : (tableRowMouse.containsMouse ? Theme.surfaceElevated : (index % 2 === 0 ? Theme.surface : Theme.background)))

                    property string rowId: model.id
                    property string rowTitle: model.title
                    property string rowArtist: model.artist
                    property string rowAlbum: model.album
                    property string rowAlbumArtist: (model.albumArtist && model.albumArtist !== "Unknown Artist") ? model.albumArtist : (model.artist || "")
                    property string rowGenre: model.genre
                    property int rowYear: model.year
                    property int rowTrackNumber: model.trackNumber
                    property string rowCodec: model.codec
                    property int rowBitrate: model.bitrate
                    property int rowSampleRate: model.sampleRate
                    property int rowBitDepth: model.bitDepth
                    property int rowPlayCount: model.playCount
                    property int rowRating: model.rating
                    property string rowDateAddedFormatted: model.dateAddedFormatted || ""
                    property string rowFilePath: model.filePath
                    property string rowFileSizeFormatted: model.fileSizeFormatted || ""
                    property string rowDurationFormatted: model.durationFormatted
                    property int rowIndex: index
                    property bool rowIsSelected: model.isSelected

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Repeater {
                            model: bridge.tableColumnKeys
                            delegate: Item {
                                id: cellItem
                                property string colKey: modelData
                                visible: bridge.isColumnVisible(colKey)
                                Layout.preferredWidth: bridge.getColumnWidth(colKey)
                                Layout.minimumWidth: colKey === "title" ? 100 : (colKey === "artist" || colKey === "album" ? 70 : 30)
                                Layout.fillWidth: colKey === "title"
                                Layout.fillHeight: true
                                clip: true

                                // 1. Track Number
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 4
                                    visible: cellItem.colKey === "trackNumber"
                                    Text {
                                        text: rowRect.rowTrackNumber > 0 ? rowRect.rowTrackNumber : (rowRect.rowIndex + 1)
                                        font.pixelSize: 10
                                        color: (bridge.currentFilePath === rowRect.rowFilePath) ? Theme.accent : Theme.textMuted
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.fillWidth: true
                                    }
                                    VectorIcon {
                                        visible: bridge.currentFilePath === rowRect.rowFilePath
                                        name: bridge.isPlaying ? "volume" : "play"
                                        width: 10
                                        height: 10
                                        color: Theme.accent
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }

                                // 2. Title
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    visible: cellItem.colKey === "title"
                                    text: rowRect.rowTitle
                                    font.pixelSize: 11
                                    font.bold: (bridge.currentFilePath === rowRect.rowFilePath)
                                    color: (bridge.currentFilePath === rowRect.rowFilePath) ? Theme.textPrimary : Theme.textSecondary
                                    elide: Text.ElideRight
                                }

                                // 3. Artist
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    visible: cellItem.colKey === "artist"
                                    text: rowRect.rowArtist
                                    font.pixelSize: 11
                                    color: (bridge.currentFilePath === rowRect.rowFilePath) ? Theme.textSecondary : Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 4. Album
                                Item {
                                    anchors.fill: parent
                                    visible: cellItem.colKey === "album"
                                    Text {
                                        id: albRowText
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        text: rowRect.rowAlbum
                                        font.pixelSize: 11
                                        font.underline: albTextMouse.containsMouse
                                        color: albTextMouse.containsMouse ? Theme.accentHover : Theme.textMuted
                                        elide: Text.ElideRight
                                    }
                                    MouseArea {
                                        id: albTextMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.requestExpandAlbum(rowRect.rowAlbum, rowRect.rowArtist);
                                        }
                                        ToolTip.visible: albTextMouse.containsMouse
                                        ToolTip.text: "Expand " + rowRect.rowAlbum
                                    }
                                }

                                // 5. Album Artist
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    visible: cellItem.colKey === "albumArtist"
                                    text: rowRect.rowAlbumArtist
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 6. Genre
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    visible: cellItem.colKey === "genre"
                                    text: rowRect.rowGenre
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 7. Year
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "year"
                                    text: rowRect.rowYear > 0 ? rowRect.rowYear : ""
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                }

                                // 8. Codec
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "codec"
                                    text: rowRect.rowCodec + (rowRect.rowBitDepth > 16 ? " " + rowRect.rowBitDepth + "b" : "")
                                    font.pixelSize: 10
                                    font.weight: Font.Medium
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                }

                                // 9. Bitrate
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "bitrate"
                                    text: rowRect.rowBitrate > 0 ? rowRect.rowBitrate + " kbps" : ""
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 10. Sample Rate
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "sampleRate"
                                    text: rowRect.rowSampleRate > 0 ? ((rowRect.rowSampleRate / 1000) + " kHz") : ""
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 11. Bit Depth
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "bitDepth"
                                    text: rowRect.rowBitDepth > 0 ? (rowRect.rowBitDepth + "-bit") : ""
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 12. Play Count
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "playCount"
                                    text: rowRect.rowPlayCount > 0 ? rowRect.rowPlayCount : ""
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                }

                                // 13. Rating
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "rating"
                                    text: rowRect.rowRating > 0 ? ("★".repeat(rowRect.rowRating) + "☆".repeat(5 - rowRect.rowRating)) : ""
                                    font.pixelSize: 10
                                    color: Theme.accent
                                }

                                // 14. Date Added
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "dateAdded"
                                    text: rowRect.rowDateAddedFormatted
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 15. File Path
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    visible: cellItem.colKey === "filePath"
                                    text: rowRect.rowFilePath
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideMiddle
                                }

                                // 16. File Size
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "fileSize"
                                    text: rowRect.rowFileSizeFormatted
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }

                                // 17. Duration
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: cellItem.colKey === "duration"
                                    text: rowRect.rowDurationFormatted
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: tableRowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onDoubleClicked: {
                            if (bridge.doubleClickAction === "Play Next") {
                                bridge.playNext(rowRect.rowId);
                            } else if (bridge.doubleClickAction === "Queue Last") {
                                bridge.queueLast(rowRect.rowId);
                            } else {
                                bridge.playTrackAtIndex(rowRect.rowIndex);
                            }
                        }
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                if (!rowRect.rowIsSelected) {
                                    bridge.trackModel.clearSelection();
                                    bridge.trackModel.setRowSelected(rowRect.rowIndex, true);
                                }
                                var sel = bridge.trackModel.getSelectedTracks();
                                var trk = { id: rowRect.rowId, title: rowRect.rowTitle, artist: rowRect.rowArtist, filePath: rowRect.rowFilePath, album: rowRect.rowAlbum, genre: rowRect.rowGenre, year: rowRect.rowYear, trackNumber: rowRect.rowTrackNumber };
                                root.requestTrackContextMenu(sel, trk, rowRect.rowFilePath, rowRect.rowTitle, rowRect.rowArtist, root.isPlaylistView, root.activePlaylistName);
                            } else {
                                if (mouse.modifiers & Qt.ControlModifier) {
                                    bridge.trackModel.toggleSelection(rowRect.rowIndex);
                                } else if (mouse.modifiers & Qt.ShiftModifier) {
                                    bridge.trackModel.selectRange(trackTableListView.currentIndex >= 0 ? trackTableListView.currentIndex : 0, rowRect.rowIndex);
                                } else {
                                    bridge.trackModel.clearSelection();
                                    bridge.trackModel.setRowSelected(rowRect.rowIndex, true);
                                    trackTableListView.currentIndex = rowRect.rowIndex;
                                }
                                bridge.openAlbumDetails(rowRect.rowAlbum, rowRect.rowArtist);
                            }
                        }
                    }
                }
            }
        }
    }
}
