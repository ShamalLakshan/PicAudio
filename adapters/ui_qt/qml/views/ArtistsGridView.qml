import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: artistsGridRoot

    property string selectedArtist: ""
    property bool inDrilldown: selectedArtist.length > 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Sub-toolbar / Breadcrumb
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

                Button {
                    text: qsTr("← All Artists")
                    visible: artistsGridRoot.inDrilldown
                    onClicked: artistsGridRoot.selectedArtist = ""
                }

                Text {
                    text: artistsGridRoot.inDrilldown ? qsTr("Artist: %1").arg(artistsGridRoot.selectedArtist) : qsTr("Artists Library")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }
            }
        }

        // View Content
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Grid of Artists (when not in drilldown)
            GridView {
                id: artistGrid
                anchors.fill: parent
                anchors.margins: Math.round(16 * Theme.uiScale)
                visible: !artistsGridRoot.inDrilldown
                clip: true
                cellWidth: Math.round(160 * Theme.uiScale)
                cellHeight: Math.round(190 * Theme.uiScale)
                model: bridge.albumModel // Groups by artist or album
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    id: artItem
                    required property var model
                    width: artistGrid.cellWidth - Math.round(12 * Theme.uiScale)
                    height: artistGrid.cellHeight - Math.round(12 * Theme.uiScale)
                    radius: Math.round(6 * Theme.uiScale)
                    color: artHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : Theme.surface
                    border.color: Theme.panelBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(8 * Theme.uiScale)
                        spacing: Math.round(6 * Theme.uiScale)

                        // Circular or rounded photo frame
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: Math.round(100 * Theme.uiScale)
                            height: Math.round(100 * Theme.uiScale)
                            radius: width / 2
                            color: Theme.background
                            border.color: Theme.panelBorder
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: "image://albumart/" + (artItem.model.artHash ? artItem.model.artHash : artItem.model.id)
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: artItem.model.artist
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Discography")
                            color: Theme.accent
                            font.pixelSize: Math.round(10 * Theme.uiScale)
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    HoverHandler { id: artHover; cursorShape: Qt.PointingHandCursor }

                    TapHandler {
                        onTapped: {
                            artistsGridRoot.selectedArtist = artItem.model.artist
                            bridge.selectAlbum(artItem.model.title, artItem.model.artist)
                        }
                    }
                }
            }

            // Drilldown View (when an artist is clicked)
            Rectangle {
                anchors.fill: parent
                visible: artistsGridRoot.inDrilldown
                color: Theme.background

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(16 * Theme.uiScale)
                    spacing: Math.round(12 * Theme.uiScale)

                    RowLayout {
                        spacing: Math.round(16 * Theme.uiScale)

                        Rectangle {
                            width: Math.round(84 * Theme.uiScale)
                            height: Math.round(84 * Theme.uiScale)
                            radius: width / 2
                            color: Theme.surface
                            border.color: Theme.panelBorder
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: "image://albumart/" + artistsGridRoot.selectedArtist
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }

                        ColumnLayout {
                            spacing: Math.round(4 * Theme.uiScale)

                            Text {
                                text: artistsGridRoot.selectedArtist
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeLarge
                                font.bold: true
                            }

                            Button {
                                text: qsTr("▶ Play Artist Discography")
                                onClicked: bridge.playAlbumNow("", artistsGridRoot.selectedArtist)
                            }
                        }
                    }

                    // Artist Tracks
                    ListView {
                        id: drillTracks
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: bridge.albumDetailTrackModel
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            required property var model
                            width: drillTracks.width
                            height: Math.round(36 * Theme.uiScale)
                            color: drillHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(12 * Theme.uiScale)
                                anchors.rightMargin: Math.round(12 * Theme.uiScale)

                                Text {
                                    text: model.title
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: model.album || ""
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSmall
                                    Layout.preferredWidth: Math.round(180 * Theme.uiScale)
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: model.durationStr || ""
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeSmall
                                }
                            }

                            HoverHandler { id: drillHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onDoubleTapped: bridge.playTrackNow(model.id)
                            }
                        }
                    }
                }
            }
        }
    }
}
