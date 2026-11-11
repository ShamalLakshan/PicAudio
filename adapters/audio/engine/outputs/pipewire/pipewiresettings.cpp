/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipewiresettings.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

using namespace Qt::StringLiterals;

namespace PicAudioEngine::Pipewire {
PipewireSettings::PipewireSettings(QWidget* parent)
    : QDialog{parent}
    , m_latency{new QSpinBox(this)}
{
    setWindowTitle(tr("PipeWire Settings"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttons, &QDialogButtonBox::accepted, this, &PipewireSettings::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this, &PipewireSettings::reject);

    auto* latencyLabel = new QLabel(tr("Latency") + ":"_L1, this);

    m_latency->setRange(0, MaximumLatency);
    m_latency->setSuffix(u" ms"_s);
    m_latency->setSpecialValueText(tr("Automatic"));

    const auto latencyTooltip
        = tr("Lower values provide more frequent audio processing but may increase underrun risk.\n"
             "Higher values improve stability but increase latency and may reduce visualisation smoothness.\n"
             "Changes take effect the next time the PipeWire output is initialised.");

    latencyLabel->setToolTip(latencyTooltip);
    m_latency->setToolTip(latencyTooltip);

    auto* layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    int row{0};
    layout->addWidget(latencyLabel, row, 0);
    layout->addWidget(m_latency, row++, 1);
    layout->addWidget(buttons, row++, 0, 1, 2, Qt::AlignBottom);

    m_latency->setValue(m_settings.value(LatencySetting, DefaultLatency).toInt());
}

void PipewireSettings::accept()
{
    m_settings.setValue(LatencySetting, m_latency->value());

    done(Accepted);
}
} // namespace PicAudioEngine::Pipewire
