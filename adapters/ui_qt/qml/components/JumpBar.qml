import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: jumpBarRoot
    height: Math.round(26 * Theme.uiScale)
    color: Theme.surface
    border.color: Theme.panelBorder
    border.width: 1

    signal letterSelected(string letter)

    property var letters: ["All", "#", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]
    property string activeLetter: bridge.tabModel ? bridge.tabModel.activeTabId.length > 0 ? (bridge.tabModel.data(bridge.tabModel.index(bridge.tabModel.activeTabIndex, 0), 264 /* JumpBarLetterRole */) || "All") : "All" : "All"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Math.round(6 * Theme.uiScale)
        anchors.rightMargin: Math.round(6 * Theme.uiScale)
        spacing: 2

        Repeater {
            model: jumpBarRoot.letters

            delegate: Rectangle {
                id: letterBtn
                required property string modelData
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(20 * Theme.uiScale)
                radius: Math.round(3 * Theme.uiScale)
                color: (jumpBarRoot.activeLetter === letterBtn.modelData) ? Theme.accent : (lHover.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent")

                Text {
                    anchors.centerIn: parent
                    text: letterBtn.modelData
                    color: (jumpBarRoot.activeLetter === letterBtn.modelData) ? "#ffffff" : Theme.textSecondary
                    font.pixelSize: Math.round(10 * Theme.uiScale)
                    font.weight: (jumpBarRoot.activeLetter === letterBtn.modelData) ? Font.Bold : Font.Normal
                }

                HoverHandler { id: lHover; cursorShape: Qt.PointingHandCursor }

                TapHandler {
                    onTapped: {
                        jumpBarRoot.activeLetter = letterBtn.modelData
                        if (bridge.tabModel) {
                            bridge.tabModel.updateActiveJumpBarLetter(letterBtn.modelData)
                        }
                        if (bridge.trackModel) {
                            bridge.trackModel.setLetterFilter(letterBtn.modelData)
                        }
                        jumpBarRoot.letterSelected(letterBtn.modelData)
                    }
                }
            }
        }
    }
}
