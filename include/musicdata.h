/**
 * This file is part of Touhou Music Player.
 *
 * Touhou Music Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Touhou Music Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Touhou Music Player.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MUSICDATA_H
#define MUSICDATA_H
#include <QSharedData>
#include <Q_INT64>
#include <QString>

struct ArchiveMusicData : public QSharedData
{
    typedef char (*Filter)(void *, char);
    ArchiveMusicData(
        const QString& archiveFileName_,
        qint64 dataBegin_,
        qint64 dataEnd_,
        Filter encoder_ = NULL,
        Filter decoder_ = NULL,
        void* userData_ = NULL
    ) :
        archiveFileName(archiveFileName_),
        dataBegin(dataBegin_),
        dataEnd(dataEnd_),
        encoder(encoder_),
        decoder(decoder_),
        userData(userData_)
    {
        Q_ASSERT(dataBegin_ <= dataEnd_);
    }
    QString archiveFileName;
    qint64 dataBegin;
    qint64 dataEnd;
    Filter encoder;
    Filter decoder;
    void* userData;
};

class MusicData
{
    public:
        MusicData() :
            _null(true)
        {
        }
        MusicData(
            const QString& fileName,
            const QString& title,
            const QString& artist,
            const QString& album,
            uint trackNumber,
            uint totalTrackNumber,
            const QString& suffix,
            qint64 size,
            bool loop,
            qint64 loopBegin,
            qint64 loopEnd,
            const ArchiveMusicData* archiveMusicData = NULL
        ) :
            _null(false),
            _fileName(fileName),
            _title(title),
            _artist(artist),
            _album(album),
            _trackNumber(trackNumber),
            _totalTrackNumber(totalTrackNumber),
            _suffix(suffix),
            _size(size),
            _loop(loop),
            _loopBegin(loopBegin),
            _loopEnd(loopEnd),
            _archiveMusicData(NULL)
        {
            Q_ASSERT(loopBegin <= loopEnd);
            if (archiveMusicData != NULL)
            {
                _archiveMusicData = new ArchiveMusicData(*archiveMusicData);
            }
        }

        bool isNull() const { return _null; }
        const QString& fileName() const { return _fileName; }
        void setFileName(const QString& newFileName) { _fileName = newFileName; }
        const QString& title() const { return _title; }
        const QString& artist() const { return _artist; }
        const QString& album() const { return _album; }
        const uint& trackNumber() const { return _trackNumber; }
        const uint& totalTrackNumber() const { return _totalTrackNumber; }
        const QString& suffix() const { return _suffix; }
        qint64 size() const { return _size; }
        bool loop() const { return _loop; }
        qint64 loopBegin() const { return _loopBegin; }
        qint64 loopEnd() const { return _loopEnd; }
        const QExplicitlySharedDataPointer<ArchiveMusicData>& archiveMusicData() const { return _archiveMusicData; }
        friend bool operator==(const MusicData& left, const MusicData& right);
    private:
        bool _null;
        QString _fileName;
        QString _title;
        QString _artist;
        QString _album;
        uint _trackNumber;
        uint _totalTrackNumber;
        QString _suffix;
        qint64 _size;
        bool _loop;
        qint64 _loopBegin;
        qint64 _loopEnd;
        QExplicitlySharedDataPointer<ArchiveMusicData> _archiveMusicData;
};

inline bool operator==(const MusicData& left, const MusicData& right)
{
    return left._archiveMusicData == right._archiveMusicData
        && left._fileName == right._fileName
        && left._title == right._title
        && left._album == right._album;
}

#endif // MUSICDATA_H
