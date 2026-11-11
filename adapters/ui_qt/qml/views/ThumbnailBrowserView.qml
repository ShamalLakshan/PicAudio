import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: thumbBrowserRoot
    color: Theme.surface
    border.color: Theme.panelBorder
    border.width: 1

    property string category: "Artist" // "Artist" | "Album" | "Genre"
    property string selectedValue: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header with Category Selector
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(38 * Theme.uiScale)
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.round(8 * Theme.uiScale)
                anchors.rightMargin: Math.round(8 * Theme.uiScale)

                Text {
                    text: qsTr("Thumbnail Browser")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    Layout.preferredWidth: Math.round(90 * Theme.uiScale)
                    model: ["Artist", "Album", "Genre"]
                    currentIndex: model.indexOf(thumbBrowserRoot.category)
                    onActivated: function(idx) { thumbBrowserRoot.category = model[idx] }
                }
            }
        }

        // List View
        ListView {
            id: thumbList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: bridge.albumModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: itemRect
                required property var model
                width: thumbList.width
                height: Math.round(44 * Theme.uiScale)
                color: (thumbBrowserRoot.selectedValue === (thumbBrowserRoot.category === "Genre" ? itemRect.model.genre : (thumbBrowserRoot.category === "Album" ? itemRect.model.title : itemRect.model.artist))) ?
                       Qt.rgba(0.2, 0.6, 1.0, 0.15) : (tbHover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Math.round(8 * Theme.uiScale)
                    anchors.rightMargin: Math.round(8 * Theme.uiScale)
                    spacing: Math.round(8 * Theme.uiScale)

                    Rectangle {
                        Layout.preferredWidth: Math.round(30 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(30 * Theme.uiScale)
                        radius: Math.round(3 * Theme.uiScale)
                        color: Theme.background
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: "image://albumart/" + (itemRect.model.artHash ? itemRect.model.artHash : itemRect.model.id)
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: thumbBrowserRoot.category === "Genre" ? (itemRect.model.genre || "Unknown") :
                              (thumbBrowserRoot.category === "Album" ? itemRect.model.title : itemRect.model.artist)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSmall
                        elide: Text.ElideRight
                    }
                }

                HoverHandler { id: tbHover; cursorShape: Qt.PointingHandCursor }

                TapHandler {
                    onTapped: {
                        var val = thumbBrowserRoot.category === "Genre" ? itemRect.model.genre :
                                  (thumbBrowserRoot.category === "Album" ? itemRect.model.title : itemRect.model.artist)
                        thumbBrowserRoot.selectedValue = val
                        if (thumbBrowserRoot.category === "Artist") {
                            bridge.trackModel.setArtistFilter(val)
                        } else if (thumbBrowserRoot.category === "Genre") {
                            bridge.trackModel.setGenreFilter(val)
                        } else {
                            bridge.selectAlbum(itemRect.model.title, itemRect.model.artist)
                        }
                    }
                }

                // DropArea for Drag-and-Drop Metadata Tagging
                DropArea {
                    anchors.fill: parent
                    onEntered: itemRect.color = Qt.rgba(0.2, 0.8, 0.4, 0.2)
                    onExited: itemRect.color = "transparent"
                    onDropped: function(drop) {
                        var targetVal = thumbBrowserRoot.category === "Genre" ? itemRect.model.genre : itemRect.model.artist
                        if (confirmDropDialogInstance) {
                            confirmDropDialogInstance.promptTagUpdate(thumbBrowserRoot.category, targetVal)
                        }
                    }
                }
            }
        }
    }
}
