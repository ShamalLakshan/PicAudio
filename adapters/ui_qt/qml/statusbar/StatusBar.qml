import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    Layout.fillWidth: true
    Layout.preferredHeight: 22
    color: Theme.surface
    border.color: Theme.panelBorder
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Text {
            text: bridge.totalTracks + " tracks | " +
                  bridge.totalAlbums + " albums | " +
                  bridge.totalLibrarySizeStr + " | Total Time: " +
                  bridge.totalDurationStr
            font.pixelSize: 9
            color: Theme.textMuted
            elide: Text.ElideRight
            Layout.maximumWidth: parent.width * 0.6
        }

        Item { Layout.fillWidth: true }

        // Indexing and library scanning progress status
        RowLayout {
            spacing: 6
            visible: bridge.isScanning || bridge.scanStatusText.length > 0

            Rectangle {
                width: 5
                height: 5
                radius: 2.5
                color: bridge.isScanning ? Theme.accent : Theme.textMuted
            }

            Text {
                text: bridge.scanStatusText
                font.pixelSize: 9
                color: bridge.isScanning ? Theme.accentHover : Theme.textMuted
            }
        }

        Rectangle {
            width: 1
            height: 12
            color: Theme.panelBorder
            visible: bridge.isScanning || bridge.scanStatusText.length > 0
        }

        Text {
            text: "PicAudio v0.1.0"
            font.pixelSize: 9
            color: Theme.textMuted
        }
    }
}
