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
#include <cmath>
#include <QSettings>
#include <QtDebug>
#include "loopmusicfile.h"

LoopMusicFile::LoopMusicFile(const MusicData& musicData, uint totalLoop) :
    _totalLoop(totalLoop)
{
    //qDebug() << Q_FUNC_INFO;
    _musicFile = MusicFileFactory::createMusicFile(musicData);
    Q_ASSERT(_musicFile->pos() == 0);
    QSettings settings;
    settings.beginGroup("Playback");
    _fadeoutTime = settings.value("Fadeout Time", 10000U).toUInt();
    settings.endGroup();
}

bool LoopMusicFile::open(MusicFile::OpenMode mode)
{
    //qDebug() << Q_FUNC_INFO;
    if (_musicFile == NULL)
        return false;
    if (!_musicFile->open(mode))
        return false;

    qint64 loopBegin = _musicFile->loopBegin();
    qint64 loopEnd = _musicFile->loopEnd();
    qint64 loopSize = loopEnd - loopBegin;

    _fadeoutSamples = static_cast<qint64>(_fadeoutTime * _musicFile->samplerate()) * 0.001;
    if (_fadeoutSamples > loopSize)
    {
        _fadeoutSamples = loopSize;
        //setErrorString(QObject::tr("Fadeout time is too long."));
        //return false;
    }
    _totalSamples = loopBegin + loopSize * _totalLoop + _fadeoutSamples;
    return true;
}

bool LoopMusicFile::sampleSeek(qint64 pos)
{
    //qDebug() << Q_FUNC_INFO;
    _setSamplesAndLoop(pos);
    _samplesToLoop(pos);
    return _musicFile->sampleSeek(pos);
}

qint64 LoopMusicFile::sampleRead(char* buffer, qint64 needSample)
{
    //qDebug() << Q_FUNC_INFO;
    if (_samples >= _totalSamples)
        return 0;
    Q_ASSERT(_musicFile->samplePos() < _musicFile->loopEnd());
    needSample = qMin(needSample, _totalSamples - _samples);
    //qDebug() << Q_FUNC_INFO << "needSample" << needSample;
    qint64 getSamples = 0;
    if (_musicFile->samplePos() >= _musicFile->loopEnd() - needSample)
    {
        //qDebug() << Q_FUNC_INFO << _musicFile->samplePos() << _musicFile->loopEnd() << needSample;
        //qDebug() << Q_FUNC_INFO << "loop" << _musicFile->loopEnd() - _musicFile->samplePos();
        getSamples = _musicFile->sampleRead(buffer, _musicFile->loopEnd() - _musicFile->samplePos());
        //qDebug() << Q_FUNC_INFO << needSample << getSamples;
        if (getSamples == -1)
            return -1;
        _musicFile->sampleSeek(_musicFile->loopBegin());
        needSample -= getSamples;
        //qDebug() << Q_FUNC_INFO << "getSamples" << getSamples;
    }
    {
        qint64 getSamples2 = _musicFile->sampleRead(buffer + getSamples * blockwidth(), needSample);
        if (getSamples2 == -1)
            return -1;
        getSamples += getSamples2;
        //qDebug() << Q_FUNC_INFO << "getSamples2" << getSamples2;
    }
    const qint64 normalSamples = (_totalSamples - _fadeoutSamples);
    if (_samples + getSamples >= normalSamples)
    {
        //qDebug() << Q_FUNC_INFO << "fadeout";
        qint16* bufferSample = reinterpret_cast<qint16*>(buffer);
        qint16* bufferSampleEnd = bufferSample + getSamples * _musicFile->channels();
        qint64 i = qMax(Q_INT64_C(0), normalSamples - _samples);
        bufferSample += i * _musicFile->channels();
        while (bufferSample < bufferSampleEnd)
        {
            qreal factor = static_cast<qreal>(_totalSamples - _samples - i) / static_cast<qreal>(_fadeoutSamples);
            Q_ASSERT(factor >= 0.0);
            //factor = cos(factor * 1.5707963267948966192313216916397514);
            factor = std::sqrt(factor);
            //qDebug() << Q_FUNC_INFO << factor << i;
            for (uint j = 0; j < _musicFile->channels(); ++j)
            {
                *bufferSample *= factor;
                ++bufferSample;
            }
            ++i;
        }
    }
    _setSamplesAndLoop(_samples + getSamples);
    return getSamples;
}

QByteArray LoopMusicFile::sampleRead(qint64 maxSample)
{
    //qDebug() << Q_FUNC_INFO;
    QByteArray ret(maxSample * blockwidth(), '\0');
    qint64 sample = sampleRead(ret.data(), maxSample);
    if (sample == -1)
        sample = 0;
    ret.resize(sample * blockwidth());
    return ret;
}

uint LoopMusicFile::_samplesToLoop(qint64& samples)
{
    //qDebug() << Q_FUNC_INFO;
    int loopEnd = _musicFile->loopEnd();
    int loopSize = loopEnd - _musicFile->loopBegin();
    Q_ASSERT(loopSize > 0);
    uint loop = 0;
    while (samples >= loopEnd)
    {
        ++loop;
        samples -= loopSize;
    }
    return loop;
}

void LoopMusicFile::_setSamplesAndLoop(qint64 newSamples)
{
    //qDebug() << Q_FUNC_INFO << newSamples;
    qint64 realSamples = _samples = newSamples;
    _loop = _samplesToLoop(realSamples);
}
