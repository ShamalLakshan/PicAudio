import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: artistsGridRoot

    property string selectedArtist: ""
    property bool inDrilldown: selectedArtist.length > 0
    property int artRevision: 0

    readonly property var filteredArtists: {
        var list = bridge.artistsList;
        if (!list) return [];
        var sQuery = (bridge.albumModel && bridge.albumModel.searchQuery) ? bridge.albumModel.searchQuery.trim().toLowerCase() : "";
        var lFilter = (bridge.albumModel && bridge.albumModel.letterFilter) ? bridge.albumModel.letterFilter.trim() : "All";

        var filtered = list;
        if (sQuery.length > 0 || (lFilter.length > 0 && lFilter !== "All")) {
            filtered = list.filter(function(artist) {
                if (!artist || artist.length === 0) return false;
                if (sQuery.length > 0) {
                    if (artist.toLowerCase().indexOf(sQuery) === -1) return false;
                }
                if (lFilter.length > 0 && lFilter !== "All") {
                    var first = artist.charAt(0).toUpperCase();
                    if (lFilter === "#") {
                        if (first >= 'A' && first <= 'Z') return false;
                    } else {
                        if (first !== lFilter.toUpperCase()) return false;
                    }
                }
                return true;
            });
        }

        var unknownIdx = filtered.indexOf("Unknown Artist");
        if (unknownIdx > 0) {
            var copy = filtered.slice();
            var item = copy.splice(unknownIdx, 1)[0];
            copy.unshift(item);
            return copy;
        }
        return filtered;
    }

    Connections {
        target: bridge
        function onArtistArtChanged(artistName) {
            artistsGridRoot.artRevision++
        }
    }

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
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
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
                model: artistsGridRoot.filteredArtists
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    id: artItem
                    required property string modelData
                    readonly property string artistName: modelData
                    readonly property bool isUnknown: artItem.artistName === "Unknown Artist"
                    width: artistGrid.cellWidth - Math.round(12 * Theme.uiScale)
                    height: artistGrid.cellHeight - Math.round(12 * Theme.uiScale)
                    radius: Math.round(6 * Theme.uiScale)
                    color: artHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : Theme.surface
                    border.color: artItem.isUnknown ? (artHover.hovered ? Theme.accent : Theme.panelBorder) : Theme.panelBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(8 * Theme.uiScale)
                        spacing: Math.round(6 * Theme.uiScale)

                        // Circular photo frame with artist photo or fallback human head/profile icon
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: Math.round(96 * Theme.uiScale)
                            height: Math.round(96 * Theme.uiScale)
                            radius: width / 2
                            color: artHover.hovered ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : Theme.surfaceElevated
                            border.color: artHover.hovered ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            clip: true

                            VectorIcon {
                                anchors.centerIn: parent
                                width: Math.round(48 * Theme.uiScale)
                                height: Math.round(48 * Theme.uiScale)
                                name: "artist"
                                color: artHover.hovered ? Theme.accent : Theme.textSecondary
                                visible: artItem.isUnknown || artistImg.status !== Image.Ready
                            }

                            Image {
                                id: artistImg
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: false
                                source: (!artItem.isUnknown && artItem.artistName) ? ("image://artistart/" + encodeURIComponent(artItem.artistName) + (artistsGridRoot.artRevision > 0 ? ("?v=" + artistsGridRoot.artRevision) : "")) : ""
                                visible: !artItem.isUnknown && status === Image.Ready
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: artItem.artistName
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            readonly property int trkCount: bridge.getArtistTrackCount(artItem.artistName)
                            text: trkCount > 0 ? (trkCount === 1 ? qsTr("1 Track") : qsTr("%1 Tracks").arg(trkCount)) : qsTr("Discography")
                            color: Theme.accent
                            font.pixelSize: Math.round(10 * Theme.uiScale)
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    HoverHandler { id: artHover; cursorShape: Qt.PointingHandCursor }

                    TapHandler {
                        onTapped: {
                            artistsGridRoot.selectedArtist = artItem.artistName
                            bridge.openArtistDetails(artItem.artistName)
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: !artistsGridRoot.inDrilldown && artistsGridRoot.filteredArtists.length === 0
                text: qsTr("No artists found in library.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeMedium
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

                        // Circular header frame with artist photo or fallback human head/profile icon
                        Rectangle {
                            width: Math.round(80 * Theme.uiScale)
                            height: Math.round(80 * Theme.uiScale)
                            radius: width / 2
                            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
                            border.color: Theme.accent
                            border.width: 1
                            clip: true

                            VectorIcon {
                                anchors.centerIn: parent
                                width: Math.round(42 * Theme.uiScale)
                                height: Math.round(42 * Theme.uiScale)
                                name: "artist"
                                color: Theme.accent
                                visible: artistsGridRoot.selectedArtist === "Unknown Artist" || drillArtistImg.status !== Image.Ready
                            }

                            Image {
                                id: drillArtistImg
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: false
                                source: (artistsGridRoot.selectedArtist && artistsGridRoot.selectedArtist !== "Unknown Artist") ? ("image://artistart/" + encodeURIComponent(artistsGridRoot.selectedArtist) + (artistsGridRoot.artRevision > 0 ? ("?v=" + artistsGridRoot.artRevision) : "")) : ""
                                visible: artistsGridRoot.selectedArtist !== "Unknown Artist" && status === Image.Ready
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

                            Text {
                                readonly property int trkCount: bridge.getArtistTrackCount(artistsGridRoot.selectedArtist)
                                readonly property int albCount: bridge.getArtistAlbumCount(artistsGridRoot.selectedArtist)
                                text: (albCount > 0 ? (albCount === 1 ? qsTr("1 Album") : qsTr("%1 Albums").arg(albCount)) + " • " : "")
                                    + (trkCount === 1 ? qsTr("1 Track") : qsTr("%1 Tracks").arg(trkCount))
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSmall
                            }

                            Button {
                                text: qsTr("▶ Play Artist Discography")
                                onClicked: bridge.playArtistNow(artistsGridRoot.selectedArtist)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
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
