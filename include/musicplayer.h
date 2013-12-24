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
#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QThread>
#include <QByteArray>

#include "musicfile.h"

enum MusicPlayerState
{
    LoadingState,
    StoppedState,
    PlayingState,
    BufferingState,
    PausedState,
    ErrorState,
};

enum MusicPlayerErrorType
{
    NoError,
    NormalError,
    FatalError,
};

struct QueuedMusic
{
    QueuedMusic(const MusicData& m, uint l):musicData(m),loop(l){}
    MusicData musicData;
    uint loop;
};

class _MusicPlayerImpl;

class MusicPlayer: public QObject
{
    Q_OBJECT

    public:
        friend class _MusicPlayerImpl;
        MusicPlayer();
        ~MusicPlayer();
        void setCurrentMusic(MusicData musicData, int loop);
        void enqueue(const MusicData & musicData, int loop);
        void clearQueue();
        MusicPlayerState state() const { return _state; }
        MusicPlayerErrorType errorType() const;
        QString errorString() const;
        qreal volume() const;
        qint64 totalSamples() const { return _totalSamples; }
        uint loop() const { return _loop; }
        uint totalLoop() const { if (_queue.isEmpty()) return 0; return _queue[0].loop; }
        uint remainLoop() const { return totalLoop() - loop(); }
    signals:
        void tick(qint64 samples);
        void stateChanged(MusicPlayerState newstate, MusicPlayerState oldstate);
        void totalSamplesChanged(qint64 newTotalSamples);
        void aboutToFinish();
        void finish();
        void currentMusicChanged(const MusicData& m);
        void volumeChanged(qreal newVolume);
        void loopChanged(uint newLoop);
    public slots:
        void play();
        void pause();
        void stop();
        void seek(qint64 samples);
        void setVolume(qreal newVolume);
        void setTotalLoop(uint newTotalLoop) { _queue[0].loop = newTotalLoop; }
    private slots:
        void _next();
    private:
        void _load();
        void _unload();
        void _setLoop(uint newLoop);
        void _setSamples(qint64 newSamples);
        void _setTotalSamples(qint64 newTotalSamples);
        void _setState(MusicPlayerState newState);
        void _fillBuffer(qint64 needSize);
        qreal _fadeoutVolume(qint64 offset);
        size_t _samplesToLoop(qint64& samples);

        QList<QueuedMusic> _queue;
        MusicFile* _file;
        qint64 _samples;
        qint64 _totalSamples;
        MusicPlayerState _state;
        uint _loop;
        uint _tickInterval;
        uint _fadeoutTime;
        qint64 _fadeoutSamples;
        bool _musicOver;
};

class MusicBufferThread : QThread
{
    Q_OBJECT
    protected:
        void run();
};

#endif // MUSICPLAYER_H
