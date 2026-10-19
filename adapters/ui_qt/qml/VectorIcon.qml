import QtQuick

Canvas {
    id: root
    width: 16
    height: 16

    property string name: "music"
    property color color: Theme.textSecondary

    onNameChanged: requestPaint()
    onColorChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.clearRect(0, 0, width, height);
        ctx.fillStyle = root.color;
        ctx.strokeStyle = root.color;
        ctx.lineWidth = Math.max(1.2, width / 12);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        var w = width;
        var h = height;

        if (name === "play") {
            ctx.beginPath();
            ctx.moveTo(w * 0.25, h * 0.18);
            ctx.lineTo(w * 0.82, h * 0.5);
            ctx.lineTo(w * 0.25, h * 0.82);
            ctx.closePath();
            ctx.fill();
        } else if (name === "pause") {
            var barW = w * 0.22;
            ctx.fillRect(w * 0.22, h * 0.18, barW, h * 0.64);
            ctx.fillRect(w * 0.56, h * 0.18, barW, h * 0.64);
        } else if (name === "stop") {
            ctx.fillRect(w * 0.22, h * 0.22, w * 0.56, h * 0.56);
        } else if (name === "previous") {
            ctx.fillRect(w * 0.16, h * 0.2, w * 0.14, h * 0.6);
            ctx.beginPath();
            ctx.moveTo(w * 0.82, h * 0.2);
            ctx.lineTo(w * 0.35, h * 0.5);
            ctx.lineTo(w * 0.82, h * 0.8);
            ctx.closePath();
            ctx.fill();
        } else if (name === "next") {
            ctx.fillRect(w * 0.7, h * 0.2, w * 0.14, h * 0.6);
            ctx.beginPath();
            ctx.moveTo(w * 0.18, h * 0.2);
            ctx.lineTo(w * 0.65, h * 0.5);
            ctx.lineTo(w * 0.18, h * 0.8);
            ctx.closePath();
            ctx.fill();
        } else if (name === "shuffle") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.3);
            ctx.lineTo(w * 0.45, h * 0.7);
            ctx.lineTo(w * 0.8, h * 0.7);
            ctx.moveTo(w * 0.15, h * 0.7);
            ctx.lineTo(w * 0.45, h * 0.3);
            ctx.lineTo(w * 0.8, h * 0.3);
            ctx.stroke();
            // arrowheads
            ctx.beginPath();
            ctx.moveTo(w * 0.7, h * 0.2);
            ctx.lineTo(w * 0.85, h * 0.3);
            ctx.lineTo(w * 0.7, h * 0.4);
            ctx.moveTo(w * 0.7, h * 0.6);
            ctx.lineTo(w * 0.85, h * 0.7);
            ctx.lineTo(w * 0.7, h * 0.8);
            ctx.stroke();
        } else if (name === "repeat") {
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.32, -0.3, Math.PI + 0.3, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.32, Math.PI - 0.3, 0.3, false);
            ctx.stroke();
            // arrows
            ctx.beginPath();
            ctx.moveTo(w * 0.75, h * 0.35);
            ctx.lineTo(w * 0.85, h * 0.48);
            ctx.lineTo(w * 0.65, h * 0.48);
            ctx.fill();
        } else if (name === "volume" || name === "volume_high") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.38);
            ctx.lineTo(w * 0.32, h * 0.38);
            ctx.lineTo(w * 0.52, h * 0.2);
            ctx.lineTo(w * 0.52, h * 0.8);
            ctx.lineTo(w * 0.32, h * 0.62);
            ctx.lineTo(w * 0.15, h * 0.62);
            ctx.closePath();
            ctx.fill();
            // arcs
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.22, -0.6, 0.6, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.36, -0.7, 0.7, false);
            ctx.stroke();
        } else if (name === "volume_mute") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.38);
            ctx.lineTo(w * 0.32, h * 0.38);
            ctx.lineTo(w * 0.52, h * 0.2);
            ctx.lineTo(w * 0.52, h * 0.8);
            ctx.lineTo(w * 0.32, h * 0.62);
            ctx.lineTo(w * 0.15, h * 0.62);
            ctx.closePath();
            ctx.fill();
            // X
            ctx.beginPath();
            ctx.moveTo(w * 0.65, h * 0.35);
            ctx.lineTo(w * 0.85, h * 0.65);
            ctx.moveTo(w * 0.85, h * 0.35);
            ctx.lineTo(w * 0.65, h * 0.65);
        } else if (name === "filter") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.22);
            ctx.lineTo(w * 0.85, h * 0.22);
            ctx.lineTo(w * 0.58, h * 0.54);
            ctx.lineTo(w * 0.58, h * 0.82);
            ctx.lineTo(w * 0.42, h * 0.72);
            ctx.lineTo(w * 0.42, h * 0.54);
            ctx.closePath();
            ctx.stroke();
        } else if (name === "search") {
            var cr = w * 0.26;
            ctx.beginPath();
            ctx.arc(w * 0.42, h * 0.42, cr, 0, Math.PI * 2);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(w * 0.6, h * 0.6);
            ctx.lineTo(w * 0.85, h * 0.85);
            ctx.stroke();
        } else if (name === "folder") {
            ctx.beginPath();
            ctx.moveTo(w * 0.12, h * 0.25);
            ctx.lineTo(w * 0.4, h * 0.25);
            ctx.lineTo(w * 0.5, h * 0.35);
            ctx.lineTo(w * 0.88, h * 0.35);
            ctx.lineTo(w * 0.88, h * 0.78);
            ctx.lineTo(w * 0.12, h * 0.78);
            ctx.closePath();
            ctx.stroke();
        } else if (name === "music") {
            ctx.beginPath();
            ctx.arc(w * 0.3, h * 0.7, w * 0.12, 0, Math.PI * 2);
            ctx.arc(w * 0.7, h * 0.6, w * 0.12, 0, Math.PI * 2);
            ctx.fill();
            ctx.beginPath();
            ctx.moveTo(w * 0.4, h * 0.7);
            ctx.lineTo(w * 0.4, h * 0.25);
            ctx.lineTo(w * 0.8, h * 0.15);
            ctx.lineTo(w * 0.8, h * 0.6);
            ctx.stroke();
        } else if (name === "disc") {
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.42, 0, Math.PI * 2);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.22, 0, Math.PI * 2);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.08, 0, Math.PI * 2);
            ctx.fill();
        } else if (name === "lyrics") {
            ctx.beginPath();
            ctx.moveTo(w * 0.16, h * 0.24);
            ctx.lineTo(w * 0.84, h * 0.24);
            ctx.moveTo(w * 0.16, h * 0.46);
            ctx.lineTo(w * 0.64, h * 0.46);
            ctx.moveTo(w * 0.16, h * 0.68);
            ctx.lineTo(w * 0.84, h * 0.68);
            ctx.moveTo(w * 0.16, h * 0.86);
            ctx.lineTo(w * 0.52, h * 0.86);
            ctx.stroke();
        } else if (name === "wave") {
            ctx.beginPath();
            ctx.moveTo(w * 0.14, h * 0.4);
            ctx.lineTo(w * 0.14, h * 0.6);
            ctx.moveTo(w * 0.32, h * 0.22);
            ctx.lineTo(w * 0.32, h * 0.78);
            ctx.moveTo(w * 0.50, h * 0.12);
            ctx.lineTo(w * 0.50, h * 0.88);
            ctx.moveTo(w * 0.68, h * 0.28);
            ctx.lineTo(w * 0.68, h * 0.72);
            ctx.moveTo(w * 0.86, h * 0.42);
            ctx.lineTo(w * 0.86, h * 0.58);
            ctx.stroke();
        } else if (name === "grid") {
            var sz = w * 0.32;
            ctx.strokeRect(w * 0.14, h * 0.14, sz, sz);
            ctx.strokeRect(w * 0.54, h * 0.14, sz, sz);
            ctx.strokeRect(w * 0.14, h * 0.54, sz, sz);
            ctx.strokeRect(w * 0.54, h * 0.54, sz, sz);
        } else if (name === "table") {
            ctx.strokeRect(w * 0.12, h * 0.15, w * 0.76, h * 0.7);
            ctx.beginPath();
            ctx.moveTo(w * 0.12, h * 0.38);
            ctx.lineTo(w * 0.88, h * 0.38);
            ctx.moveTo(w * 0.12, h * 0.62);
            ctx.lineTo(w * 0.88, h * 0.62);
            ctx.moveTo(w * 0.36, h * 0.15);
            ctx.lineTo(w * 0.36, h * 0.85);
            ctx.stroke();
        } else if (name === "album_tracks") {
            ctx.strokeRect(w * 0.12, h * 0.2, w * 0.3, w * 0.3);
            ctx.beginPath();
            ctx.moveTo(w * 0.52, h * 0.25);
            ctx.lineTo(w * 0.88, h * 0.25);
            ctx.moveTo(w * 0.52, h * 0.45);
            ctx.lineTo(w * 0.88, h * 0.45);
            ctx.moveTo(w * 0.12, h * 0.68);
            ctx.lineTo(w * 0.88, h * 0.68);
            ctx.moveTo(w * 0.12, h * 0.82);
            ctx.lineTo(w * 0.88, h * 0.82);
            ctx.stroke();
        } else if (name === "panel_left") {
            ctx.strokeRect(w * 0.12, h * 0.15, w * 0.76, h * 0.7);
            ctx.fillRect(w * 0.12, h * 0.15, w * 0.26, h * 0.7);
        } else if (name === "panel_right") {
            ctx.strokeRect(w * 0.12, h * 0.15, w * 0.76, h * 0.7);
            ctx.fillRect(w * 0.62, h * 0.15, w * 0.26, h * 0.7);
        } else if (name === "clear" || name === "close") {
            ctx.beginPath();
            ctx.moveTo(w * 0.22, h * 0.22);
            ctx.lineTo(w * 0.78, h * 0.78);
            ctx.moveTo(w * 0.78, h * 0.22);
            ctx.lineTo(w * 0.22, h * 0.78);
            ctx.stroke();
        } else if (name === "chevron_left") {
            ctx.beginPath();
            ctx.moveTo(w * 0.65, h * 0.2);
            ctx.lineTo(w * 0.35, h * 0.5);
            ctx.lineTo(w * 0.65, h * 0.8);
            ctx.stroke();
        } else if (name === "chevron_right") {
            ctx.beginPath();
            ctx.moveTo(w * 0.35, h * 0.2);
            ctx.lineTo(w * 0.65, h * 0.5);
            ctx.lineTo(w * 0.35, h * 0.8);
            ctx.stroke();
        } else if (name === "fullscreen") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.35);
            ctx.lineTo(w * 0.15, h * 0.15);
            ctx.lineTo(w * 0.35, h * 0.15);
            ctx.moveTo(w * 0.65, h * 0.15);
            ctx.lineTo(w * 0.85, h * 0.15);
            ctx.lineTo(w * 0.85, h * 0.35);
            ctx.moveTo(w * 0.85, h * 0.65);
            ctx.lineTo(w * 0.85, h * 0.85);
            ctx.lineTo(w * 0.65, h * 0.85);
            ctx.moveTo(w * 0.35, h * 0.85);
            ctx.lineTo(w * 0.15, h * 0.85);
            ctx.lineTo(w * 0.15, h * 0.65);
            ctx.stroke();
        } else if (name === "queue") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.3);
            ctx.lineTo(w * 0.6, h * 0.3);
            ctx.moveTo(w * 0.15, h * 0.5);
            ctx.lineTo(w * 0.6, h * 0.5);
            ctx.moveTo(w * 0.15, h * 0.7);
            ctx.lineTo(w * 0.85, h * 0.7);
            ctx.stroke();
            // mini play arrow
            ctx.beginPath();
            ctx.moveTo(w * 0.72, h * 0.26);
            ctx.lineTo(w * 0.88, h * 0.4);
            ctx.lineTo(w * 0.72, h * 0.54);
            ctx.closePath();
            ctx.fill();
        } else if (name === "playlist") {
            ctx.beginPath();
            ctx.moveTo(w * 0.15, h * 0.28);
            ctx.lineTo(w * 0.62, h * 0.28);
            ctx.moveTo(w * 0.15, h * 0.48);
            ctx.lineTo(w * 0.62, h * 0.48);
            ctx.moveTo(w * 0.15, h * 0.68);
            ctx.lineTo(w * 0.48, h * 0.68);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.68, h * 0.68, w * 0.1, 0, Math.PI * 2);
            ctx.fill();
            ctx.beginPath();
            ctx.moveTo(w * 0.78, h * 0.68);
            ctx.lineTo(w * 0.78, h * 0.35);
            ctx.lineTo(w * 0.88, h * 0.35);
            ctx.stroke();
        } else if (name === "heart") {
            ctx.beginPath();
            ctx.moveTo(w * 0.5, h * 0.78);
            ctx.bezierCurveTo(w * 0.15, h * 0.5, w * 0.1, h * 0.25, w * 0.35, h * 0.25);
            ctx.bezierCurveTo(w * 0.45, h * 0.25, w * 0.5, h * 0.35, w * 0.5, h * 0.35);
            ctx.bezierCurveTo(w * 0.5, h * 0.35, w * 0.55, h * 0.25, w * 0.65, h * 0.25);
            ctx.bezierCurveTo(w * 0.9, h * 0.25, w * 0.85, h * 0.5, w * 0.5, h * 0.78);
            ctx.closePath();
            ctx.fill();
        } else if (name === "trash") {
            ctx.beginPath();
            ctx.moveTo(w * 0.2, h * 0.3);
            ctx.lineTo(w * 0.8, h * 0.3);
            ctx.moveTo(w * 0.35, h * 0.2);
            ctx.lineTo(w * 0.65, h * 0.2);
            ctx.moveTo(w * 0.28, h * 0.3);
            ctx.lineTo(w * 0.32, h * 0.8);
            ctx.lineTo(w * 0.68, h * 0.8);
            ctx.lineTo(w * 0.72, h * 0.3);
            ctx.stroke();
        } else if (name === "equalizer") {
            ctx.beginPath();
            ctx.moveTo(w * 0.25, h * 0.15);
            ctx.lineTo(w * 0.25, h * 0.85);
            ctx.moveTo(w * 0.5, h * 0.15);
            ctx.lineTo(w * 0.5, h * 0.85);
            ctx.moveTo(w * 0.75, h * 0.15);
            ctx.lineTo(w * 0.75, h * 0.85);
            ctx.stroke();
            ctx.fillRect(w * 0.18, h * 0.35, w * 0.14, h * 0.14);
            ctx.fillRect(w * 0.43, h * 0.55, w * 0.14, h * 0.14);
            ctx.fillRect(w * 0.68, h * 0.25, w * 0.14, h * 0.14);
        } else if (name === "repeat_one") {
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.34, -0.3, Math.PI + 0.3, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.34, Math.PI - 0.3, 0.3, false);
            ctx.stroke();
            // arrows
            ctx.beginPath();
            ctx.moveTo(w * 0.76, h * 0.35);
            ctx.lineTo(w * 0.88, h * 0.48);
            ctx.lineTo(w * 0.64, h * 0.48);
            ctx.fill();
            // numeral 1 in center
            ctx.beginPath();
            ctx.moveTo(w * 0.44, h * 0.42);
            ctx.lineTo(w * 0.50, h * 0.36);
            ctx.lineTo(w * 0.50, h * 0.65);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(w * 0.42, h * 0.65);
            ctx.lineTo(w * 0.58, h * 0.65);
            ctx.stroke();
        } else if (name === "more_vert") {
            var r = Math.max(1.2, w * 0.08);
            ctx.beginPath(); ctx.arc(w * 0.5, h * 0.25, r, 0, Math.PI * 2); ctx.fill();
            ctx.beginPath(); ctx.arc(w * 0.5, h * 0.5, r, 0, Math.PI * 2); ctx.fill();
            ctx.beginPath(); ctx.arc(w * 0.5, h * 0.75, r, 0, Math.PI * 2); ctx.fill();
        } else if (name === "settings") {
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.22, 0, Math.PI * 2);
            ctx.stroke();
            for (var i = 0; i < 8; i++) {
                var angle = i * (Math.PI / 4);
                var cos = Math.cos(angle);
                var sin = Math.sin(angle);
                ctx.beginPath();
                ctx.moveTo(w * 0.5 + cos * w * 0.24, h * 0.5 + sin * h * 0.24);
                ctx.lineTo(w * 0.5 + cos * w * 0.38, h * 0.5 + sin * h * 0.38);
                ctx.stroke();
            }
        } else if (name === "copy") {
            ctx.strokeRect(w * 0.2, h * 0.35, w * 0.45, h * 0.45);
            ctx.beginPath();
            ctx.moveTo(w * 0.35, h * 0.35);
            ctx.lineTo(w * 0.35, h * 0.2);
            ctx.lineTo(w * 0.8, h * 0.2);
            ctx.lineTo(w * 0.8, h * 0.65);
            ctx.lineTo(w * 0.65, h * 0.65);
            ctx.stroke();
        } else if (name === "external") {
            ctx.strokeRect(w * 0.18, h * 0.32, w * 0.5, h * 0.5);
            ctx.beginPath();
            ctx.moveTo(w * 0.48, h * 0.48);
            ctx.lineTo(w * 0.82, h * 0.18);
            ctx.moveTo(w * 0.6, h * 0.18);
            ctx.lineTo(w * 0.82, h * 0.18);
            ctx.lineTo(w * 0.82, h * 0.4);
            ctx.stroke();
        } else if (name === "check") {
            ctx.beginPath();
            ctx.moveTo(w * 0.22, h * 0.52);
            ctx.lineTo(w * 0.44, h * 0.74);
            ctx.lineTo(w * 0.82, h * 0.26);
            ctx.stroke();
        } else if (name === "arrow_up") {
            ctx.beginPath();
            ctx.moveTo(w * 0.25, h * 0.6);
            ctx.lineTo(w * 0.5, h * 0.3);
            ctx.lineTo(w * 0.75, h * 0.6);
            ctx.stroke();
        } else if (name === "arrow_down") {
            ctx.beginPath();
            ctx.moveTo(w * 0.25, h * 0.4);
            ctx.lineTo(w * 0.5, h * 0.7);
            ctx.lineTo(w * 0.75, h * 0.4);
            ctx.stroke();
        } else if (name === "info") {
            ctx.beginPath();
            ctx.arc(w * 0.5, h * 0.5, w * 0.36, 0, Math.PI * 2);
            ctx.stroke();
            ctx.fillRect(w * 0.45, h * 0.25, w * 0.1, w * 0.1);
            ctx.fillRect(w * 0.45, h * 0.42, w * 0.1, h * 0.3);
        }
    }
}
