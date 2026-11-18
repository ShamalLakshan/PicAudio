import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: albumAndTracksRoot

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left Panel: Albums List / Selector
        Rectangle {
            Layout.preferredWidth: Math.round(260 * Theme.uiScale)
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(36 * Theme.uiScale)
                    color: Theme.background
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Math.round(10 * Theme.uiScale)
                        anchors.rightMargin: Math.round(10 * Theme.uiScale)

                        Text {
                            text: qsTr("Albums (%1)").arg(bridge.albumModel ? bridge.albumModel.rowCount() : 0)
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Albums List
                ListView {
                    id: albList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.albumModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        id: albItem
                        required property var model
                        readonly property bool isUnknown: albItem.model.isSpecialUntagged || albItem.model.isUnknownAlbum || albItem.model.title === "Unknown Album" || albItem.model.title === "Untagged & Loose Tracks"
                        width: albList.width
                        height: Math.round(52 * Theme.uiScale)
                        color: (bridge.selectedAlbumTitle === albItem.model.title && bridge.selectedAlbumArtist === albItem.model.artist) ?
                               Qt.rgba(0.2, 0.6, 1.0, 0.15) : (albHover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                        border.color: (bridge.selectedAlbumTitle === albItem.model.title && bridge.selectedAlbumArtist === albItem.model.artist) ?
                                      Theme.accent : "transparent"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Math.round(8 * Theme.uiScale)
                            anchors.rightMargin: Math.round(8 * Theme.uiScale)
                            spacing: Math.round(8 * Theme.uiScale)

                            Rectangle {
                                Layout.preferredWidth: Math.round(36 * Theme.uiScale)
                                Layout.preferredHeight: Math.round(36 * Theme.uiScale)
                                color: Theme.background
                                radius: Math.round(2 * Theme.uiScale)
                                clip: true

                                VectorIcon {
                                    anchors.centerIn: parent
                                    name: "album"
                                    width: Math.round(20 * Theme.uiScale)
                                    height: Math.round(20 * Theme.uiScale)
                                    color: albHover.hovered ? Theme.accent : Theme.textSecondary
                                    visible: albItem.isUnknown || albImg.status !== Image.Ready
                                }

                                Image {
                                    id: albImg
                                    anchors.fill: parent
                                    source: (!albItem.isUnknown && albItem.model.artHash) ? ("image://albumart/" + albItem.model.artHash) : (!albItem.isUnknown ? ("image://albumart/" + albItem.model.id) : "")
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    visible: !albItem.isUnknown && status === Image.Ready
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: albItem.model.title
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: albItem.model.artist
                                    color: Theme.textSecondary
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        HoverHandler { id: albHover; cursorShape: Qt.PointingHandCursor }

                        TapHandler {
                            onTapped: bridge.selectAlbum(albItem.model.title, albItem.model.artist)
                        }
                    }
                }
            }
        }

        // Right Panel: Selected Album Artwork Card + Tracklist
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.background

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Album Header Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(140 * Theme.uiScale)
                    color: Theme.surface
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(16 * Theme.uiScale)
                        spacing: Math.round(16 * Theme.uiScale)

                        Rectangle {
                            Layout.preferredWidth: Math.round(108 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(108 * Theme.uiScale)
                            color: Theme.background
                            border.color: Theme.panelBorder
                            radius: Math.round(4 * Theme.uiScale)
                            clip: true

                            VectorIcon {
                                anchors.centerIn: parent
                                name: "album"
                                width: Math.round(44 * Theme.uiScale)
                                height: Math.round(44 * Theme.uiScale)
                                color: Theme.accent
                                visible: (bridge.selectedAlbumTitle === "Unknown Album" || bridge.selectedAlbumTitle === "Untagged & Loose Tracks") || albumHeaderImg.status !== Image.Ready
                            }

                            Image {
                                id: albumHeaderImg
                                anchors.fill: parent
                                source: (bridge.selectedAlbumTitle !== "Unknown Album" && bridge.selectedAlbumTitle !== "Untagged & Loose Tracks") ? ("image://albumart/" + bridge.selectedAlbumTitle) : ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: (bridge.selectedAlbumTitle !== "Unknown Album" && bridge.selectedAlbumTitle !== "Untagged & Loose Tracks") && status === Image.Ready
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Math.round(4 * Theme.uiScale)

                            Text {
                                text: bridge.selectedAlbumTitle.length > 0 ? bridge.selectedAlbumTitle : qsTr("Select an Album")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeLarge
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: bridge.selectedAlbumArtist
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeBase
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                spacing: Math.round(8 * Theme.uiScale)
                                visible: bridge.selectedAlbumTitle.length > 0

                                Button {
                                    text: qsTr("▶ Play Album")
                                    onClicked: bridge.playAlbumNow(bridge.selectedAlbumTitle, bridge.selectedAlbumArtist)
                                }

                                Button {
                                    text: qsTr("+ Queue Album")
                                    onClicked: bridge.queueAlbum(bridge.selectedAlbumTitle, bridge.selectedAlbumArtist)
                                }
                            }
                        }
                    }
                }

                // Album Tracks Table
                ListView {
                    id: tracksList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.albumDetailTrackModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        required property var model
                        width: tracksList.width
                        height: Math.round(36 * Theme.uiScale)
                        color: trackItemHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                        border.color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Math.round(12 * Theme.uiScale)
                            anchors.rightMargin: Math.round(12 * Theme.uiScale)
                            spacing: Math.round(12 * Theme.uiScale)

                            Text {
                                text: model.displayTrackNumber || (model.trackNumber > 0 ? model.trackNumber : "-")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                                Layout.preferredWidth: Math.round(28 * Theme.uiScale)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: model.title
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: bridge.selectedAlbumTitle === "Untagged & Loose Tracks"
                                    text: (model.artist && model.artist !== "Unknown Artist") ? model.artist : qsTr("Unknown Artist")
                                    color: Theme.textMuted
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            Text {
                                text: model.audiophileBadge || model.codec || ""
                                color: Theme.accent
                                font.pixelSize: Math.round(10 * Theme.uiScale)
                                Layout.preferredWidth: Math.round(120 * Theme.uiScale)
                            }

                            Text {
                                text: model.durationStr || ""
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }

                        HoverHandler { id: trackItemHover; cursorShape: Qt.PointingHandCursor }

                        TapHandler {
                            onDoubleTapped: bridge.playTrackNow(model.id)
                        }
                    }
                }
            }
        }
    }
}
