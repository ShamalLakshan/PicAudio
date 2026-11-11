/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "outputtransaction.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

using namespace Qt::StringLiterals;

namespace {
QString tempTemplate(const QString& destination)
{
    const QFileInfo info{destination};
    QString name = u"."_s + info.completeBaseName() + u".picaudio-XXXXXX"_s;
    if(!info.suffix().isEmpty()) {
        name += u'.' + info.suffix();
    }
    return info.absoluteDir().filePath(name);
}
} // namespace

namespace PicAudioEngine {
OutputTransaction::OutputTransaction(QString destination)
    : m_destination{std::move(destination)}
    , m_temporary{tempTemplate(m_destination)}
{
    m_temporary.setAutoRemove(true);
}

QString OutputTransaction::path() const
{
    return m_temporary.fileName();
}

bool OutputTransaction::prepare(QString& error)
{
    if(!m_temporary.open()) {
        error = u"Failed to create temporary output: %1"_s.arg(m_temporary.errorString());
        return false;
    }
    m_temporary.close();
    return true;
}

bool OutputTransaction::commit(QString& error)
{
    QFile source{path()};
    if(!source.open(QIODevice::ReadOnly)) {
        error = u"Failed to open temporary output: %1"_s.arg(source.errorString());
        return false;
    }

    QSaveFile destination{m_destination};
    destination.setDirectWriteFallback(false);
    if(!destination.open(QIODevice::WriteOnly)) {
        error = u"Failed to prepare output file: %1"_s.arg(destination.errorString());
        return false;
    }

    while(!source.atEnd()) {
        const QByteArray data = source.read(1024LL * 1024);
        if(data.isEmpty()) {
            if(source.error() != QFileDevice::NoError) {
                error = u"Failed to read temporary output: %1"_s.arg(source.errorString());
                return false;
            }
            break;
        }
        if(destination.write(data) != data.size()) {
            error = u"Failed to write output file: %1"_s.arg(destination.errorString());
            return false;
        }
    }

    if(!destination.commit()) {
        error = u"Failed to commit output file: %1"_s.arg(destination.errorString());
        return false;
    }
    return true;
}
} // namespace PicAudioEngine
