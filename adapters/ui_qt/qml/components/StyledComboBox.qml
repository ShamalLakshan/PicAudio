import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

ComboBox {
    id: cbRoot
    Layout.preferredWidth: 220
    Layout.preferredHeight: 30
    hoverEnabled: true

    background: Rectangle {
        implicitWidth: 220
        implicitHeight: 30
        color: cbRoot.hovered ? Theme.surfaceElevated : Theme.background
        border.color: cbRoot.activeFocus ? Theme.accent : (cbRoot.hovered ? Theme.accentHover : Theme.panelBorder)
        border.width: 1
        radius: Theme.cornerRadiusSmall
    }

    contentItem: Text {
        leftPadding: Theme.spacingMedium + 2
        rightPadding: (cbRoot.indicator ? cbRoot.indicator.width : 10) + Theme.spacingMedium + 4
        text: cbRoot.displayText
        font.pixelSize: Theme.fontSizeSmall
        font.family: Theme.fontFamily
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        function positionToRectangle(pos) { return Qt.rect(0, 0, 0, 0); }
    }

    indicator: VectorIcon {
        x: cbRoot.width - width - Theme.spacingMedium - 2
        y: Math.round((cbRoot.height - height) / 2)
        width: 10
        height: 10
        name: "arrow_down"
        color: cbRoot.hovered ? Theme.accentHover : Theme.textSecondary
    }

    popup: Popup {
        y: cbRoot.height + 2
        width: Math.max(cbRoot.width, 140)
        implicitHeight: Math.min(260, Math.max(40, (contentItem.contentHeight || 0) + topPadding + bottomPadding + 8))
        padding: 4
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent | Popup.CloseOnPressOutside
        focus: true
        clip: true

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: cbRoot.delegateModel
            currentIndex: cbRoot.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }

        background: Rectangle {
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
    }

    delegate: ItemDelegate {
        id: delegateItem
        width: ListView.view ? ListView.view.width : cbRoot.width
        implicitHeight: 30
        highlighted: cbRoot.highlightedIndex === index
        hoverEnabled: true

        contentItem: Text {
            text: {
                if (cbRoot.textRole && cbRoot.textRole.length > 0) {
                    if (typeof modelData !== "undefined" && modelData && typeof modelData === "object" && modelData[cbRoot.textRole] !== undefined) {
                        return modelData[cbRoot.textRole];
                    }
                    if (typeof model !== "undefined" && model && model[cbRoot.textRole] !== undefined) {
                        return model[cbRoot.textRole];
                    }
                }
                if (typeof modelData !== "undefined" && modelData !== null) {
                    return modelData.toString();
                }
                if (typeof cbRoot.textAt === "function") {
                    return cbRoot.textAt(index);
                }
                return "";
            }
            color: (delegateItem.highlighted || delegateItem.hovered) ? Theme.accentHover : Theme.textPrimary
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: (delegateItem.highlighted || delegateItem.hovered) ? Theme.selection : "transparent"
            radius: Theme.cornerRadiusSmall
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.NoButton
        }

        onClicked: {
            cbRoot.currentIndex = index
            cbRoot.activated(index)
            cbRoot.popup.close()
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.NoButton
    }
}
