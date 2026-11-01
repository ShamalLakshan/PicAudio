import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 700)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 550)) - height) / 2)
    width: Math.min(680, Overlay.overlay ? Overlay.overlay.width - 32 : 680)
    height: Math.min(520, Overlay.overlay ? Overlay.overlay.height - 32 : 520)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string selectedCategory: "All"

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 700);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 550);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        filterInput.text = "";
        selectedCategory = "All";
        filterInput.forceActiveFocus();
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                MovableHeaderArea {
                    target: root
                }

                RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                VectorIcon {
                    name: "info"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "Keyboard Shortcuts Cheat Sheet"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: closeMouse.containsMouse ? Theme.selection : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "clear"
                        width: 10
                        height: 10
                        color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        // Search & Category Filter Bar
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.panelPadding
            spacing: Theme.spacingSmall

            // Search box
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: Theme.background
                border.color: filterInput.activeFocus ? Theme.accent : Theme.panelBorder
                border.width: 1
                radius: Theme.cornerRadiusSmall
                clip: true

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingMedium
                    spacing: Theme.spacingSmall

                    VectorIcon {
                        name: "search"
                        width: 12
                        height: 12
                        color: Theme.textMuted
                    }

                    TextInput {
                        id: filterInput
                        Layout.fillWidth: true
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSmall
                        selectByMouse: true

                        Text {
                            text: "Type to search actions, categories, or keys..."
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeSmall
                            visible: !filterInput.text && !filterInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: Theme.surfaceElevated
                        visible: filterInput.text.length > 0

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "clear"
                            width: 8
                            height: 8
                            color: Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: filterInput.text = ""
                        }
                    }
                }
            }

            // Category filter pills
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Repeater {
                    model: ["All", "Playback Transport", "File & Library", "Edit & Selection", "View & Navigation", "Tools & Help"]
                    delegate: Rectangle {
                        property bool isSelected: root.selectedCategory === modelData
                        height: 24
                        width: catText.implicitWidth + 16
                        radius: Theme.cornerRadiusSmall
                        color: isSelected ? Theme.selection : (catMouse.containsMouse ? Theme.surfaceElevated : Theme.background)
                        border.color: isSelected ? Theme.accent : Theme.panelBorder
                        border.width: 1

                        Text {
                            id: catText
                            anchors.centerIn: parent
                            text: modelData === "Playback Transport" ? "Playback" : (modelData === "File & Library" ? "File" : (modelData === "Edit & Selection" ? "Edit" : (modelData === "View & Navigation" ? "View" : (modelData === "Tools & Help" ? "Tools" : modelData))))
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: isSelected
                            color: isSelected ? Theme.accentHover : Theme.textSecondary
                        }

                        MouseArea {
                            id: catMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedCategory = modelData
                        }
                    }
                }
            }
        }

        // Shortcuts List
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.panelPadding
            Layout.rightMargin: Theme.panelPadding
            Layout.bottomMargin: Theme.panelPadding
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
            clip: true

            ListView {
                id: shortcutsList
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: bridge.hotkeysModel
                ScrollBar.vertical: ScrollBar { }

                delegate: Rectangle {
                    width: shortcutsList.width - 8
                    height: visible ? 34 : 0
                    radius: Theme.cornerRadiusSmall
                    color: index % 2 === 0 ? "transparent" : Theme.surfaceElevated
                    visible: {
                        var q = filterInput.text.trim().toLowerCase();
                        var catMatch = (root.selectedCategory === "All") || (modelData.category === root.selectedCategory);
                        if (!catMatch) return false;
                        if (!q) return true;
                        return modelData.actionName.toLowerCase().indexOf(q) !== -1 ||
                               modelData.category.toLowerCase().indexOf(q) !== -1 ||
                               modelData.currentSequence.toLowerCase().indexOf(q) !== -1;
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        spacing: Theme.spacingMedium

                        // Category chip
                        Rectangle {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 18
                            radius: 3
                            color: Theme.selection
                            border.color: Theme.accent
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: modelData.category
                                font.pixelSize: 9
                                font.bold: true
                                color: Theme.accentHover
                                elide: Text.ElideRight
                            }
                        }

                        // Action Name
                        Text {
                            text: modelData.actionName
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textPrimary
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        // Shortcut Key Sequence Pill
                        Rectangle {
                            Layout.preferredHeight: 20
                            Layout.preferredWidth: Math.max(30, keyText.implicitWidth + 12)
                            radius: 3
                            color: Theme.surface
                            border.color: Theme.panelBorder
                            border.width: 1

                            Text {
                                id: keyText
                                anchors.centerIn: parent
                                text: modelData.currentSequence.length > 0 ? modelData.currentSequence : "None"
                                font.pixelSize: 10
                                font.bold: true
                                font.family: Theme.fontFamily
                                color: modelData.currentSequence.length > 0 ? Theme.accent : Theme.textMuted
                            }
                        }
                    }
                }
            }
        }

        // Footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                Text {
                    text: "To customize keyboard shortcuts, visit Preferences > Hotkeys"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textMuted
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Button {
                    text: "Close"
                    implicitHeight: 26
                    implicitWidth: 80
                    onClicked: root.close()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 520
        minHeight: 400
    }
}
}
