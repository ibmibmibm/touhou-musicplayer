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
#include "musicplayer.h"

MusicPlayer::MusicPlayer()
{
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    mediaObject = new Phonon::MediaObject(this);

    tickInterval = 200;
    mediaObject->setTickInterval(tickInterval);

    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(_tick(qint64)));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(_stateChanged(Phonon::State, Phonon::State)));
    connect(mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SLOT(_totalTimeChanged(qint64)));
    connect(mediaObject, SIGNAL(currentSourceChanged(const Phonon::MediaSource &)),
            this, SLOT(_currentSourceChanged(const Phonon::MediaSource &)));
    connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(_aboutToFinish()));
    connect(audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(_volumeChanged(qreal)));

    Phonon::createPath(mediaObject, audioOutput);
    fadeoutTime = 5000;
}

void MusicPlayer::setCurrentMusic(MusicFile musicFile, int loop)
{
    buffer.clear();
    loop_buffer.clear();
    buffer_hash.clear();
    buffer_hash.insert(musicFile.fileName(), musicFile);
    currentMusic = musicFile;
    totalLoop = loop;
    _setLoop(0);
    mediaObject->setCurrentSource(Phonon::MediaSource(musicFile.fileName()));
}

void MusicPlayer::enqueue(const MusicFile& musicFile, int loop)
{
    buffer << musicFile;
    loop_buffer << loop;
    buffer_hash.insert(musicFile.fileName(), musicFile);
    //mediaObject->enqueue(Phonon::MediaSource(musicFile.fileName()));
}

void MusicPlayer::clearQueue()
{
    buffer.clear();
    loop_buffer.clear();
    buffer_hash.clear();
    mediaObject->clearQueue();
}

void MusicPlayer::seek(int samples)
{
    int loopsize = currentMusic.loopEnd() - currentMusic.loopStart();
    int loop = samples - currentMusic.loopStart();
    if (loop < 0)
        loop = 0;
    _setLoop(loop / loopsize);
    mediaObject->seek((loop % loopsize + currentMusic.loopStart()) * 0.02267573696145124716553287982 + 0.5); // 1000/44100
}

void MusicPlayer::_setLoop(int newLoop)
{
    if (currentLoop != newLoop)
    {
        currentLoop = newLoop;
        emit remainLoopChanged(totalLoop - currentLoop);
    }
}

void MusicPlayer::_tick(qint64 time)
{
    int samples = time * 44.1;
    if (!currentMusic.isLoop())
    {
        emit tick(samples);
        return;
    }
    uint fadeoutSamples = fadeoutTime * 44.1;
    if (currentLoop < totalLoop)
    {
        if (static_cast<uint>(samples) >= currentMusic.loopEnd())
        {
            int diff = samples - currentMusic.loopEnd();
            mediaObject->seek((diff + currentMusic.loopStart()) * 0.02267573696145124716553287982 + 0.5); // 1000/44100
            _setLoop(currentLoop + 1);
            samples -= currentMusic.loopEnd() - currentMusic.loopStart();
        }
        goto out;
    }
    if (static_cast<uint>(samples) < fadeoutSamples + currentMusic.loopStart())
    {
        audioOutput->setVolume(static_cast<qreal>(fadeoutSamples + currentMusic.loopStart() - samples) / static_cast<qreal>(fadeoutSamples));
        goto out;
    }
    emit aboutToFinish();
    currentMusic = buffer.front();
    buffer.pop_front();
    totalLoop = loop_buffer.front();
    loop_buffer.pop_front();
    _setLoop(0);
    mediaObject->clearQueue();
    mediaObject->setCurrentSource(Phonon::MediaSource(currentMusic.fileName()));
    mediaObject->play();
    return;
out:
    int loopsize = currentMusic.loopEnd() - currentMusic.loopStart();
    emit tick(samples + loopsize * currentLoop);
}

void MusicPlayer::_totalTimeChanged(qint64 newTotalTime)
{
    if (newTotalTime <= 0)
        return;
    if (currentMusic.isLoop())
    {
        int loopsize = currentMusic.loopEnd() - currentMusic.loopStart();
        emit totalSamplesChanged(currentMusic.loopStart() + loopsize * totalLoop + fadeoutTime * 44.1);
    }
    else
        emit totalSamplesChanged(newTotalTime * 44.1);
}

void MusicPlayer::_stateChanged(Phonon::State newstate, Phonon::State oldstate)
{
    emit stateChanged(static_cast<MusicPlayerState>(newstate), static_cast<MusicPlayerState>(oldstate));
}

void MusicPlayer::_aboutToFinish()
{
    if (!currentMusic.isLoop() && currentLoop < totalLoop)
    {
        this->enqueue(currentMusic, currentLoop - 1);
        return;
    }
    if (currentMusic.isLoop() && currentLoop <= totalLoop)
        return;
    emit aboutToFinish();
}

void MusicPlayer::_currentSourceChanged(const Phonon::MediaSource& source)
{
    audioOutput->setVolume(1.0);
    MusicFile music = buffer_hash.value(source.fileName());
    int idx = buffer.indexOf(music);
    buffer.removeAt(idx);
    loop_buffer.removeAt(idx);
    emit currentMusicChanged(music);
}

void MusicPlayer::_volumeChanged(qreal newVolume)
{
    emit volumeChanged(newVolume * 128);
}

