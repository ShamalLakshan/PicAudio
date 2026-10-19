import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: root

    property bool active: bridge.isPlaying
    property color waveColor: Theme.accent
    property color waveColorSecondary: Theme.accentHover
    property int visualizerStyle: 0 // 0: Fluid Wave, 1: Audio Spectrum Bars
    property real currentAmplitude: 0.0
    property real phase: 0.0

    property color backgroundColor: Theme.surfaceElevated
    property color borderColor: Theme.panelBorder
    property int visualizerRadius: Theme.cornerRadiusSmall
    property int visualizerBorderWidth: 1

    implicitWidth: 280
    implicitHeight: 70
    clip: true

    Connections {
        target: bridge
        function onVisualizerDataChanged() {
            if (root.visible) {
                waveCanvas.requestPaint();
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
        radius: root.visualizerRadius
        border.color: root.borderColor
        border.width: root.visualizerBorderWidth

        Canvas {
            id: waveCanvas
            anchors.fill: parent
            anchors.margins: 4
            antialiasing: true
            renderTarget: Canvas.FramebufferObject

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.clearRect(0, 0, width, height);

                var w = width;
                var h = height;
                if (w <= 0 || h <= 0) return;

                var midY = h * 0.5;
                var realLevel = bridge.visualizerLevel;
                var amp = Math.max(root.currentAmplitude, realLevel);
                var realWave = bridge.visualizerWaveform;
                var realBands = bridge.visualizerBands;
                var hasRealAudio = realBands && realBands.length > 0 && amp > 0.005;

                if (root.visualizerStyle === 0) {
                    // Style 0: Fluid Waveform driven by real audio PCM
                    var maxWaveHeight = h * 0.42 * (hasRealAudio ? Math.min(1.0, amp * 1.5 + 0.1) : amp);

                    if (hasRealAudio && realWave && realWave.length >= 16) {
                        var wavePts = realWave.length;

                        // Layer 1: Semi-transparent background wave fill
                        ctx.beginPath();
                        ctx.moveTo(0, midY);
                        for (var i = 0; i < wavePts; ++i) {
                            var px = (i / (wavePts - 1)) * w;
                            var s1 = realWave[i];
                            var py1 = midY - s1 * (maxWaveHeight * 0.7);
                            if (i === 0) {
                                ctx.moveTo(px, py1);
                            } else {
                                var prevX = ((i - 1) / (wavePts - 1)) * w;
                                var prevY = midY - realWave[i - 1] * (maxWaveHeight * 0.7);
                                var cpx = (prevX + px) * 0.5;
                                ctx.quadraticCurveTo(prevX, prevY, cpx, (prevY + py1) * 0.5);
                            }
                        }
                        ctx.lineTo(w, h);
                        ctx.lineTo(0, h);
                        ctx.closePath();

                        var grad1 = ctx.createLinearGradient(0, midY - maxWaveHeight, 0, h);
                        grad1.addColorStop(0, Qt.rgba(root.waveColorSecondary.r, root.waveColorSecondary.g, root.waveColorSecondary.b, 0.28));
                        grad1.addColorStop(1, Qt.rgba(root.waveColorSecondary.r, root.waveColorSecondary.g, root.waveColorSecondary.b, 0.02));
                        ctx.fillStyle = grad1;
                        ctx.fill();

                        // Layer 2: Real PCM Audio Center Wave Stroke
                        ctx.beginPath();
                        for (var j = 0; j < wavePts; ++j) {
                            var x2 = (j / (wavePts - 1)) * w;
                            var s2 = realWave[j];
                            var y2 = midY - s2 * maxWaveHeight;
                            if (j === 0) {
                                ctx.moveTo(x2, y2);
                            } else {
                                var px2 = ((j - 1) / (wavePts - 1)) * w;
                                var py2 = midY - realWave[j - 1] * maxWaveHeight;
                                var cx2 = (px2 + x2) * 0.5;
                                ctx.quadraticCurveTo(px2, py2, cx2, (py2 + y2) * 0.5);
                            }
                        }
                        ctx.lineWidth = Math.max(1.8, Math.min(2.8, h * 0.04));
                        ctx.strokeStyle = root.waveColor;
                        ctx.stroke();
                    } else {
                        // Fallback fluid resting wave
                        ctx.beginPath();
                        ctx.moveTo(0, midY);
                        for (var x1 = 0; x1 <= w; x1 += 4) {
                            var y1 = midY + Math.sin(x1 * 0.02 + root.phase * 0.7) * (maxWaveHeight * 0.6)
                                          + Math.cos(x1 * 0.035 - root.phase * 0.5) * (maxWaveHeight * 0.4);
                            ctx.lineTo(x1, y1);
                        }
                        ctx.lineTo(w, h);
                        ctx.lineTo(0, h);
                        ctx.closePath();

                        var gradFallback = ctx.createLinearGradient(0, midY - maxWaveHeight, 0, h);
                        gradFallback.addColorStop(0, Qt.rgba(root.waveColorSecondary.r, root.waveColorSecondary.g, root.waveColorSecondary.b, 0.20));
                        gradFallback.addColorStop(1, Qt.rgba(root.waveColorSecondary.r, root.waveColorSecondary.g, root.waveColorSecondary.b, 0.02));
                        ctx.fillStyle = gradFallback;
                        ctx.fill();

                        ctx.beginPath();
                        ctx.moveTo(0, midY);
                        for (var x2 = 0; x2 <= w; x2 += 3) {
                            var y2 = midY + Math.sin(x2 * 0.028 + root.phase) * maxWaveHeight
                                          + Math.sin(x2 * 0.015 - root.phase * 1.3) * (maxWaveHeight * 0.35);
                            ctx.lineTo(x2, y2);
                        }
                        ctx.lineWidth = Math.max(1.5, Math.min(2.5, h * 0.04));
                        ctx.strokeStyle = root.waveColor;
                        ctx.stroke();
                    }

                    // Resting baseline center
                    if (amp < 0.08) {
                        ctx.beginPath();
                        ctx.moveTo(0, midY);
                        ctx.lineTo(w, midY);
                        ctx.lineWidth = 1;
                        ctx.strokeStyle = Qt.rgba(root.waveColor.r, root.waveColor.g, root.waveColor.b, 0.25);
                        ctx.stroke();
                    }
                } else {
                    // Style 1: Real Multi-Band FFT Audio Spectrum Bars
                    var barCount = (realBands && realBands.length > 0) ? realBands.length : 36;
                    var totalSpacing = (barCount - 1) * 2;
                    var barWidth = Math.max(2, (w - totalSpacing) / barCount);
                    var maxBarH = h * 0.85;

                    for (var b = 0; b < barCount; ++b) {
                        var bandVal = (realBands && realBands.length > b) ? realBands[b] : 0.0;
                        if (!hasRealAudio) {
                            var normX = b / (barCount - 1);
                            var env = Math.sin(normX * Math.PI);
                            bandVal = env * (Math.sin(b * 0.4 + root.phase * 1.4) * 0.5 + 0.5) * amp;
                        }

                        var barH = Math.max(3, bandVal * maxBarH);
                        var barX = b * (barWidth + 2);
                        var barY = midY - barH * 0.5;

                        var barGrad = ctx.createLinearGradient(barX, barY, barX, barY + barH);
                        barGrad.addColorStop(0, root.waveColorSecondary);
                        barGrad.addColorStop(1, root.waveColor);
                        ctx.fillStyle = barGrad;

                        var rad = Math.min(barWidth * 0.5, 2.5);
                        ctx.beginPath();
                        ctx.moveTo(barX + rad, barY);
                        ctx.lineTo(barX + barWidth - rad, barY);
                        ctx.quadraticCurveTo(barX + barWidth, barY, barX + barWidth, barY + rad);
                        ctx.lineTo(barX + barWidth, barY + barH - rad);
                        ctx.quadraticCurveTo(barX + barWidth, barY + barH, barX + barWidth - rad, barY + barH);
                        ctx.lineTo(barX + rad, barY + barH);
                        ctx.quadraticCurveTo(barX, barY + barH, barX, barY + barH - rad);
                        ctx.lineTo(barX, barY + rad);
                        ctx.quadraticCurveTo(barX, barY, barX + rad, barY);
                        ctx.closePath();
                        ctx.fill();
                    }
                }
            }
        }

        // Animation drive timer for smooth phase rotation & decay
        Timer {
            id: animTimer
            interval: 33
            running: root.visible && (root.active || root.currentAmplitude > 0.005)
            repeat: true
            onTriggered: {
                root.phase += 0.08;
                var targetAmp = root.active ? 1.0 : 0.0;
                root.currentAmplitude += (targetAmp - root.currentAmplitude) * 0.12;
                if (Math.abs(targetAmp - root.currentAmplitude) < 0.005) {
                    root.currentAmplitude = targetAmp;
                }
                waveCanvas.requestPaint();
            }
        }

        // Click to toggle visualizer style
        MouseArea {
            id: waveMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.visualizerStyle = (root.visualizerStyle === 0) ? 1 : 0;
                waveCanvas.requestPaint();
            }
        }

        ToolTip.visible: waveMouse.containsMouse
        ToolTip.text: root.visualizerStyle === 0 ? "Waveform Visualizer • Click to switch to Equalizer Bars" : "Equalizer Bars • Click to switch to Fluid Waveform"
    }
}
