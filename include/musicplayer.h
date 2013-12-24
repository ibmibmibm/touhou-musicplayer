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

#include <QObject>
#include <phonon/phononnamespace.h>
#include <phonon/audiooutput.h>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>

#include <portaudio.h>

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

class MusicPlayer : public QObject
{
    Q_OBJECT

    public:
        MusicPlayer();
        void setCurrentMusic(MusicFile musicFile, int loop);
        void enqueue(const MusicFile & musicFile, int loop);
        void clearQueue();
        MusicPlayerState state() const { return static_cast<MusicPlayerState>(mediaObject->state()); }
        MusicPlayerErrorType errorType() const { return static_cast<MusicPlayerErrorType>(mediaObject->errorType()); }
        QString errorString() const { return mediaObject->errorString(); }
        int volume() const { return audioOutput->volume() * 128; }
        qint64 totalTime() const { return mediaObject->totalTime(); }
        int remainLoop() const { return totalLoop - currentLoop; }
    signals:
        void tick(int samples);
        void stateChanged(MusicPlayerState newstate, MusicPlayerState oldstate);
        void totalSamplesChanged(int newTotalSamples);
        void aboutToFinish();
        void currentMusicChanged(const MusicFile& m);
        void volumeChanged(int newVolume);
        void remainLoopChanged(int newRemainLoop);
    public slots:
        void play() { mediaObject->play(); }
        void pause() { mediaObject->pause(); }
        void stop() { mediaObject->stop(); }
        void seek(int samples);
        void setVolume(int newVolume) { audioOutput->setVolume(newVolume * 0.0078125); } // 1/128
    private slots:
        void _setLoop(int newLoop);
        void _tick(qint64 time);
        void _stateChanged(Phonon::State newstate, Phonon::State oldstate);
        void _aboutToFinish();
        void _currentSourceChanged(const Phonon::MediaSource& source);
        void _totalTimeChanged(qint64 newTotalTime);
        void _volumeChanged(qreal newVolume);
    private:
        Phonon::MediaObject *mediaObject;
        Phonon::AudioOutput *audioOutput;
        QHash<QString, MusicFile> buffer_hash;
        QList<MusicFile> buffer;
        QList<int> loop_buffer;
        MusicFile currentMusic;
        int tickInterval;
        int currentLoop;
        int totalLoop;
        int fadeoutTime;
};

#endif // MUSICPLAYER_H
