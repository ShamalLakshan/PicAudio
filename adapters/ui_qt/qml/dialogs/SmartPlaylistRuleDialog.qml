import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 720)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 540)) - height) / 2)
    width: Math.min(740, Overlay.overlay ? Math.max(600, Overlay.overlay.width - 48) : 740)
    height: Math.min(560, Overlay.overlay ? Math.max(460, Overlay.overlay.height - 48) : 560)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string playlistName: ""
    property bool isEditing: playlistName.length > 0
    property var rulesList: []
    property bool matchAll: true

    signal saved()

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 720);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 540);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        if (playlistName.length > 0) {
            nameInput.text = playlistName;
            var jsonStr = bridge.getSmartPlaylistRules(playlistName);
            try {
                var parsed = JSON.parse(jsonStr);
                if (Array.isArray(parsed)) {
                    rulesList = parsed;
                } else if (parsed && parsed.rules && Array.isArray(parsed.rules)) {
                    rulesList = parsed.rules;
                    matchAll = (parsed.matchAll !== false);
                } else {
                    rulesList = [{ field: "genre", operator: "contains", value: "" }];
                }
            } catch (e) {
                rulesList = [{ field: "genre", operator: "contains", value: jsonStr || "" }];
            }
        } else {
            nameInput.text = "New Smart Playlist";
            rulesList = [{ field: "rating", operator: ">=", value: "4" }];
            matchAll = true;
        }
        rulesModel.clear();
        for (var i = 0; i < rulesList.length; ++i) {
            rulesModel.append(rulesList[i]);
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    ListModel {
        id: rulesModel
    }

    readonly property var availableFields: [
        { key: "title", label: "Title" },
        { key: "artist", label: "Artist" },
        { key: "album", label: "Album" },
        { key: "albumArtist", label: "Album Artist" },
        { key: "genre", label: "Genre" },
        { key: "year", label: "Year" },
        { key: "rating", label: "Rating (Stars 1-5)" },
        { key: "isFavorite", label: "Favorite" },
        { key: "playCount", label: "Play Count" },
        { key: "skipCount", label: "Skip Count" },
        { key: "bitDepth", label: "Bit Depth (bits)" },
        { key: "sampleRate", label: "Sample Rate (Hz)" },
        { key: "codec", label: "Format / Codec" }
    ]

    readonly property var availableOperators: [
        { key: "contains", label: "contains" },
        { key: "doesNotContain", label: "does not contain" },
        { key: "equals", label: "equals" },
        { key: "notEquals", label: "does not equal" },
        { key: "greaterThan", label: "is greater than (>)" },
        { key: "greaterThanOrEquals", label: "is at least (>=)" },
        { key: "lessThan", label: "is less than (<)" },
        { key: "lessThanOrEquals", label: "is at most (<=)" },
        { key: "isTrue", label: "is true (yes)" },
        { key: "isFalse", label: "is false (no)" }
    ]

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
                        name: "playlist"
                        width: 14
                        height: 14
                        color: Theme.accent
                    }

                    Text {
                        text: root.isEditing ? ("Edit Smart Playlist: " + root.playlistName) : "Create Smart Auto-Playlist"
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

            // Top Options Area
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // Playlist Name Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text { text: "Playlist Name:"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            color: Theme.surface
                            border.color: nameInput.activeFocus ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                            clip: true

                            TextInput {
                                id: nameInput
                                anchors.fill: parent
                                anchors.margins: 6
                                verticalAlignment: TextInput.AlignVCenter
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBase
                                selectByMouse: true
                            }
                        }
                    }

                    // Matching Mode Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text { text: "Match Criteria:"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textSecondary }

                        Rectangle {
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 26
                            radius: Theme.cornerRadiusSmall
                            color: root.matchAll ? Theme.selection : Theme.surface
                            border.color: root.matchAll ? Theme.accent : Theme.panelBorder

                            Text { anchors.centerIn: parent; text: "Match ALL Rules (AND)"; font.pixelSize: 10; font.bold: root.matchAll; color: root.matchAll ? Theme.textPrimary : Theme.textMuted }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.matchAll = true }
                        }

                        Rectangle {
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 26
                            radius: Theme.cornerRadiusSmall
                            color: !root.matchAll ? Theme.selection : Theme.surface
                            border.color: !root.matchAll ? Theme.accent : Theme.panelBorder

                            Text { anchors.centerIn: parent; text: "Match ANY Rule (OR)"; font.pixelSize: 10; font.bold: !root.matchAll; color: !root.matchAll ? Theme.textPrimary : Theme.textMuted }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.matchAll = false }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // Rules List
            ScrollView {
                id: rulesScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ListView {
                    id: rulesListView
                    width: rulesScrollView.availableWidth
                    model: rulesModel
                    spacing: 6

                    delegate: Rectangle {
                        width: rulesListView.width - 20
                        height: 42
                        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                        color: Theme.surfaceElevated
                        border.color: Theme.panelBorder
                        radius: Theme.cornerRadiusSmall

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8

                            // Field ComboBox
                            StyledComboBox {
                                Layout.preferredWidth: 150
                                model: root.availableFields.map(function(f) { return f.label; })
                                currentIndex: {
                                    for (var f = 0; f < root.availableFields.length; ++f) {
                                        if (root.availableFields[f].key === model.field) return f;
                                    }
                                    return 0;
                                }
                                onActivated: function(idx) {
                                    rulesModel.setProperty(index, "field", root.availableFields[idx].key);
                                }
                            }

                            // Operator ComboBox
                            StyledComboBox {
                                Layout.preferredWidth: 140
                                model: root.availableOperators.map(function(o) { return o.label; })
                                currentIndex: {
                                    var op = ("" + model.operator).trim();
                                    var norm = op;
                                    if (op === ">=") norm = "greaterThanOrEquals";
                                    else if (op === "<=") norm = "lessThanOrEquals";
                                    else if (op === ">") norm = "greaterThan";
                                    else if (op === "<") norm = "lessThan";
                                    else if (op === "=" || op === "==" || op === "is") norm = "equals";
                                    else if (op === "!=" || op === "isNot") norm = "notEquals";
                                    else if (op === "like") norm = "contains";
                                    else if (op === "notLike") norm = "doesNotContain";
                                    for (var o = 0; o < root.availableOperators.length; ++o) {
                                        if (root.availableOperators[o].key === norm || root.availableOperators[o].key === op) return o;
                                    }
                                    return 0;
                                }
                                onActivated: function(idx) {
                                    rulesModel.setProperty(index, "operator", root.availableOperators[idx].key);
                                }
                            }

                            // Value input
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                color: Theme.background
                                border.color: valInput.activeFocus ? Theme.accent : Theme.panelBorder
                                border.width: 1
                                radius: Theme.cornerRadiusSmall
                                clip: true

                                TextInput {
                                    id: valInput
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    text: model.value || ""
                                    selectByMouse: true
                                    onTextChanged: {
                                        rulesModel.setProperty(index, "value", text);
                                    }
                                }
                            }

                            // Delete button
                            Rectangle {
                                Layout.preferredWidth: 26
                                Layout.preferredHeight: 26
                                radius: Theme.cornerRadiusSmall
                                color: delRuleMouse.containsMouse ? Theme.selection : "transparent"
                                border.color: Theme.panelBorder

                                VectorIcon {
                                    anchors.centerIn: parent
                                    name: "clear"
                                    width: 10
                                    height: 10
                                    color: Theme.accent
                                }
                                MouseArea {
                                    id: delRuleMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (rulesModel.count > 1) {
                                            rulesModel.remove(index);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Footer
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: 36 // Safe margin for ResizeGrip
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: addRuleText.implicitWidth + 24
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: addRuleMouse.containsMouse ? Theme.selection : Theme.surface
                        border.color: Theme.panelBorder

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            VectorIcon { name: "plus"; width: 10; height: 10; color: Theme.accent }
                            Text { id: addRuleText; text: "Add Condition"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textPrimary }
                        }
                        MouseArea {
                            id: addRuleMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                rulesModel.append({ field: "artist", operator: "contains", value: "" });
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: cancelMouse.containsMouse ? Theme.surface : "transparent"
                        border.color: Theme.panelBorder
                        Text { anchors.centerIn: parent; text: "Cancel"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                        MouseArea {
                            id: cancelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 30
                        radius: Theme.cornerRadiusSmall
                        color: saveMouse.containsMouse ? Theme.accentHover : Theme.accent
                        Text { anchors.centerIn: parent; text: "Save Playlist"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary }
                        MouseArea {
                            id: saveMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var name = nameInput.text.trim();
                                if (name.length === 0) return;

                                var rulesArr = [];
                                for (var i = 0; i < rulesModel.count; ++i) {
                                    rulesArr.push({
                                        field: rulesModel.get(i).field,
                                        operator: rulesModel.get(i).operator,
                                        value: rulesModel.get(i).value
                                    });
                                }

                                var obj = {
                                    name: name,
                                    matchAll: root.matchAll,
                                    rules: rulesArr,
                                    orderBy: "title ASC",
                                    limit: 0
                                };
                                bridge.createSmartPlaylist(name, JSON.stringify(obj));
                                root.saved();
                                root.close();
                            }
                        }
                    }
                }
            }
        }
    }
}
