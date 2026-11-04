import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property string text: ""
    property color textColor: Theme.accentHover
    property color badgeColor: Theme.selection
    property color badgeBorderColor: Theme.accent
    property int badgeBorderWidth: 1
    property int badgeRadius: Theme.cornerRadiusSmall
    property int fontSize: Theme.fontSizeSmall - 1
    property bool fontBold: true
    property int horizontalPadding: 6
    property int verticalPadding: 2
    property int maxWidth: -1

    property alias borderColor: root.badgeBorderColor
    property alias bold: root.fontBold

    visible: text.length > 0
    clip: true

    implicitWidth: Math.min(maxWidth > 0 ? maxWidth : 10000, label.implicitWidth + horizontalPadding * 2)
    implicitHeight: Math.max(14, label.implicitHeight + verticalPadding * 2)
    width: implicitWidth
    height: implicitHeight

    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: implicitHeight
    Layout.minimumWidth: implicitWidth
    Layout.maximumWidth: maxWidth > 0 ? maxWidth : Number.POSITIVE_INFINITY
    Layout.alignment: Qt.AlignVCenter

    color: badgeColor
    border.color: badgeBorderColor
    border.width: badgeBorderWidth
    radius: badgeRadius

    Text {
        id: label
        anchors.fill: parent
        anchors.leftMargin: root.horizontalPadding
        anchors.rightMargin: root.horizontalPadding
        anchors.topMargin: root.verticalPadding
        anchors.bottomMargin: root.verticalPadding
        text: root.text
        font.pixelSize: root.fontSize
        font.family: Theme.fontFamily
        font.bold: root.fontBold
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
