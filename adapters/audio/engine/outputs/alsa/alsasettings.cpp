/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "alsasettings.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

using namespace Qt::StringLiterals;

namespace PicAudioEngine {
AlsaSettings::AlsaSettings(QWidget* parent)
    : QDialog{parent}
    , m_bufferLength{new QSpinBox(this)}
    , m_periodLength{new QSpinBox(this)}
{
    setWindowTitle(tr("ALSA Settings"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttons, &QDialogButtonBox::accepted, this, &AlsaSettings::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this, &AlsaSettings::reject);

    auto* bufferLabel = new QLabel(tr("Buffer length") + ":"_L1, this);
    auto* periodLabel = new QLabel(tr("Period length") + ":"_L1, this);

    m_bufferLength->setRange(200, 10000);
    m_bufferLength->setSuffix(u" ms"_s);
    m_periodLength->setRange(20, 5000);
    m_periodLength->setSuffix(u" ms"_s);

    auto* layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    int row{0};
    layout->addWidget(bufferLabel, row, 0);
    layout->addWidget(m_bufferLength, row++, 1);
    layout->addWidget(periodLabel, row, 0);
    layout->addWidget(m_periodLength, row++, 1);
    layout->addWidget(buttons, row++, 0, 1, 2, Qt::AlignBottom);

    m_bufferLength->setValue(m_settings.value(BufferLengthSetting, DefaultBufferLength).toInt());
    m_periodLength->setValue(m_settings.value(PeriodLengthSetting, DefaultPeriodLength).toInt());
}

void AlsaSettings::accept()
{
    m_settings.setValue(BufferLengthSetting, m_bufferLength->value());
    m_settings.setValue(PeriodLengthSetting, m_periodLength->value());

    done(Accepted);
}
} // namespace PicAudioEngine
