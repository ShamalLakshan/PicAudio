import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../views"

Window {
    id: floatingWin
    width: Math.round(480 * Theme.uiScale)
    height: Math.round(560 * Theme.uiScale)
    minimumWidth: Math.round(320 * Theme.uiScale)
    minimumHeight: Math.round(320 * Theme.uiScale)
    title: qsTr("PicAudio - Detached Floating Panel")
    color: Theme.background
    visible: bridge.layoutModel.isFloatingWindowActive

    property var floatingElements: bridge.layoutModel.getElementsForZone("floatingWindow")

    Connections {
        target: bridge.layoutModel
        function onLayoutChanged() {
            floatingWin.floatingElements = bridge.layoutModel.getElementsForZone("floatingWindow")
            floatingWin.visible = bridge.layoutModel.isFloatingWindowActive
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header strip
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
                    text: qsTr("Detached Workspace Panel")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: qsTr("(Configure in Arrange Panels)")
                    color: Theme.textSecondary
                    font.pixelSize: Math.round(10 * Theme.uiScale)
                }
            }
        }

        // Content Area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.background

            ListView {
                anchors.fill: parent
                model: floatingWin.floatingElements
                clip: true

                delegate: Item {
                    required property var modelData
                    width: parent.width
                    height: Math.round(300 * Theme.uiScale)

                    // Render component based on elementId
                    Loader {
                        anchors.fill: parent
                        sourceComponent: {
                            if (modelData.elementId === "lyrics") return lyricsComp
                            if (modelData.elementId === "spectrum_visualizer") return visualizerComp
                            if (modelData.elementId === "playing_tracks") return queueComp
                            return placeholderComp
                        }
                    }

                    Component {
                        id: lyricsComp
                        Item {
                            Text {
                                anchors.centerIn: parent
                                text: bridge.currentTrackLyrics.length > 0 ? bridge.currentTrackLyrics : qsTr("No synchronized lyrics found")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    Component {
                        id: visualizerComp
                        WaveVisualizer {
                            anchors.fill: parent
                        }
                    }

                    Component {
                        id: queueComp
                        QueueView {
                            anchors.fill: parent
                        }
                    }

                    Component {
                        id: placeholderComp
                        Rectangle {
                            color: Theme.surface
                            border.color: Theme.panelBorder
                            Text {
                                anchors.centerIn: parent
                                text: modelData.name
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeBase
                            }
                        }
                    }
                }
            }
        }
    }
}
