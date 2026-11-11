/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "archiveinput.h"

#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(ARCH_READER, "picaudio.engine.archivereader")

namespace PicAudioEngine {
PicAudioEngine::GeneralArchiveReader::GeneralArchiveReader(std::shared_ptr<AudioLoader> audioLoader)
    : m_audioLoader{std::move(audioLoader)}
{ }

GeneralArchiveReader::~GeneralArchiveReader()
{
    m_audioLoader.reset();
}

QStringList GeneralArchiveReader::extensions() const
{
    return m_loadedReader.reader ? m_loadedReader.reader->extensions() : QStringList{};
}

bool GeneralArchiveReader::canReadCover() const
{
    if(m_loadedReader.input.archiveReader) {
        return true;
    }
    if(m_loadedReader.reader) {
        return m_loadedReader.reader->canReadCover();
    }

    return false;
}

bool GeneralArchiveReader::canWriteMetaData() const
{
    // TODO: Support writing
    return false;
}

int GeneralArchiveReader::subsongCount() const
{
    return m_loadedReader.reader ? m_loadedReader.reader->subsongCount() : 1;
}

bool GeneralArchiveReader::init(const AudioSource& source)
{
    const Track track{source.filepath};

    m_loadedReader = m_audioLoader->loadReaderForArchiveTrack(track);
    if(!m_loadedReader.reader) {
        qCDebug(ARCH_READER) << "No reader available for archive track:" << track.filepath();
    }

    return m_loadedReader.reader != nullptr;
}

bool GeneralArchiveReader::readTrack(const AudioSource& source, Track& track)
{
    AudioSource trackSource{source};
    if(!trackSource.device) {
        trackSource.device = m_loadedReader.input.device.get();
    }
    if(trackSource.size == 0) {
        trackSource.size = m_loadedReader.input.source.size;
    }
    if(trackSource.modifiedTime == 0) {
        trackSource.modifiedTime = m_loadedReader.input.source.modifiedTime;
    }
    if(track.fileSize() == 0) {
        if(trackSource.size > 0) {
            track.setFileSize(trackSource.size);
        }
        else if(trackSource.device) {
            track.setFileSize(trackSource.device->size());
        }
    }
    if(track.modifiedTime() == 0) {
        if(trackSource.modifiedTime > 0) {
            track.setModifiedTime(trackSource.modifiedTime);
        }
        else {
            const QDateTime modifiedTime = QFileInfo{track.archivePath()}.lastModified();
            track.setModifiedTime(modifiedTime.isValid() ? modifiedTime.toMSecsSinceEpoch() : 0);
        }
    }

    trackSource.archiveReader = m_loadedReader.input.archiveReader.get();
    return m_loadedReader.reader && m_loadedReader.reader->readTrack(trackSource, track);
}

QByteArray GeneralArchiveReader::readCover(const AudioSource& /*source*/, const Track& track, Track::Cover cover)
{
    QByteArray coverData;
    if(m_loadedReader.reader) {
        AudioSource coverSource;
        coverSource.filepath      = track.filepath();
        coverSource.device        = m_loadedReader.input.device.get();
        coverSource.archiveReader = m_loadedReader.input.archiveReader.get();
        coverData                 = m_loadedReader.reader->readCover(coverSource, track, cover);
    }
    if(coverData.isEmpty() && m_loadedReader.input.archiveReader
       && m_loadedReader.input.archiveReader->init(track.archivePath())) {
        coverData = m_loadedReader.input.archiveReader->readCover(track, cover);
    }
    return coverData;
}

bool GeneralArchiveReader::writeTrack(const AudioSource& /*source*/, const Track& track, WriteOptions options)
{
    if(!m_loadedReader.reader) {
        return false;
    }

    AudioSource trackSource;
    trackSource.filepath      = track.filepath();
    trackSource.device        = m_loadedReader.input.device.get();
    trackSource.archiveReader = m_loadedReader.input.archiveReader.get();
    return m_loadedReader.reader->writeTrack(trackSource, track, options);
}
} // namespace PicAudioEngine
