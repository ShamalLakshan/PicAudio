import QtQuick
import QtQuick.Controls
import ".."

Slider {
    id: control
    hoverEnabled: true

    property bool bipolar: (from < 0 && to > 0)

    implicitWidth: horizontal ? 160 : 28
    implicitHeight: horizontal ? 28 : 160

    background: Item {
        x: control.leftPadding + (control.horizontal ? 0 : Math.round((control.availableWidth - 4) / 2))
        y: control.topPadding + (control.horizontal ? Math.round((control.availableHeight - 4) / 2) : 0)
        width: control.horizontal ? control.availableWidth : 4
        height: control.horizontal ? 4 : control.availableHeight

        // Background groove
        Rectangle {
            anchors.fill: parent
            radius: 2
            color: Theme.sliderTrack
            border.color: Theme.panelBorder
            border.width: 1
        }

        // Center zero indicator notch for vertical bipolar sliders (e.g. Equalizer)
        Rectangle {
            visible: !control.horizontal && control.bipolar
            anchors.horizontalCenter: parent.horizontalCenter
            y: Math.round(parent.height / 2 - 1)
            width: 10
            height: 2
            radius: 1
            color: Theme.textMuted
        }

        // Active filled progress groove
        Rectangle {
            radius: 2
            color: Theme.sliderTrackFilled

            x: {
                if (!control.horizontal) return 0;
                if (control.bipolar && (control.to > control.from)) {
                    var zeroPos = (0 - control.from) / (control.to - control.from);
                    var curPos = control.position;
                    return Math.round(Math.min(zeroPos, curPos) * parent.width);
                }
                return 0;
            }
            y: {
                if (control.horizontal) return 0;
                if (control.bipolar && (control.to > control.from)) {
                    var zeroVPos = 1.0 - (0 - control.from) / (control.to - control.from);
                    var curVPos = control.visualPosition;
                    return Math.round(Math.min(zeroVPos, curVPos) * parent.height);
                }
                return Math.round(control.visualPosition * parent.height);
            }
            width: {
                if (!control.horizontal) return parent.width;
                if (control.bipolar && (control.to > control.from)) {
                    var zeroPos = (0 - control.from) / (control.to - control.from);
                    var curPos = control.position;
                    return Math.max(2, Math.round(Math.abs(curPos - zeroPos) * parent.width));
                }
                return Math.round(control.position * parent.width);
            }
            height: {
                if (control.horizontal) return parent.height;
                if (control.bipolar && (control.to > control.from)) {
                    var zeroVPos = 1.0 - (0 - control.from) / (control.to - control.from);
                    var curVPos = control.visualPosition;
                    return Math.max(2, Math.round(Math.abs(curVPos - zeroVPos) * parent.height));
                }
                return Math.round((1.0 - control.visualPosition) * parent.height);
            }
        }
    }

    handle: Rectangle {
        x: Math.round(control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height)))
        implicitWidth: control.horizontal ? 16 : 18
        implicitHeight: control.horizontal ? 16 : 12
        radius: control.horizontal ? 8 : 4
        color: control.pressed ? Theme.textPrimary : (control.hovered ? Theme.sliderHandleHover : Theme.sliderHandle)
        border.color: control.hovered ? Theme.accentHover : Theme.panelBorder
        border.width: 1

        // Center line notch on vertical faders for realistic audio hardware tactile feel
        Rectangle {
            visible: !control.horizontal
            anchors.centerIn: parent
            width: parent.width - 6
            height: 2
            radius: 1
            color: control.pressed ? Theme.background : (control.hovered ? Theme.background : Theme.surface)
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.NoButton
    }
}
