import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: albumCoversRoot

    property int thumbnailSize: (bridge.viewRegistry && bridge.tabModel) ? (bridge.viewRegistry.getView(bridge.tabModel.activeViewId).thumbnailSize || 160) : 160
    property string selectedAlbumTitle: ""
    property string selectedAlbumArtist: ""
    property string selectedAlbumArtSource: ""
    property bool drawerOpen: selectedAlbumTitle.length > 0

    signal requestAlbumContextMenu(string title, string artist)
    signal requestTrackContextMenu(var track)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Sub-toolbar with thumbnail size slider and album count
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(36 * Theme.uiScale)
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.round(12 * Theme.uiScale)
                anchors.rightMargin: Math.round(12 * Theme.uiScale)

                Text {
                    text: qsTr("%1 Albums").arg(bridge.albumModel ? bridge.albumModel.rowCount() : 0)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: qsTr("Size:")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                Slider {
                    id: sizeSlider
                    Layout.preferredWidth: Math.round(120 * Theme.uiScale)
                    from: 100
                    to: 280
                    stepSize: 10
                    value: albumCoversRoot.thumbnailSize
                    onMoved: albumCoversRoot.thumbnailSize = value
                }
            }
        }

        // Main Area: Grid + Expandable Drawer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridView {
                id: grid
                anchors.fill: parent
                anchors.margins: Math.round(12 * Theme.uiScale)
                anchors.bottomMargin: drawerOpen ? Math.round(280 * Theme.uiScale) : Math.round(12 * Theme.uiScale)
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                cellWidth: albumCoversRoot.thumbnailSize + Math.round(20 * Theme.uiScale)
                cellHeight: albumCoversRoot.thumbnailSize + Math.round(68 * Theme.uiScale)
                model: bridge.albumModel

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                    id: cardItem
                    required property var model
                    width: grid.cellWidth - Math.round(10 * Theme.uiScale)
                    height: grid.cellHeight - Math.round(10 * Theme.uiScale)
                    radius: Math.round(4 * Theme.uiScale)
                    color: (albumCoversRoot.selectedAlbumTitle === cardItem.model.title && albumCoversRoot.selectedAlbumArtist === cardItem.model.artist) ?
                           Qt.rgba(0.2, 0.6, 1.0, 0.15) : (cardHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : Theme.surface)
                    border.color: (albumCoversRoot.selectedAlbumTitle === cardItem.model.title && albumCoversRoot.selectedAlbumArtist === cardItem.model.artist) ?
                                  Theme.accent : Theme.panelBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(6 * Theme.uiScale)
                        spacing: Math.round(4 * Theme.uiScale)

                        // Artwork Frame
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: cardItem.width - Math.round(12 * Theme.uiScale)
                            color: Theme.background
                            clip: true
                            radius: Math.round(2 * Theme.uiScale)

                            Image {
                                anchors.fill: parent
                                source: "image://albumart/" + (cardItem.model.artHash ? cardItem.model.artHash : cardItem.model.id)
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }

                            // Play Button Hover Overlay
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                                anchors.margins: Math.round(6 * Theme.uiScale)
                                width: Math.round(28 * Theme.uiScale)
                                height: Math.round(28 * Theme.uiScale)
                                radius: width / 2
                                color: Theme.accent
                                visible: cardHover.hovered

                                Text {
                                    anchors.centerIn: parent
                                    text: "▶"
                                    color: "#ffffff"
                                    font.pixelSize: Math.round(12 * Theme.uiScale)
                                }

                                TapHandler {
                                    onTapped: bridge.playAlbumNow(cardItem.model.title, cardItem.model.artist)
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: cardItem.model.title
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: cardItem.model.artist + (cardItem.model.year > 0 ? " • " + cardItem.model.year : "")
                            color: Theme.textSecondary
                            font.pixelSize: Math.round(10 * Theme.uiScale)
                            elide: Text.ElideRight
                        }
                    }

                    HoverHandler { id: cardHover; cursorShape: Qt.PointingHandCursor }

                    TapHandler {
                        onTapped: {
                            if (albumCoversRoot.selectedAlbumTitle === cardItem.model.title && albumCoversRoot.selectedAlbumArtist === cardItem.model.artist) {
                                albumCoversRoot.selectedAlbumTitle = ""
                                albumCoversRoot.selectedAlbumArtist = ""
                                albumCoversRoot.selectedAlbumArtSource = ""
                            } else {
                                albumCoversRoot.selectedAlbumTitle = cardItem.model.title
                                albumCoversRoot.selectedAlbumArtist = cardItem.model.artist
                                albumCoversRoot.selectedAlbumArtSource = "image://albumart/" + (cardItem.model.artHash ? cardItem.model.artHash : cardItem.model.id)
                                bridge.selectAlbum(cardItem.model.title, cardItem.model.artist)
                            }
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: albumCoversRoot.requestAlbumContextMenu(cardItem.model.title, cardItem.model.artist)
                    }
                }
            }

            // Inline Drawer / Sub-panel (slides up from bottom of grid)
            Rectangle {
                id: inlineDrawer
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: drawerOpen ? Math.round(270 * Theme.uiScale) : 0
                visible: height > 0
                clip: true
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                Behavior on height {
                    NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(14 * Theme.uiScale)
                    spacing: Math.round(16 * Theme.uiScale)

                    // Left Side: Album Info & Art Card
                    Rectangle {
                        Layout.preferredWidth: Math.round(320 * Theme.uiScale)
                        Layout.fillHeight: true
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            spacing: Math.round(12 * Theme.uiScale)

                            // Album Art Box
                            Rectangle {
                                Layout.preferredWidth: Math.round(130 * Theme.uiScale)
                                Layout.preferredHeight: Math.round(130 * Theme.uiScale)
                                Layout.alignment: Qt.AlignVCenter
                                color: Theme.background
                                border.color: Theme.panelBorder
                                border.width: 1
                                radius: Math.round(4 * Theme.uiScale)
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: albumCoversRoot.selectedAlbumArtSource.length > 0 ?
                                            albumCoversRoot.selectedAlbumArtSource :
                                            ("image://albumart/" + bridge.selectedAlbumArtHash)
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: Math.round(36 * Theme.uiScale)
                                    height: Math.round(36 * Theme.uiScale)
                                    radius: width / 2
                                    color: Theme.accent
                                    opacity: artHover.containsMouse ? 0.95 : 0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "▶"
                                        color: "#ffffff"
                                        font.pixelSize: Math.round(14 * Theme.uiScale)
                                    }

                                    MouseArea {
                                        id: artHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: bridge.playAlbumNow(albumCoversRoot.selectedAlbumTitle, albumCoversRoot.selectedAlbumArtist)
                                    }
                                }
                            }

                            // Album Details Column
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: Math.round(5 * Theme.uiScale)

                                Text {
                                    text: albumCoversRoot.selectedAlbumTitle
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeLarge
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: albumCoversRoot.selectedAlbumArtist
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeBase
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    visible: (bridge.selectedAlbumYear && bridge.selectedAlbumYear.length > 0) || (bridge.selectedAlbumGenre && bridge.selectedAlbumGenre.length > 0)
                                    text: (bridge.selectedAlbumYear ? bridge.selectedAlbumYear : "") +
                                          ((bridge.selectedAlbumYear && bridge.selectedAlbumGenre) ? " • " : "") +
                                          (bridge.selectedAlbumGenre || "")
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSmall
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: qsTr("%1 tracks • %2").arg(bridge.albumDetailTrackModel ? bridge.albumDetailTrackModel.rowCount() : 0).arg(bridge.selectedAlbumDuration || "0:00")
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSizeSmall
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                // Buttons Row
                                RowLayout {
                                    spacing: Math.round(8 * Theme.uiScale)
                                    Layout.topMargin: Math.round(6 * Theme.uiScale)

                                    Rectangle {
                                        Layout.preferredWidth: playAllText.implicitWidth + 24
                                        Layout.preferredHeight: Math.round(28 * Theme.uiScale)
                                        radius: Theme.cornerRadiusSmall
                                        color: playAllHover.hovered ? Theme.accentHover : Theme.accent

                                        RowLayout {
                                            anchors.centerIn: parent
                                            spacing: 6
                                            Text { text: "▶"; color: "#ffffff"; font.pixelSize: 10 }
                                            Text { id: playAllText; text: qsTr("Play Album"); color: "#ffffff"; font.bold: true; font.pixelSize: Theme.fontSizeSmall }
                                        }

                                        HoverHandler { id: playAllHover; cursorShape: Qt.PointingHandCursor }
                                        TapHandler {
                                            onTapped: bridge.playAlbumNow(albumCoversRoot.selectedAlbumTitle, albumCoversRoot.selectedAlbumArtist)
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: queueAllText.implicitWidth + 20
                                        Layout.preferredHeight: Math.round(28 * Theme.uiScale)
                                        radius: Theme.cornerRadiusSmall
                                        color: queueAllHover.hovered ? Theme.surface : "transparent"
                                        border.color: Theme.panelBorder
                                        border.width: 1

                                        RowLayout {
                                            anchors.centerIn: parent
                                            spacing: 4
                                            Text { text: "＋"; color: Theme.textPrimary; font.pixelSize: 11 }
                                            Text { id: queueAllText; text: qsTr("Queue"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSmall }
                                        }

                                        HoverHandler { id: queueAllHover; cursorShape: Qt.PointingHandCursor }
                                        TapHandler {
                                            onTapped: bridge.queueAlbumLast(albumCoversRoot.selectedAlbumTitle, albumCoversRoot.selectedAlbumArtist)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 1px Vertical Separator
                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        color: Theme.panelBorder
                    }

                    // Right Side: Tracklist for this album
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surface
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Math.round(4 * Theme.uiScale)
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Track Header Row
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.round(26 * Theme.uiScale)
                                color: Theme.background
                                border.color: Theme.panelBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Math.round(10 * Theme.uiScale)
                                    anchors.rightMargin: Math.round(12 * Theme.uiScale)
                                    spacing: Math.round(8 * Theme.uiScale)

                                    Text {
                                        text: "#"
                                        color: Theme.textMuted
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                        font.bold: true
                                        Layout.preferredWidth: Math.round(26 * Theme.uiScale)
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    Text {
                                        text: qsTr("TITLE")
                                        color: Theme.textMuted
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: qsTr("DURATION")
                                        color: Theme.textMuted
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                        font.bold: true
                                        Layout.preferredWidth: Math.round(60 * Theme.uiScale)
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }

                            // Tracks ListView
                            ListView {
                                id: detailTrackList
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                model: bridge.albumDetailTrackModel
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                delegate: Rectangle {
                                    id: trackItem
                                    required property var model
                                    required property int index
                                    width: detailTrackList.width
                                    height: Math.round(28 * Theme.uiScale)
                                    color: trackHover.hovered ? Theme.selection : (index % 2 === 1 ? Qt.rgba(1, 1, 1, 0.02) : "transparent")

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: Math.round(10 * Theme.uiScale)
                                        anchors.rightMargin: Math.round(12 * Theme.uiScale)
                                        spacing: Math.round(8 * Theme.uiScale)

                                        Text {
                                            text: trackItem.model.trackNumber > 0 ? trackItem.model.trackNumber : ""
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontSizeSmall
                                            Layout.preferredWidth: Math.round(26 * Theme.uiScale)
                                            horizontalAlignment: Text.AlignRight
                                        }

                                        Text {
                                            text: trackItem.model.title
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeSmall
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            text: trackItem.model.durationFormatted || trackItem.model.durationStr || ""
                                            color: Theme.textSecondary
                                            font.pixelSize: Math.round(10 * Theme.uiScale)
                                            Layout.preferredWidth: Math.round(60 * Theme.uiScale)
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    HoverHandler { id: trackHover; cursorShape: Qt.PointingHandCursor }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        onDoubleTapped: bridge.playTrackFromDetail(trackItem.index)
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.RightButton
                                        onTapped: {
                                            var trk = {
                                                id: trackItem.model.id || "",
                                                title: trackItem.model.title || "",
                                                artist: trackItem.model.artist || "",
                                                filePath: trackItem.model.filePath || "",
                                                album: trackItem.model.album || "",
                                                genre: trackItem.model.genre || "",
                                                year: trackItem.model.year || 0,
                                                trackNumber: trackItem.model.trackNumber || 0
                                            };
                                            albumCoversRoot.requestTrackContextMenu(trk);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Close Drawer Button
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: Math.round(26 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(26 * Theme.uiScale)
                        radius: Math.round(4 * Theme.uiScale)
                        color: closeHover.hovered ? Theme.surface : "transparent"
                        border.color: closeHover.hovered ? Theme.panelBorder : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: Theme.textSecondary
                            font.pixelSize: Math.round(12 * Theme.uiScale)
                        }

                        HoverHandler { id: closeHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            onTapped: {
                                albumCoversRoot.selectedAlbumTitle = ""
                                albumCoversRoot.selectedAlbumArtist = ""
                                albumCoversRoot.selectedAlbumArtSource = ""
                            }
                        }
                    }
                }
            }
        }
    }
}
