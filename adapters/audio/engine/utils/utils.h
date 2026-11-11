/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QStringView>
#include <QTime>

#include <optional>
#include <vector>

class QAction;
class QColor;
class QHeaderView;
class QImage;
class QMainWindow;
class QMenu;
class QPixmap;
class QSettings;
class QSize;
class QString;
class QWidget;

namespace PicAudioEngine::Utils {
enum class DateTimePrecision : uint8_t
{
    Year   = 4,
    Month  = 7,
    Day    = 10,
    Hour   = 12,
    Minute = 16,
    Second = 19
};

struct PICAUDIO_UTILS_EXPORT ParsedDateTime
{
    QDate date;
    QTime time;
    DateTimePrecision precision;

    [[nodiscard]] QDateTime toDateTime() const
    {
        return {date, time};
    }
};

PICAUDIO_UTILS_EXPORT int randomNumber(int min, int max);

PICAUDIO_UTILS_EXPORT uint64_t currentDateToInt();
PICAUDIO_UTILS_EXPORT QString formatTimeMs(uint64_t time);
PICAUDIO_UTILS_EXPORT std::optional<ParsedDateTime> parseDateTime(QStringView value);
PICAUDIO_UTILS_EXPORT QDateTime dateStringToDate(const QString& str);
PICAUDIO_UTILS_EXPORT std::optional<int64_t> dateStringToMs(const QString& str);
PICAUDIO_UTILS_EXPORT QString msToDateString(int64_t dateMs);

PICAUDIO_UTILS_EXPORT QStringList extensionsToWildcards(const QStringList& extensions);
PICAUDIO_UTILS_EXPORT QString extensionsToFilterList(const QStringList& extensions, const QString& name);
PICAUDIO_UTILS_EXPORT QString extensionFromFilter(const QString& filter);

PICAUDIO_UTILS_EXPORT QPixmap scalePixmap(const QPixmap& image, const QSize& size, double dpr, bool upscale = false);
PICAUDIO_UTILS_EXPORT QPixmap scalePixmap(const QPixmap& image, int width, double dpr, bool upscale = false);
PICAUDIO_UTILS_EXPORT QImage scaleImage(const QImage& image, const QSize& size, double dpr, bool upscale = false);
PICAUDIO_UTILS_EXPORT QImage scaleImage(const QImage& image, int width, double dpr, bool upscale = false);
PICAUDIO_UTILS_EXPORT QPixmap changePixmapColour(const QPixmap& orig, const QColor& color);

PICAUDIO_UTILS_EXPORT QMainWindow* getMainWindow();
PICAUDIO_UTILS_EXPORT double windowDpr();
PICAUDIO_UTILS_EXPORT QSize proportionateSize(const QWidget* widget, double widthFactor, double heightFactor);
PICAUDIO_UTILS_EXPORT void showMessageBox(const QString& text, const QString& infoText);
PICAUDIO_UTILS_EXPORT void appendMenuActions(QMenu* originalMenu, QMenu* menu);
PICAUDIO_UTILS_EXPORT void forwardMenuStatusTips(QMenu* menu, QWidget* target = nullptr);
PICAUDIO_UTILS_EXPORT QAction* cloneMenuAction(QMenu* menu, const QAction* source, const std::function<void()>& handler);
PICAUDIO_UTILS_EXPORT int visibleSectionCount(const QHeaderView* headerView);
PICAUDIO_UTILS_EXPORT int firstVisualIndex(const QHeaderView* headerView);
PICAUDIO_UTILS_EXPORT int realVisualIndex(const QHeaderView* headerView, int logicalIndex);
PICAUDIO_UTILS_EXPORT std::vector<int> logicalIndexOrder(const QHeaderView* headerView);

PICAUDIO_UTILS_EXPORT void saveState(QWidget* widget, QSettings& settings, const QString& name = {});
PICAUDIO_UTILS_EXPORT void restoreState(QWidget* widget, const QSettings& settings, const QString& name = {});

PICAUDIO_UTILS_EXPORT bool isDarkMode();
} // namespace PicAudioEngine::Utils
