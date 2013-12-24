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
#include <QByteArray>
#include <QTimer>

#include "threadmusicfile.h"

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
        qint64 samples() const { if (_file == NULL) return 0; return _file->samplePos(); }
        qint64 totalSamples() const { if (_file == NULL) return 0; return _file->sampleSize(); }
        uint loop() const { if (_file == NULL) return 0; return _file->loop(); }
        uint totalLoop() const { if (_file == NULL) return 0; return _file->totalLoop(); }
        uint remainLoop() const { return totalLoop() - loop(); }

        int deviceCount() const;
        int defaultDevice() const;
        QString device(int id) const;
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
    private slots:
        void _next();
        void _tick();
        void _loopChanged(uint newLoop) { emit loopChanged(newLoop); }
    private:
        void _load();
        void _unload();
        void _setState(MusicPlayerState newState);

        QList<QueuedMusic> _queue;
        ThreadMusicFile* _file;
        MusicPlayerState _state;
        QTimer _timer;
        uint _loop;
        uint _tickInterval;
        bool _musicOver;
        bool _emitAboutToFinish;
};

#endif // MUSICPLAYER_H
