import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Dialog {
    id: confirmDropDialog
    title: qsTr("Batch Tagging Confirmation")
    modal: true
    width: Math.min(480, parent ? parent.width - 32 : 480)
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    property string tagField: ""
    property string targetValue: ""

    function promptTagUpdate(field, value) {
        tagField = field
        targetValue = value
        confirmDropDialog.open()
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cardCornerRadius
    }

    contentItem: ColumnLayout {
        spacing: Math.round(12 * Theme.uiScale)

        Text {
            text: qsTr("Update Tag for Selected Tracks?")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("This will batch update the %1 tag of the selected track(s) to \"%2\". This operation directly modifies file tags on disk.").arg(confirmDropDialog.tagField).arg(confirmDropDialog.targetValue)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.WordWrap
        }
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: Rectangle { color: Theme.surface; border.color: Theme.panelBorder; border.width: 1 }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: confirmDropDialog.close()
        }

        Button {
            text: qsTr("Apply Update")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                if (bridge.trackModel) {
                    bridge.trackModel.batchUpdateTag(confirmDropDialog.tagField, confirmDropDialog.targetValue)
                }
                confirmDropDialog.close()
            }
        }
    }
}
