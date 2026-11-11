import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: libraryExplorerRoot
    color: Theme.surface
    border.color: Theme.panelBorder
    border.width: 1

    property string searchFilter: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header & Type-To-Search Box
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(72 * Theme.uiScale)
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(8 * Theme.uiScale)
                spacing: Math.round(4 * Theme.uiScale)

                Text {
                    text: qsTr("Library Explorer")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Type to search...")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSmall
                    background: Rectangle {
                        color: Theme.surface
                        border.color: Theme.panelBorder
                        radius: Math.round(3 * Theme.uiScale)
                    }
                    onTextChanged: libraryExplorerRoot.searchFilter = text.toLowerCase()
                }
            }
        }

        // Tree / Categorized Hierarchy
        ListView {
            id: explorerList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: bridge.albumModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: nodeItem
                required property var model
                visible: libraryExplorerRoot.searchFilter.length === 0 ||
                         model.artist.toLowerCase().indexOf(libraryExplorerRoot.searchFilter) !== -1 ||
                         model.title.toLowerCase().indexOf(libraryExplorerRoot.searchFilter) !== -1

                width: explorerList.width
                height: visible ? Math.round(32 * Theme.uiScale) : 0
                color: nodeHover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Math.round(12 * Theme.uiScale)
                    anchors.rightMargin: Math.round(8 * Theme.uiScale)
                    spacing: Math.round(6 * Theme.uiScale)

                    Text {
                        text: "📁"
                        font.pixelSize: Math.round(11 * Theme.uiScale)
                    }

                    Text {
                        Layout.fillWidth: true
                        text: model.artist + " / " + model.title
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSmall
                        elide: Text.ElideRight
                    }
                }

                HoverHandler { id: nodeHover; cursorShape: Qt.PointingHandCursor }

                TapHandler {
                    onTapped: bridge.selectAlbum(model.title, model.artist)
                }
            }
        }
    }
}
