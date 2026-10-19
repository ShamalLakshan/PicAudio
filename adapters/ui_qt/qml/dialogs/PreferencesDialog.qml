import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Dialog {
    id: root
    title: "Preferences & Settings"
    width: Math.min(840, Overlay.overlay ? Overlay.overlay.width - 32 : 840)
    height: Math.min(620, Overlay.overlay ? Overlay.overlay.height - 32 : 620)
    modal: true
    anchors.centerIn: Overlay.overlay

    property int activeCategory: 1 // default to Themes & Appearance

    component StyledComboBox: ComboBox {
        id: cbRoot
        Layout.preferredWidth: 220
        Layout.preferredHeight: 28
        background: Rectangle {
            implicitWidth: 220
            implicitHeight: 28
            color: Theme.surfaceElevated
            border.color: cbRoot.activeFocus ? Theme.accent : Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
        contentItem: Text {
            leftPadding: Theme.spacingMedium
            rightPadding: cbRoot.indicator.width + Theme.spacingMedium
            text: cbRoot.displayText
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            color: Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        indicator: VectorIcon {
            x: cbRoot.width - width - Theme.spacingMedium
            y: (cbRoot.height - height) / 2
            width: 10
            height: 10
            name: "arrow_down"
            color: Theme.textSecondary
        }
        popup: Popup {
            y: cbRoot.height + 2
            width: cbRoot.width
            implicitHeight: Math.min(220, contentItem.implicitHeight + 8)
            padding: 4
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: cbRoot.popup.visible ? cbRoot.delegateModel : null
                currentIndex: cbRoot.highlightedIndex
                ScrollIndicator.vertical: ScrollIndicator { }
            }
            background: Rectangle {
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1
                radius: Theme.cornerRadiusSmall
            }
        }
        delegate: ItemDelegate {
            width: cbRoot.width - 8
            height: 26
            highlighted: cbRoot.highlightedIndex === index
            contentItem: Text {
                text: modelData
                color: highlighted ? Theme.accentHover : Theme.textPrimary
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: highlighted ? Theme.selection : "transparent"
                radius: Theme.cornerRadiusSmall
            }
        }
    }

    component StyledCheckBox: CheckBox {
        id: cbControl
        implicitHeight: 28
        indicator: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            x: cbControl.leftPadding
            y: parent.height / 2 - height / 2
            radius: Theme.cornerRadiusSmall
            color: cbControl.checked ? Theme.accent : Theme.surfaceElevated
            border.color: cbControl.checked ? Theme.accent : Theme.panelBorder
            border.width: 1

            VectorIcon {
                anchors.centerIn: parent
                width: 10
                height: 10
                name: "check"
                color: Theme.background
                visible: cbControl.checked
            }
        }
        contentItem: Text {
            text: cbControl.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            color: Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            leftPadding: cbControl.indicator.width + Theme.spacingMedium
            elide: Text.ElideRight
        }
    }

    component StyledSlider: Slider {
        id: sldControl
        implicitHeight: 28
        background: Rectangle {
            x: sldControl.leftPadding
            y: sldControl.topPadding + sldControl.availableHeight / 2 - 2
            implicitWidth: 160
            implicitHeight: 4
            width: sldControl.availableWidth
            height: 4
            radius: 2
            color: Theme.surfaceElevated

            Rectangle {
                width: sldControl.visualPosition * parent.width
                height: parent.height
                color: Theme.accent
                radius: 2
            }
        }
        handle: Rectangle {
            x: sldControl.leftPadding + sldControl.visualPosition * (sldControl.availableWidth - width)
            y: sldControl.topPadding + sldControl.availableHeight / 2 - height / 2
            implicitWidth: 14
            implicitHeight: 14
            radius: 7
            color: sldControl.pressed ? Theme.textPrimary : Theme.accentHover
            border.color: Theme.panelBorder
            border.width: 1
        }
    }

    component StyledButton: Button {
        id: btnControl
        implicitHeight: 28
        padding: Theme.spacingMedium
        contentItem: Text {
            text: btnControl.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.bold: btnControl.highlighted
            color: btnControl.highlighted ? Theme.accentHover : Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: btnControl.highlighted ? Theme.selection : (btnControl.hovered ? Theme.surfaceElevated : Theme.background)
            border.color: btnControl.highlighted ? Theme.accent : Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
    }

    component StyledTextField: TextField {
        id: tfControl
        implicitHeight: 28
        leftPadding: Theme.spacingMedium
        rightPadding: Theme.spacingMedium
        font.pixelSize: Theme.fontSizeSmall
        font.family: Theme.fontFamily
        color: Theme.textPrimary
        selectionColor: Theme.selection
        selectedTextColor: Theme.accentHover
        background: Rectangle {
            color: Theme.surfaceElevated
            border.color: tfControl.activeFocus ? Theme.accent : Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
    }

    FileDialog {
        id: themeFileDialog
        title: "Select Theme File (.json)"
        nameFilters: ["JSON Theme files (*.json)"]
        onAccepted: {
            bridge.theme.installTheme(selectedFile.toString());
        }
    }

    FileDialog {
        id: exportBackupDialog
        title: "Export SQLite Database Backup"
        fileMode: FileDialog.SaveFile
        nameFilters: ["SQLite Database (*.db)", "All Files (*)"]
        onAccepted: {
            bridge.exportDatabaseBackup(selectedFile.toString());
        }
    }

    FileDialog {
        id: exportDiagnosticsDialog
        title: "Export Diagnostics Report"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Text Report (*.txt)", "All Files (*)"]
        onAccepted: {
            bridge.exportDiagnosticsReport(selectedFile.toString());
        }
    }

    FolderDialog {
        id: addFolderDialog
        title: "Add Folder to Monitored Music Folders"
        onAccepted: {
            bridge.addMonitoredFolder(selectedFolder.toString());
        }
    }

    Dialog {
        id: confirmClearDbDialog
        title: "Clear Entire Music Library"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(420, Overlay.overlay ? Overlay.overlay.width - 32 : 420)
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusMedium
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text {
                text: "Are you sure you want to clear the entire music database?"
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
                color: Theme.textPrimary
            }
            Text {
                text: "All indexed track records and metadata will be permanently purged from the local database. Your actual audio files on disk will NOT be touched or modified."
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            bridge.clearLibrary();
        }
    }

    Dialog {
        id: resetFactoryDefaultsDialog
        title: "Reset All Preferences to Defaults"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(420, Overlay.overlay ? Overlay.overlay.width - 32 : 420)
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusMedium
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text {
                text: "Are you sure you want to reset all settings?"
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
                color: Theme.textPrimary
            }
            Text {
                text: "All application preferences, appearance themes, audio configurations, hotkeys, and scrobbling credentials will be reset to factory defaults."
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            bridge.resetAllSettingsToDefaults();
        }
    }

    FileDialog {
        id: importKeymapDialog
        title: "Import Keymap JSON"
        nameFilters: ["Keymap JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (bridge.importHotkeys(selectedFile.toString())) {
                bridge.showToast("Keymap imported successfully", "success");
            } else {
                bridge.showToast("Failed to import keymap JSON", "error");
            }
        }
    }

    FileDialog {
        id: exportKeymapDialog
        title: "Export Keymap JSON"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Keymap JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (bridge.exportHotkeys(selectedFile.toString())) {
                bridge.showToast("Keymap exported successfully", "success");
            } else {
                bridge.showToast("Failed to export keymap JSON", "error");
            }
        }
    }

    property string editingActionId: ""
    property string editingActionName: ""
    property string editingKeySequence: ""
    property string hotkeyConflictText: ""
    property string dbIntegrityStatus: ""

    Dialog {
        id: rebindHotkeyDialog
        title: "Rebind Shortcut"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(440, Overlay.overlay ? Overlay.overlay.width - 32 : 440)
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusMedium
        }
        contentItem: ColumnLayout {
            spacing: 12

            Text {
                text: "Rebinding action: " + root.editingActionName
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
                color: Theme.textPrimary
            }

            Text {
                text: "Press any key combination to capture new shortcut:"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: Theme.surfaceElevated
                border.color: recorderInput.activeFocus ? Theme.accent : Theme.panelBorder
                border.width: 1
                radius: Theme.cornerRadiusSmall

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    Text {
                        text: root.editingKeySequence !== "" ? root.editingKeySequence : "Press shortcut keys..."
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: root.editingKeySequence !== "" ? Theme.accentHover : Theme.textMuted
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    StyledButton {
                        text: "Clear"
                        implicitHeight: 24
                        onClicked: {
                            root.editingKeySequence = ""
                            root.hotkeyConflictText = ""
                        }
                    }
                }

                Item {
                    id: recorderInput
                    anchors.fill: parent
                    focus: true
                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Control || event.key === Qt.Key_Shift ||
                            event.key === Qt.Key_Alt || event.key === Qt.Key_Meta) {
                            return;
                        }
                        var parts = []
                        if (event.modifiers & Qt.ControlModifier) parts.push("Ctrl")
                        if (event.modifiers & Qt.ShiftModifier) parts.push("Shift")
                        if (event.modifiers & Qt.AltModifier) parts.push("Alt")
                        if (event.modifiers & Qt.MetaModifier) parts.push("Meta")

                        var keyText = ""
                        if (event.key === Qt.Key_Space) keyText = "Space"
                        else if (event.key === Qt.Key_Escape) keyText = "Escape"
                        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) keyText = "Return"
                        else if (event.key === Qt.Key_Left) keyText = "Left"
                        else if (event.key === Qt.Key_Right) keyText = "Right"
                        else if (event.key === Qt.Key_Up) keyText = "Up"
                        else if (event.key === Qt.Key_Down) keyText = "Down"
                        else if (event.key === Qt.Key_Delete) keyText = "Del"
                        else if (event.key >= Qt.Key_F1 && event.key <= Qt.Key_F12) keyText = "F" + (event.key - Qt.Key_F1 + 1)
                        else if (event.text && event.text.length > 0 && event.text.charCodeAt(0) >= 32 && event.text.charCodeAt(0) <= 126) {
                            keyText = event.text.toUpperCase()
                        } else {
                            keyText = String.fromCharCode(event.key)
                        }

                        if (keyText) {
                            parts.push(keyText)
                            var seq = parts.join("+")
                            root.editingKeySequence = seq
                            var conflict = bridge.checkHotkeyConflict(root.editingActionId, seq)
                            if (conflict !== "") {
                                root.hotkeyConflictText = "⚠️ Conflict: Already mapped to \"" + conflict + "\""
                            } else {
                                root.hotkeyConflictText = ""
                            }
                        }
                        event.accepted = true
                    }
                }
            }

            Text {
                text: root.hotkeyConflictText
                visible: root.hotkeyConflictText !== ""
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.accentHover
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            bridge.setHotkey(root.editingActionId, root.editingKeySequence);
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    header: Rectangle {
        height: 38
        color: Theme.surfaceElevated
        border.color: Theme.panelBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Text {
                text: "PREFERENCES & SETTINGS"
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                font.letterSpacing: 1.0
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 22
                height: 22
                radius: 3
                color: closeBtnMouse.containsMouse ? Theme.selection : "transparent"
                VectorIcon {
                    anchors.centerIn: parent
                    name: "close"
                    width: 10
                    height: 10
                    color: Theme.textSecondary
                }
                MouseArea {
                    id: closeBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }
    }

    contentItem: RowLayout {
        spacing: 0

        // Categories
        Rectangle {
            Layout.preferredWidth: 190
            Layout.fillHeight: true
            color: Theme.background
            border.color: Theme.panelBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 8
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 2

                Repeater {
                    model: [
                        { idx: 0, label: "General", icon: "music" },
                        { idx: 1, label: "Themes & Appearance", icon: "grid" },
                        { idx: 2, label: "Audio Output", icon: "volume" },
                        { idx: 3, label: "Playback & DSP", icon: "equalizer" },
                        { idx: 4, label: "Queue Ergonomics", icon: "queue" },
                        { idx: 5, label: "Library & Folders", icon: "folder" },
                        { idx: 6, label: "Metadata & Scrobbling", icon: "settings" },
                        { idx: 7, label: "Keyboard Shortcuts", icon: "info" },
                        { idx: 8, label: "Diagnostics & Advanced", icon: "info" },
                        { idx: 9, label: "Plugins & Extensions", icon: "equalizer" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        radius: Theme.cornerRadiusSmall
                        color: activeCategory === modelData.idx ? Theme.selection : (catMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            VectorIcon {
                                name: modelData.icon
                                width: 12
                                height: 12
                                color: activeCategory === modelData.idx ? Theme.accent : Theme.textMuted
                            }

                            Text {
                                text: modelData.label
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: activeCategory === modelData.idx
                                color: activeCategory === modelData.idx ? Theme.textPrimary : Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            id: catMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: activeCategory = modelData.idx
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Settings content
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface

            // General Settings
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 0
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "General Settings"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Configure interface localization, application startup behavior, window closing actions, and desktop alerts."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Language
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Interface Language"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Select application display language for menus, tooltips, and dialogs."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: languageCombo
                            model: ["System Default", "English", "Deutsch", "Français", "Español", "日本語"]
                            currentIndex: Math.max(0, model.indexOf(bridge.language))
                            onActivated: function(index) {
                                bridge.setLanguage(model[index])
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Startup Action
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Startup Action"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Behavior when launching PicAudio."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: startupCombo
                            model: ["Restore Session", "Open Empty", "Play Immediately"]
                            currentIndex: Math.max(0, model.indexOf(bridge.startupAction))
                            onActivated: function(index) {
                                bridge.setStartupAction(model[index])
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Window Lifecycle: Close Action & Minimize Action
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Window Close Action"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Choose what happens when the main window close button is pressed."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: closeActionCombo
                            model: ["Exit Application", "Minimize to System Tray"]
                            currentIndex: Math.max(0, model.indexOf(bridge.closeAction))
                            onActivated: function(index) {
                                bridge.setCloseAction(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Window Minimize Action"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Choose where the application goes when minimized."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: minimizeActionCombo
                            model: ["Minimize to Taskbar", "Minimize to System Tray"]
                            currentIndex: Math.max(0, model.indexOf(bridge.minimizeAction))
                            onActivated: function(index) {
                                bridge.setMinimizeAction(model[index])
                            }
                        }
                    }

                    // Single-Instance
                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Allow only a single instance of PicAudio (forward tracks to running instance)"
                        checked: bridge.singleInstance
                        onToggled: bridge.setSingleInstance(checked)
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Software Updates
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Software Updates"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Check for new releases and security patches."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            StyledComboBox {
                                id: updateIntervalCombo
                                Layout.preferredWidth: 140
                                model: ["Never", "On Startup", "Daily", "Weekly", "Monthly"]
                                currentIndex: Math.max(0, model.indexOf(bridge.updateCheckInterval))
                                onActivated: function(index) {
                                    bridge.setUpdateCheckInterval(model[index])
                                }
                            }
                            StyledButton {
                                text: "Check Now"
                                onClicked: bridge.checkForUpdates()
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Notifications Section
                    Text {
                        text: "Desktop Notifications"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Show native desktop notifications on track changes"
                        checked: bridge.showNotifications
                        onToggled: bridge.setShowNotifications(checked)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        opacity: bridge.showNotifications ? 1.0 : 0.4
                        enabled: bridge.showNotifications

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Notification Duration"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "How long toast alerts remain visible before fading out."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: notifDurationSlider
                                Layout.preferredWidth: 140
                                from: 1
                                to: 10
                                stepSize: 1
                                value: bridge.notificationDurationSec
                                onMoved: bridge.setNotificationDurationSec(Math.round(value))
                            }
                            Text {
                                text: Math.round(notifDurationSlider.value) + "s"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 30
                            }
                        }
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        opacity: bridge.showNotifications ? 1.0 : 0.4
                        enabled: bridge.showNotifications
                        text: "Suppress notifications when PicAudio is the focused foreground window"
                        checked: bridge.suppressNotificationsWhenFocused
                        onToggled: bridge.setSuppressNotificationsWhenFocused(checked)
                    }
                }
            }

            // Themes & Appearance
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 1
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Installed Themes"
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }

                        StyledButton {
                            text: "Install Theme from File..."
                            onClicked: themeFileDialog.open()
                        }

                        StyledButton {
                            text: "Open Themes Folder"
                            onClicked: bridge.theme.openThemesFolder()
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Theme Cards Grid
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 12
                        columnSpacing: 12

                        Repeater {
                            model: bridge.theme.availableThemes

                                delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 110
                                radius: Theme.cornerRadiusSmall
                                color: bridge.theme.themeId === modelData.id ? Theme.selection : (themeCardMouse.containsMouse ? Theme.surfaceElevated : Theme.background)
                                border.color: bridge.theme.themeId === modelData.id ? Theme.accent : Theme.panelBorder
                                border.width: bridge.theme.themeId === modelData.id ? 2 : 1

                                MouseArea {
                                    id: themeCardMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        bridge.theme.themeId = modelData.id;
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.name
                                            font.pixelSize: Theme.fontSizeBase
                                            font.bold: true
                                            color: Theme.textPrimary
                                        }
                                        Rectangle {
                                            visible: !modelData.isBuiltIn
                                            color: Theme.surfaceElevated
                                            border.color: Theme.accent
                                            border.width: 1
                                            radius: 2
                                            implicitWidth: communityLabel.implicitWidth + 8
                                            implicitHeight: communityLabel.implicitHeight + 4
                                            Text {
                                                id: communityLabel
                                                anchors.centerIn: parent
                                                text: "COMMUNITY"
                                                font.pixelSize: 8
                                                font.bold: true
                                                color: Theme.accent
                                            }
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: "v" + modelData.version
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            color: Theme.textMuted
                                        }
                                        Rectangle {
                                            visible: !modelData.isBuiltIn && bridge.theme.themeId !== modelData.id
                                            width: 18
                                            height: 18
                                            radius: 2
                                            color: deleteMouse.containsMouse ? Theme.error : "transparent"
                                            Text {
                                                anchors.centerIn: parent
                                                text: "×"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: deleteMouse.containsMouse ? "#ffffff" : Theme.textMuted
                                            }
                                            MouseArea {
                                                id: deleteMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    bridge.theme.deleteTheme(modelData.id);
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        text: "By " + modelData.author
                                        font.pixelSize: Theme.fontSizeSmall
                                        color: Theme.textSecondary
                                    }

                                    Text {
                                        text: modelData.description
                                        font.pixelSize: Theme.fontSizeSmall - 1
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Item { Layout.fillHeight: true }

                                    // Palette Swatches
                                    RowLayout {
                                        spacing: 6
                                        Repeater {
                                            model: modelData.colors ? [
                                                modelData.colors.background,
                                                modelData.colors.surface,
                                                modelData.colors.surfaceElevated,
                                                modelData.colors.accent,
                                                modelData.colors.textPrimary
                                            ] : []

                                            delegate: Rectangle {
                                                width: 18
                                                height: 14
                                                radius: 2
                                                color: modelData || Theme.surfaceElevated
                                                border.color: Theme.panelBorder
                                                border.width: 1
                                            }
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: bridge.theme.themeId === modelData.id ? "ACTIVE" : "CLICK TO ACTIVATE"
                                            font.pixelSize: 8
                                            font.bold: true
                                            color: bridge.theme.themeId === modelData.id ? Theme.accent : Theme.textMuted
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Display Density
                    Text {
                        text: "Display Density & Metrics Scale"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        spacing: 12
                        Repeater {
                            model: ["Compact", "Standard", "Comfortable"]
                            delegate: StyledButton {
                                text: modelData
                                highlighted: bridge.theme.densityPreset === modelData
                                onClicked: bridge.theme.densityPreset = modelData
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Typography & Font Settings
                    Text {
                        text: "Typography & Font Customization"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Font Family"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Global typeface family for the UI."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: fontCombo
                            model: ["Inter, sans-serif", "Segoe UI, sans-serif", "SF Pro, sans-serif", "Roboto, sans-serif", "System Monospace"]
                            currentIndex: Math.max(0, model.indexOf(bridge.fontFamily))
                            onActivated: function(index) {
                                bridge.setFontFamily(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Base Font Size"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Controls baseline font scaling throughout the interface."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: baseFontSizeSlider
                                Layout.preferredWidth: 140
                                from: 9
                                to: 16
                                stepSize: 1
                                value: bridge.baseFontSize
                                onMoved: bridge.setBaseFontSize(Math.round(value))
                            }
                            Text {
                                text: Math.round(baseFontSizeSlider.value) + " pt"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 40
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Track Table & UI Layout
                    Text {
                        text: "Table & Interface Layout"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Track Table Row Height"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Compact spacing for large libraries or taller rows for touch ergonomics."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: rowHeightSlider
                                Layout.preferredWidth: 140
                                from: 24
                                to: 48
                                stepSize: 2
                                value: bridge.tableRowHeight
                                onMoved: bridge.setTableRowHeight(Math.round(value))
                            }
                            Text {
                                text: Math.round(rowHeightSlider.value) + " px"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 40
                            }
                        }
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Enable alternating row background zebra striping in track tables"
                        checked: bridge.tableAlternatingRows
                        onToggled: bridge.setTableAlternatingRows(checked)
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Display high-density audio waveform seekbar in player deck"
                        checked: bridge.waveformSeekbar
                        onToggled: bridge.setWaveformSeekbar(checked)
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Display bottom status bar (track count, audio specs, scanning state)"
                        checked: bridge.showStatusBar
                        onToggled: bridge.setShowStatusBar(checked)
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Cover Art & Cache
                    Text {
                        text: "Artwork & Thumbnail Cache"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Thumbnail Scaling Quality"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Trade off image rendering speed against sharpness."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: artQualityCombo
                            model: ["Fast (Nearest)", "Balanced (Bilinear)", "Smooth (High Quality)"]
                            currentIndex: Math.max(0, model.indexOf(bridge.artThumbnailQuality))
                            onActivated: function(index) {
                                bridge.setArtThumbnailQuality(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Disk Cache Size Limit"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Maximum local storage dedicated to cached album covers."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: artCacheSlider
                                Layout.preferredWidth: 140
                                from: 128
                                to: 4096
                                stepSize: 128
                                value: bridge.artCacheLimitMb
                                onMoved: bridge.setArtCacheLimitMb(Math.round(value))
                            }
                            Text {
                                text: Math.round(artCacheSlider.value) + " MB"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 50
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        StyledButton {
                            text: "Clear Artwork Disk Cache"
                            onClicked: bridge.clearArtCache()
                        }
                    }
                }
            }

            // Audio Output & Hardware Pipeline
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 2
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Audio Hardware & Output Pipeline"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Configure audio output drivers, hardware exclusivity, buffers, and bit-perfect playback pipeline."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Hardware Status Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 74
                        radius: Theme.cornerRadiusSmall
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 14

                            Rectangle {
                                width: 36
                                height: 36
                                radius: Theme.cornerRadiusSmall
                                color: Theme.selection
                                border.color: Theme.accent
                                border.width: 1

                                VectorIcon {
                                    anchors.centerIn: parent
                                    name: "volume"
                                    width: 18
                                    height: 18
                                    color: Theme.accentHover
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                RowLayout {
                                    spacing: 8
                                    Text {
                                        text: "Audio Backend:"
                                        font.pixelSize: Theme.fontSizeSmall
                                        font.bold: true
                                        color: Theme.textPrimary
                                    }
                                    Text {
                                        text: bridge.audioBackend
                                        font.pixelSize: Theme.fontSizeSmall
                                        font.bold: true
                                        color: Theme.accentHover
                                    }
                                    Text {
                                        text: "(" + (bridge.bitPerfectExclusive ? "Exclusive" : "Shared") + ")"
                                        font.pixelSize: Theme.fontSizeSmall - 1
                                        color: bridge.bitPerfectExclusive ? Theme.accentHover : Theme.textMuted
                                    }
                                }

                                Text {
                                    text: (bridge.currentAudioSpecs.length > 0 ? bridge.currentAudioSpecs + " • " : "") + "Device: " + bridge.currentAudioDevice
                                    font.pixelSize: Theme.fontSizeSmall - 1
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Audio Backend
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Audio Output Backend"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Select low-level audio driver interface."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: backendCombo
                            Layout.preferredWidth: 260
                            model: ["Linux PipeWire Lock-Free Client", "ALSA Direct Hardware (hw:)", "PulseAudio Legacy Client"]
                            currentIndex: Math.max(0, model.indexOf(bridge.audioBackend))
                            onActivated: function(index) {
                                bridge.setAudioBackend(model[index])
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Output Device
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Output Device"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Select physical sound card, DAC, or virtual output sink."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            StyledComboBox {
                                id: audioDeviceCombo
                                Layout.preferredWidth: 220
                                model: bridge.availableAudioDevices
                                currentIndex: Math.max(0, model.indexOf(bridge.currentAudioDevice))
                                onActivated: function(index) {
                                    bridge.setCurrentAudioDevice(model[index])
                                }
                            }
                            StyledButton {
                                text: "Refresh"
                                onClicked: bridge.refreshAudioDevices()
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Bit-Perfect Mode
                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Bit-perfect integer routing (exclusive mode, bypass OS mixer & resampling)"
                        checked: bridge.bitPerfectExclusive
                        onToggled: bridge.setBitPerfectExclusive(checked)
                    }

                    // Buffer Latency
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Hardware Buffer Latency"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Buffer duration in milliseconds. Lower values decrease seek delay; higher values prevent underruns."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: latencySlider
                                Layout.preferredWidth: 140
                                from: 10
                                to: 500
                                stepSize: 5
                                value: bridge.bufferLatencyMs
                                onMoved: bridge.setBufferLatencyMs(Math.round(value))
                            }
                            Text {
                                text: Math.round(latencySlider.value) + " ms"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 45
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // DSP & Pipeline Preferences
                    Text {
                        text: "DSP & Resampling Pipeline"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Resampling Quality"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Algorithm used when source sample rate does not match DAC native rate."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: resamplerCombo
                            Layout.preferredWidth: 260
                            model: ["SoX Resampler Very High Quality (VHQ)", "SoX Resampler High Quality", "Speex Resampler", "Linear Resampler (Low Latency)"]
                            currentIndex: Math.max(0, model.indexOf(bridge.resamplerQuality))
                            onActivated: function(index) {
                                bridge.setResamplerQuality(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Dithering Algorithm"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Applied during bit-depth truncation (e.g. 24-bit to 16-bit output)."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: ditherCombo
                            Layout.preferredWidth: 260
                            model: ["None (Direct Truncation)", "Flat TPDF (Triangular)", "High-Pass TPDF", "Shibata Noise Shaping"]
                            currentIndex: Math.max(0, model.indexOf(bridge.ditherMode))
                            onActivated: function(index) {
                                bridge.setDitherMode(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Channel Processing & Downmix"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Multi-channel conversion strategy for stereo outputs."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: channelCombo
                            Layout.preferredWidth: 260
                            model: ["Stereo Passthrough", "Downmix Surround to Stereo", "Upmix Stereo to 5.1", "Mono Downmix"]
                            currentIndex: Math.max(0, model.indexOf(bridge.channelProcessing))
                            onActivated: function(index) {
                                bridge.setChannelProcessing(model[index])
                            }
                        }
                    }
                }
            }

            // Playback & DSP
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 3
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Playback Transitions & DSP"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Configure sample-accurate gapless transitions, crossfading profiles, ReplayGain loudness normalization, and seek ergonomics."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Gapless Playback & Crossfade
                    Text {
                        text: "Transitions & Fading"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Enable sample-accurate gapless playback (background pre-buffering between tracks)"
                        checked: bridge.gaplessPlayback
                        onToggled: bridge.setGaplessPlayback(checked)
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Enable smooth crossfading between consecutive tracks"
                        checked: bridge.crossfadeEnabled
                        onToggled: bridge.setCrossfadeEnabled(checked)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        opacity: bridge.crossfadeEnabled ? 1.0 : 0.4
                        enabled: bridge.crossfadeEnabled

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Crossfade Duration"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Duration of overlapping transition between consecutive tracks."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: crossfadeSlider
                                Layout.preferredWidth: 140
                                from: 0.5
                                to: 10.0
                                stepSize: 0.5
                                value: bridge.crossfadeDurationSec
                                onMoved: bridge.setCrossfadeDurationSec(value)
                            }
                            Text {
                                text: crossfadeSlider.value.toFixed(1) + " s"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 45
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        opacity: bridge.crossfadeEnabled ? 1.0 : 0.4
                        enabled: bridge.crossfadeEnabled

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Crossfade Curve Profile"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Select attenuation curve profile during volume transitions."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: crossfadeCurveCombo
                            Layout.preferredWidth: 260
                            model: ["Equal Power (Constant Volume)", "Linear Transition", "Logarithmic Fade"]
                            currentIndex: Math.max(0, model.indexOf(bridge.crossfadeCurve))
                            onActivated: function(index) {
                                bridge.setCrossfadeCurve(model[index])
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // ReplayGain & Loudness Normalization
                    Text {
                        text: "ReplayGain Loudness Normalization"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "ReplayGain Mode"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Target loudness standard (EBU R128 / ReplayGain 2.0)."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: replayGainCombo
                            Layout.preferredWidth: 260
                            model: ["Disabled", "Track Gain (Uniform Loudness)", "Album Gain (Preserves Album Dynamics)", "Smart Gain (Auto Track/Album)"]
                            currentIndex: Math.max(0, model.indexOf(bridge.replayGainMode))
                            onActivated: function(index) {
                                bridge.setReplayGainMode(model[index])
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        opacity: bridge.replayGainMode !== "Disabled" ? 1.0 : 0.4
                        enabled: bridge.replayGainMode !== "Disabled"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Tagged Track Preamp"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Volume offset applied to tracks with ReplayGain tags."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: taggedPreampSlider
                                Layout.preferredWidth: 140
                                from: -12
                                to: 12
                                stepSize: 1
                                value: bridge.replayGainPreampDb
                                onMoved: bridge.setReplayGainPreampDb(Math.round(value))
                            }
                            Text {
                                text: (taggedPreampSlider.value > 0 ? "+" : "") + Math.round(taggedPreampSlider.value) + " dB"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 55
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        opacity: bridge.replayGainMode !== "Disabled" ? 1.0 : 0.4
                        enabled: bridge.replayGainMode !== "Disabled"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Untagged Fallback Preamp"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Volume attenuation applied to tracks missing ReplayGain tags."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: untaggedPreampSlider
                                Layout.preferredWidth: 140
                                from: -12
                                to: 12
                                stepSize: 1
                                value: bridge.replayGainPreampWithoutGainDb
                                onMoved: bridge.setReplayGainPreampWithoutGainDb(Math.round(value))
                            }
                            Text {
                                text: (untaggedPreampSlider.value > 0 ? "+" : "") + Math.round(untaggedPreampSlider.value) + " dB"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 55
                            }
                        }
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Enable true-peak anti-clipping limiter (prevents inter-sample DAC clipping)"
                        checked: bridge.truePeakLimiter
                        onToggled: bridge.setTruePeakLimiter(checked)
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Seek Ergonomics & Transport
                    Text {
                        text: "Transport & Seek Steps"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Short Seek Step (Arrow Keys)"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Time skipped with Left / Right arrow keys."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: shortSeekSlider
                                Layout.preferredWidth: 140
                                from: 1
                                to: 10
                                stepSize: 1
                                value: bridge.shortSeekStepSec
                                onMoved: bridge.setShortSeekStepSec(Math.round(value))
                            }
                            Text {
                                text: Math.round(shortSeekSlider.value) + " s"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 40
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Long Seek Step (Shift + Arrow Keys)"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Time skipped with Shift + Left / Right keys."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: longSeekSlider
                                Layout.preferredWidth: 140
                                from: 15
                                to: 60
                                stepSize: 5
                                value: bridge.longSeekStepSec
                                onMoved: bridge.setLongSeekStepSec(Math.round(value))
                            }
                            Text {
                                text: Math.round(longSeekSlider.value) + " s"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 40
                            }
                        }
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Stop playback after current track finishes (Single-track playback mode)"
                        checked: bridge.stopAfterCurrentTrack
                        onToggled: bridge.setStopAfterCurrentTrack(checked)
                    }
                }
            }

            // Queue Ergonomics
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 4
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Queue & Interaction Ergonomics"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Customize mouse double-click and middle-click gestures, queue exhaustion behavior, shuffle strategies, and history retention."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Mouse Click Actions
                    Text {
                        text: "Mouse Click Actions"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Track Double-Click Action"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Action executed when double-clicking a track in the library or album view."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            Repeater {
                                model: ["Play Now", "Play Next", "Queue Last"]
                                delegate: StyledButton {
                                    text: modelData
                                    highlighted: bridge.doubleClickAction === modelData
                                    onClicked: bridge.setDoubleClickAction(modelData)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Track Middle-Click Action"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Action executed when middle-clicking a track with mouse wheel."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            Repeater {
                                model: ["Queue Last", "Play Next", "Play Now"]
                                delegate: StyledButton {
                                    text: modelData
                                    highlighted: bridge.middleClickAction === modelData
                                    onClicked: bridge.setMiddleClickAction(modelData)
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Queue Exhaustion & Autoplay
                    Text {
                        text: "Queue Exhaustion & Autoplay"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "When Queue Ends"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Behavior when all upcoming queued tracks finish playing."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            Repeater {
                                model: ["Stop Playback", "Loop Context", "Smart Autoplay (Similar Tracks)"]
                                delegate: StyledButton {
                                    text: modelData
                                    highlighted: bridge.queueAutoFillMode === modelData
                                    onClicked: bridge.setQueueAutoFillMode(modelData)
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Shuffle Mode Preference
                    Text {
                        text: "Shuffle Strategy"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Default Shuffle Mode"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Fisher-Yates track shuffle or album-by-album grouped shuffle."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 8
                            StyledButton {
                                text: "Track Shuffle (Fisher-Yates)"
                                highlighted: bridge.shuffleMode === 1
                                onClicked: bridge.setShuffleMode(1)
                            }
                            StyledButton {
                                text: "Album Shuffle (Keep Album Tracks)"
                                highlighted: bridge.shuffleMode === 2
                                onClicked: bridge.setShuffleMode(2)
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Playback History
                    Text {
                        text: "Playback History"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "History Retention Size"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Maximum number of past tracks remembered in the reversible history stack."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            spacing: 10
                            StyledSlider {
                                id: historySlider
                                Layout.preferredWidth: 140
                                from: 50
                                to: 1000
                                stepSize: 50
                                value: bridge.historyRetentionLimit
                                onMoved: bridge.setHistoryRetentionLimit(Math.round(value))
                            }
                            Text {
                                text: Math.round(historySlider.value) + " tracks"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accentHover
                                Layout.preferredWidth: 70
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        StyledButton {
                            text: "Clear Playback History Now"
                            onClicked: bridge.clearPlaybackHistory()
                        }
                    }
                }
            }

            // Library & Folders
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 5
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Monitored Music Folders"
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        StyledButton {
                            text: "+ Add Folder..."
                            onClicked: addFolderDialog.open()
                        }
                    }

                    Text {
                        text: "Configure monitored filesystem directories, real-time file watching, format filters, and library maintenance."
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Monitored Directories List
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(70, count * 38)
                        model: bridge.monitoredFolders
                        clip: true
                        interactive: false

                        delegate: Rectangle {
                            width: parent.width
                            height: 34
                            color: Theme.background
                            border.color: Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                VectorIcon {
                                    name: "folder"
                                    width: 14
                                    height: 14
                                    color: Theme.accent
                                }
                                Text {
                                    text: modelData
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textPrimary
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                StyledButton {
                                    text: "Reveal"
                                    onClicked: bridge.showInFileManager(modelData)
                                }
                                StyledButton {
                                    text: "Remove"
                                    onClicked: bridge.removeMonitoredFolder(index)
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        visible: bridge.monitoredFolders.length === 0
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            VectorIcon { name: "folder"; width: 16; height: 16; color: Theme.textMuted }
                            Text {
                                text: "No monitored folders configured. Click '+ Add Folder...' to add music."
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                            }
                        }
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Enable real-time filesystem watcher (auto-detects added, modified, or deleted files)"
                        checked: bridge.filesystemWatcher
                        onToggled: bridge.setFilesystemWatcher(checked)
                    }

                    StyledCheckBox {
                        Layout.fillWidth: true
                        text: "Automatically rescan all monitored folders on application startup"
                        checked: bridge.autoScanOnStartup
                        onToggled: bridge.setAutoScanOnStartup(checked)
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Whitelisted File Formats
                    Text {
                        text: "Whitelisted Audio Formats"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Select audio file extensions that will be scanned and indexed into the database."
                        font.pixelSize: Theme.fontSizeSmall - 1
                        color: Theme.textMuted
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 5
                        rowSpacing: 8
                        columnSpacing: 12

                        Repeater {
                            model: ["FLAC", "WAV", "ALAC", "AIFF", "DSD (DSF/DFF)", "MP3", "AAC", "M4A", "OGG", "OPUS"]
                            delegate: StyledCheckBox {
                                text: modelData
                                checked: bridge.isFormatFilterEnabled(modelData)
                                onToggled: bridge.setFormatFilterEnabled(modelData, checked)
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Folder Exclusion Patterns
                    Text {
                        text: "Folder Exclusion Patterns"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Comma-separated subfolder names and glob patterns to ignore during scanning."
                        font.pixelSize: Theme.fontSizeSmall - 1
                        color: Theme.textMuted
                    }

                    StyledTextField {
                        Layout.fillWidth: true
                        text: bridge.excludeFolders
                        onEditingFinished: bridge.setExcludeFolders(text)
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Artwork Search Priority
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Cover Artwork Search Priority"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Priority order between embedded audio file tags and external folder images (cover.jpg, folder.jpg)."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledComboBox {
                            id: artworkPriorityCombo
                            Layout.preferredWidth: 260
                            model: ["Embedded Tags First", "External Folder Art First"]
                            currentIndex: Math.max(0, model.indexOf(bridge.artworkPriority))
                            onActivated: function(index) {
                                bridge.setArtworkPriority(model[index])
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Album Grouping & Featuring Artists
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Group Albums with Guest/Featuring Artists"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Text {
                                text: "Keep albums with guest or featuring artists unified into a single album card instead of splitting them by artist combinations."
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        StyledCheckBox {
                            text: "Unified"
                            checked: bridge.groupAlbumsByTitle
                            onToggled: bridge.setGroupAlbumsByTitle(checked)
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Maintenance Operations
                    Text {
                        text: "Library Maintenance & Database Utilities"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        rowSpacing: 10
                        columnSpacing: 10

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Rescan All Folders Now"
                            onClicked: bridge.rescanAllMonitoredFolders()
                        }

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Incremental Quick Scan"
                            onClicked: bridge.incrementalQuickScan()
                        }

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Purge Missing / Dead Files"
                            onClicked: bridge.purgeMissingTracks()
                        }

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Export Database Backup..."
                            onClicked: exportBackupDialog.open()
                        }

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Optimize SQLite Database"
                            onClicked: bridge.optimizeDatabase()
                        }

                        StyledButton {
                            Layout.fillWidth: true
                            text: "Clear Entire Database"
                            onClicked: confirmClearDbDialog.open()
                        }
                    }
                }
            }

            // Metadata & Scrobbling
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 6
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Metadata & Scrobbling"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Last.fm
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: lastfmCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: lastfmCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            StyledCheckBox {
                                text: "Enable Last.fm Scrobbling"
                                checked: bridge.lastfmEnabled
                                onToggled: bridge.setLastfmEnabled(checked)
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                enabled: bridge.lastfmEnabled
                                opacity: bridge.lastfmEnabled ? 1.0 : 0.5

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "Username"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                                    StyledTextField {
                                        Layout.fillWidth: true
                                        text: bridge.lastfmUsername
                                        placeholderText: "Last.fm username"
                                        onEditingFinished: bridge.setLastfmUsername(text)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "Session Key / Token"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                                    StyledTextField {
                                        Layout.fillWidth: true
                                        text: bridge.lastfmSessionKey
                                        placeholderText: "Session key"
                                        echoMode: TextInput.PasswordEchoOnEdit
                                        onEditingFinished: bridge.setLastfmSessionKey(text)
                                    }
                                }
                            }
                        }
                    }

                    // ListenBrainz
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: lbCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: lbCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            StyledCheckBox {
                                text: "Enable ListenBrainz Scrobbling"
                                checked: bridge.listenbrainzEnabled
                                onToggled: bridge.setListenbrainzEnabled(checked)
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                enabled: bridge.listenbrainzEnabled
                                opacity: bridge.listenbrainzEnabled ? 1.0 : 0.5

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "User API Token"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                                    StyledTextField {
                                        Layout.fillWidth: true
                                        text: bridge.listenbrainzToken
                                        placeholderText: "ListenBrainz user API token"
                                        echoMode: TextInput.PasswordEchoOnEdit
                                        onEditingFinished: bridge.setListenbrainzToken(text)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "API Base URL"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary }
                                    StyledTextField {
                                        Layout.fillWidth: true
                                        text: bridge.listenbrainzApiUrl
                                        placeholderText: "https://api.listenbrainz.org/1/"
                                        onEditingFinished: bridge.setListenbrainzApiUrl(text)
                                    }
                                }
                            }
                        }
                    }

                    // Scrobble Rules
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: rulesCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: rulesCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            Text {
                                text: "Scrobble Thresholds & Cache"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                Text {
                                    text: "Playback percentage trigger: " + bridge.scrobbleThresholdPercent + "%"
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    Layout.preferredWidth: 200
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 50
                                    to: 100
                                    stepSize: 5
                                    value: bridge.scrobbleThresholdPercent
                                    onMoved: bridge.setScrobbleThresholdPercent(Math.round(value))
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                Text {
                                    text: "Maximum elapsed time: " + bridge.scrobbleThresholdTimeSec + "s"
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    Layout.preferredWidth: 200
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 30
                                    to: 300
                                    stepSize: 10
                                    value: bridge.scrobbleThresholdTimeSec
                                    onMoved: bridge.setScrobbleThresholdTimeSec(Math.round(value))
                                }
                            }

                            StyledCheckBox {
                                text: "Offline Scrobble Cache (Queue scrobbles when offline and sync upon reconnect)"
                                checked: bridge.offlineScrobbleCache
                                onToggled: bridge.setOfflineScrobbleCache(checked)
                            }
                        }
                    }

                    // Lyrics & Extraction
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: lyricsCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: lyricsCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Lyrics & Tag Extraction"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                Text {
                                    text: "Lyrics Provider Priority"
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                }
                                StyledComboBox {
                                    model: [
                                        "Local .lrc sidecar first",
                                        "Embedded USLT/SYLT tags first",
                                        "Online lyrics services"
                                    ]
                                    currentIndex: Math.max(0, model.indexOf(bridge.lyricsProviderOrder))
                                    onActivated: bridge.setLyricsProviderOrder(currentText)
                                }
                            }

                            StyledCheckBox {
                                text: "Automatically fetch missing synchronized lyrics online"
                                checked: bridge.autoFetchLyrics
                                onToggled: bridge.setAutoFetchLyrics(checked)
                            }
                        }
                    }
                }
            }

            // Keyboard Shortcuts
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 7
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Keyboard Shortcuts & Hotkeys"
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        StyledButton {
                            text: "Import Keymap..."
                            onClicked: importKeymapDialog.open()
                        }
                        StyledButton {
                            text: "Export Keymap..."
                            onClicked: exportKeymapDialog.open()
                        }
                        StyledButton {
                            text: "Restore All Defaults"
                            onClicked: bridge.restoreDefaultHotkeys()
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    StyledCheckBox {
                        text: "Enable System-Wide Global Media Keys Hook (Play, Pause, Next, Prev)"
                        checked: bridge.globalMediaKeysEnabled
                        onToggled: bridge.setGlobalMediaKeysEnabled(checked)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StyledTextField {
                            id: shortcutFilterInput
                            Layout.fillWidth: true
                            placeholderText: "Search shortcuts by name or key sequence..."
                        }
                    }

                    // Hotkeys list table
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 340
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                        clip: true

                        ListView {
                            id: hotkeysListView
                            anchors.fill: parent
                            anchors.margins: 4
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            model: bridge.hotkeysModel
                            ScrollBar.vertical: ScrollBar { }

                            delegate: Rectangle {
                                width: hotkeysListView.width - 8
                                height: visible ? 36 : 0
                                radius: Theme.cornerRadiusSmall
                                color: index % 2 === 0 ? "transparent" : Theme.surfaceElevated
                                visible: {
                                    var q = shortcutFilterInput.text.trim().toLowerCase();
                                    if (!q) return true;
                                    return modelData.actionName.toLowerCase().indexOf(q) !== -1 ||
                                           modelData.category.toLowerCase().indexOf(q) !== -1 ||
                                           modelData.currentSequence.toLowerCase().indexOf(q) !== -1;
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    // Category badge
                                    Rectangle {
                                        Layout.preferredWidth: 105
                                        Layout.preferredHeight: 20
                                        radius: 3
                                        color: Theme.selection
                                        border.color: Theme.accent
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.category
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: Theme.accentHover
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Action name
                                    Text {
                                        text: modelData.actionName
                                        font.pixelSize: Theme.fontSizeSmall
                                        color: Theme.textPrimary
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    // Key sequence chip
                                    Rectangle {
                                        Layout.preferredWidth: 110
                                        Layout.preferredHeight: 22
                                        radius: 3
                                        color: Theme.surface
                                        border.color: Theme.panelBorder
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.currentSequence !== "" ? modelData.currentSequence : "None"
                                            font.pixelSize: Theme.fontSizeSmall
                                            font.bold: true
                                            color: Theme.accent
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Edit button
                                    StyledButton {
                                        text: "Edit"
                                        implicitHeight: 24
                                        implicitWidth: 50
                                        onClicked: {
                                            root.editingActionId = modelData.id
                                            root.editingActionName = modelData.actionName
                                            root.editingKeySequence = modelData.currentSequence
                                            root.hotkeyConflictText = ""
                                            rebindHotkeyDialog.open()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Diagnostics & Advanced
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 8
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Diagnostics & Advanced Settings"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    // Logging Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: logCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: logCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Logging & System Diagnostics"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                Text {
                                    text: "Log Verbosity Level"
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                }
                                StyledComboBox {
                                    model: ["Trace", "Debug", "Info", "Warning", "Error"]
                                    currentIndex: Math.max(0, model.indexOf(bridge.loggingVerbosity))
                                    onActivated: bridge.setLoggingVerbosity(currentText)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                StyledButton {
                                    Layout.fillWidth: true
                                    text: "Open Log Directory"
                                    onClicked: bridge.openLogDirectory()
                                }

                                StyledButton {
                                    Layout.fillWidth: true
                                    text: "Export Diagnostics Report..."
                                    onClicked: exportDiagnosticsDialog.open()
                                }
                            }
                        }
                    }

                    // Audio & Database Health Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: healthCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: healthCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Database & Pipeline Health"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                StyledButton {
                                    Layout.fillWidth: true
                                    text: "Run SQLite Integrity Check"
                                    onClicked: {
                                        root.dbIntegrityStatus = bridge.checkDatabaseIntegrity()
                                    }
                                }

                                StyledButton {
                                    Layout.fillWidth: true
                                    text: "Audio Stream Diagnostics"
                                    onClicked: bridge.openAudioDiagnostics()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: integrityText.implicitHeight + 16
                                visible: root.dbIntegrityStatus !== ""
                                color: Theme.surfaceElevated
                                border.color: Theme.accent
                                border.width: 1
                                radius: Theme.cornerRadiusSmall

                                Text {
                                    id: integrityText
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    text: root.dbIntegrityStatus
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.accentHover
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Reset Settings Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: resetCol.implicitHeight + 24
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall

                        ColumnLayout {
                            id: resetCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Factory Defaults"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            Text {
                                text: "Reset all application preferences, hotkeys, themes, and audio configurations to initial values."
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            StyledButton {
                                Layout.fillWidth: true
                                text: "Reset All Preferences to Factory Defaults..."
                                onClicked: resetFactoryDefaultsDialog.open()
                            }
                        }
                    }
                }
            }

            // Plugins & Extensions
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: activeCategory === 9
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                clip: true

                ColumnLayout {
                    width: parent.width - 24
                    spacing: 16

                    Text {
                        text: "Plugins & Extensions (PicAudio SDK)"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

                    Text {
                        text: "Capability ports active: IPlayerPlugin, ILibraryPlugin, IVisualizerPlugin"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            VectorIcon { name: "equalizer"; width: 20; height: 20; color: Theme.accent }
                            ColumnLayout {
                                Text { text: "Spectrum Visualizer"; font.bold: true; color: Theme.textPrimary }
                                Text { text: "FFT Audio Spectrum Analyzer using IVisualizerPlugin"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textMuted }
                            }
                        }
                    }
                }
            }
        }
    }
}
