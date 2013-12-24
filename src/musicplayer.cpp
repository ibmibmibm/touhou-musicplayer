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
#include <portaudio.h>
#include <cstring>
#include "musicplayer.h"
#include "musicfile_ogg.h"
#include "musicfile_wav.h"
#include "musicplayer.h"
#include "helperfuncs.h"

enum _MusicPlayerError
{
    _FileNotFounded = 1,
    _UnknowFileFormat,
};

class _MusicPlayerImpl
{
    public:
        bool portaudioInitialized;
        PaError portaudioError;
        PaStream *stream;
        qreal volume;
        qreal targetVolume;
        size_t buffer_size;
        size_t buffer_max_size;
        char* buffer;
        MusicPlayer* hook;
        _MusicPlayerImpl() :
            portaudioInitialized(false),
            portaudioError(paNoError),
            stream(NULL),
            volume(1.0),
            targetVolume(1.0),
            buffer_size(0),
            buffer_max_size(16384),
            buffer(new char[buffer_max_size]),
            hook(NULL)
        {
            //qDebug() << Q_FUNC_INFO;
            PaError err = Pa_Initialize();
            if (err != paNoError)
            {
                portaudioError = err;
                return;
            }
            portaudioInitialized = true;
        }
        ~_MusicPlayerImpl()
        {
            //qDebug() << Q_FUNC_INFO;
            if (portaudioInitialized)
            {
                PaError err = Pa_Terminate();
                if (err == paNoError)
                    portaudioInitialized = false;
                else
                    portaudioError = err;
            }
            delete [] buffer;
        }
        int streamCallback(const void * /*inputBuffer*/, void *outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* /*timeInfo*/,
            PaStreamCallbackFlags /*statusFlags*/)
        {
            //qDebug() << Q_FUNC_INFO << "framesPerBuffer" << framesPerBuffer;
            hook->_fillBuffer(framesPerBuffer);
            //qDebug() << Q_FUNC_INFO << "buffer_size" << buffer_size;
            if (buffer_size <= 0)
            {
                memset(outputBuffer, 0, framesPerBuffer * hook->_file->blockwidth());
                return paComplete;
            }
            unsigned long samples = buffer_size / hook->_file->blockwidth();
            memcpy(outputBuffer, buffer, buffer_size);
            buffer_size = 0;
            qint16 *outBuffer = static_cast<qint16*>(outputBuffer);
            qreal fadeoutVolume;
            for (unsigned long i = 0; i < samples; ++i)
            {
                fadeoutVolume = hook->_fadeoutVolume(i);
                for (size_t j = 0; j < hook->_file->channels(); ++j)
                {
                    *outBuffer *= volume;
                    *outBuffer *= fadeoutVolume;
                    ++outBuffer;
                }
                if (volume == targetVolume)
                    continue;
                if (volume < targetVolume)
                {
                    if (volume <= targetVolume - 0.01)
                        volume += 0.01;
                    else
                        volume = targetVolume;
                }
                else if (volume > targetVolume)
                {
                    if (volume >= targetVolume + 0.01)
                        volume -= 0.01;
                    else
                        volume = targetVolume;
                }
            }
            return paContinue;
        }
        static int streamCallback(const void * inputBuffer, void *outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void * userData)
        {
            return static_cast<_MusicPlayerImpl*>(userData)->streamCallback(
                inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
        }
} _playerImpl;

void MusicBufferThread::run()
{
}

MusicPlayer::MusicPlayer() :
    _file(NULL),
    _samples(0),
    _totalSamples(0),
    _tickInterval(100),
    _fadeoutTime(10000)
{
    Q_ASSERT(_playerImpl.hook == NULL);
    _playerImpl.hook = this;
    connect(this, SIGNAL(finish()), this, SLOT(_next()));
    _setState(StoppedState);
}

MusicPlayer::~MusicPlayer()
{
    //qDebug() << Q_FUNC_INFO;
    stop();
    _unload();
    _playerImpl.hook = NULL;
}

void MusicPlayer::_fillBuffer(qint64 needSample)
{
    //qDebug() << Q_FUNC_INFO << needSample;
    if (_musicOver)
    {
        //qDebug() << Q_FUNC_INFO << "_musicOver";
        _playerImpl.buffer_size = 0;
        _setSamples(_totalSamples);
        emit finish();
        return;
    }
    qint64 realSamples = _samples;
    _samplesToLoop(realSamples);
    qint64 loopEnd = _file->loopEnd();
    Q_ASSERT(realSamples < loopEnd);
    qint64 targetSample = realSamples + needSample;
    _playerImpl.buffer_size = 0;
    qint64 getSamples = 0;
    if (loopEnd <= targetSample)
    {
        //qDebug() << Q_FUNC_INFO << "loop";
        getSamples = _file->sampleRead(_playerImpl.buffer, loopEnd - realSamples);
        needSample -= getSamples;
        _file->sampleSeek(_file->loopBegin());
    }
    getSamples += _file->sampleRead(_playerImpl.buffer + getSamples * _file->blockwidth(), needSample);
    //qDebug() << Q_FUNC_INFO << needSample << getSamples;
    _playerImpl.buffer_size = getSamples * _file->blockwidth();
    _setSamples(_samples + getSamples);
}

qreal MusicPlayer::_fadeoutVolume(qint64 offset)
{
    qint64 realSamples = _samples + offset;
    size_t loop = _samplesToLoop(realSamples);
    if (loop < _queue[0].loop)
        return 1.0;
    realSamples -= _file->loopBegin();
    if (realSamples > _fadeoutSamples)
    {
        _musicOver = true;
        return 0.0;
    }
    qreal fadeoutVolume = static_cast<qreal>(_fadeoutSamples - realSamples) / static_cast<qreal>(_fadeoutSamples);
    return fadeoutVolume * fadeoutVolume;
}

void MusicPlayer::_load()
{
    //qDebug() << Q_FUNC_INFO;
    if (!_file)
    {
        Q_ASSERT(_file == NULL);
        if (_queue[0].musicData.suffix() == ".ogg")
        {
            _file = new MusicFile_Ogg(_queue[0].musicData);
            if (!_file->open(QIODevice::ReadOnly))
            {
                delete _file;
                _file = NULL;
                return;
            }
            //qDebug() << Q_FUNC_INFO << "MusicFile_Ogg";
        }
        else if (_queue[0].musicData.suffix() == ".wav")
        {
            _file = new MusicFile_Wav(_queue[0].musicData);
            if (!_file->open(QIODevice::ReadOnly))
            {
                delete _file;
                _file = NULL;
                return;
            }
            //qDebug() << Q_FUNC_INFO << "MusicFile_Wav";
        }
        qint64 loopBegin =  _file->loopBegin();
        qint64 loopSize = _file->loopEnd() - loopBegin;
        _fadeoutSamples = _fadeoutTime * 44.1;
        qint64 totalSamples = loopBegin + loopSize * _queue[0].loop + _fadeoutSamples;
        _setTotalSamples(totalSamples);
        _file->seek(0);
    }
}

void MusicPlayer::_unload()
{
    //qDebug() << Q_FUNC_INFO;
    if (_file)
    {
        _file->close();
        delete _file;
        _file = NULL;
    }
    _setSamples(0);
}

void MusicPlayer::_next()
{
    //qDebug() << Q_FUNC_INFO;
    stop();
    _unload();
    _queue.removeFirst();
    if (_queue.size() == 0)
        return;
    _load();
    currentMusicChanged(_queue[0].musicData);
    play();
}

void MusicPlayer::play()
{
    //qDebug() << Q_FUNC_INFO;
    MusicPlayerState s = state();
    if (s != StoppedState && s != PausedState)
        return;
    if (_queue.size() == 0)
        return;
    //qDebug() << Q_FUNC_INFO << "FileName" << _file->fileName();
    _setState(BufferingState);
    const PaDeviceIndex deviceIndex = Pa_GetDefaultOutputDevice();
    //qDebug() << Q_FUNC_INFO << "Device name" << QString::fromLocal8Bit(Pa_GetDeviceInfo(deviceIndex)->name);
    PaStreamParameters outputparam;
    outputparam.device = deviceIndex;
    outputparam.channelCount = _file->channels();
    outputparam.sampleFormat = paInt16;
    outputparam.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultHighOutputLatency;
    outputparam.hostApiSpecificStreamInfo = NULL;
    PaError err = Pa_OpenStream(
            &_playerImpl.stream,
            NULL,
            &outputparam,
            44100,
            _playerImpl.buffer_max_size / _file->blockwidth(),
            paNoFlag,
            _MusicPlayerImpl::streamCallback,
            &_playerImpl);
    if (err != paNoError)
    {
        _playerImpl.portaudioError = err;
        _setState(ErrorState);
        return;
    }
    err = Pa_StartStream(_playerImpl.stream);
    if (err != paNoError)
    {
        _playerImpl.portaudioError = err;
        _setState(ErrorState);
        return;
    }
    _setState(PlayingState);
}

void MusicPlayer::pause()
{
    //qDebug() << Q_FUNC_INFO;
    if (state() != PlayingState)
        return;
    PaError err = Pa_CloseStream(_playerImpl.stream);
    if (err != paNoError)
    {
        _playerImpl.portaudioError = err;
        _setState(ErrorState);
        return;
    }
    _setState(PausedState);
}

void MusicPlayer::stop()
{
    //qDebug() << Q_FUNC_INFO;
    MusicPlayerState s = state();
    if (s != PlayingState && s != PausedState)
        return;
    PaError err = Pa_CloseStream(_playerImpl.stream);
    _file->seek(0);
    _setSamples(0);
    if (err != paNoError)
    {
        _playerImpl.portaudioError = err;
        _setState(ErrorState);
        return;
    }
    _setState(StoppedState);
}

QString MusicPlayer::errorString() const
{
    //qDebug() << Q_FUNC_INFO;
    if (_playerImpl.portaudioError < 0)
        return Pa_GetErrorText(_playerImpl.portaudioError);
    switch (_playerImpl.portaudioError)
    {
        case _FileNotFounded:
            return "File not founded.";
        case _UnknowFileFormat:
            return "Unable to identify file format.";
        default:
            return QString("Unknow error %1").arg(_playerImpl.portaudioError);
    }
}

MusicPlayerErrorType MusicPlayer::errorType() const
{
    //qDebug() << Q_FUNC_INFO;
    switch (_playerImpl.portaudioError)
    {
        case paNoError:
            return NoError;
        case paDeviceUnavailable:
        case _FileNotFounded:
        case _UnknowFileFormat:
            return NormalError;
        case paNotInitialized:
        case paUnanticipatedHostError:
        case paInvalidChannelCount:
        case paInvalidSampleRate:
        case paInvalidDevice:
        case paInvalidFlag:
        case paSampleFormatNotSupported:
        case paBadIODeviceCombination:
        case paInsufficientMemory:
        case paBufferTooBig:
        case paBufferTooSmall:
        case paNullCallback:
        case paBadStreamPtr:
        case paTimedOut:
        case paInternalError:
        case paIncompatibleHostApiSpecificStreamInfo:
        case paStreamIsStopped:
        case paStreamIsNotStopped:
        case paInputOverflowed:
        case paOutputUnderflowed:
        case paHostApiNotFound:
        case paInvalidHostApi:
        case paIncompatibleStreamHostApi:
        case paBadBufferPtr:
        default:
            return FatalError;
    }
}

void MusicPlayer::setCurrentMusic(MusicData musicData, int loop)
{
    //qDebug() << Q_FUNC_INFO;
    stop();
    _unload();
    _setState(LoadingState);
    _queue.clear();
    _queue << QueuedMusic(musicData, loop);
    _setLoop(0);
    _setSamples(0);
    _load();
    _setState(StoppedState);
    currentMusicChanged(musicData);
}

void MusicPlayer::enqueue(const MusicData& musicData, int loop)
{
    //qDebug() << Q_FUNC_INFO;
    _queue << QueuedMusic(musicData, loop);
}

void MusicPlayer::clearQueue()
{
    //qDebug() << Q_FUNC_INFO;
    stop();
    _queue.clear();
}

void MusicPlayer::seek(qint64 samples)
{
    qint64 realSamples = samples;
    _samplesToLoop(realSamples);
    qDebug() << Q_FUNC_INFO << samples << realSamples;
    _file->sampleSeek(realSamples);
    _setSamples(samples);
}


size_t MusicPlayer::_samplesToLoop(qint64& samples)
{
    if (_queue.size() == 0)
        return 0;
    int loopEnd = _file->loopEnd();
    int loopSize = loopEnd - _file->loopBegin();
    Q_ASSERT(loopSize > 0);
    int loop = 0;
    while (samples >= loopEnd)
    {
        ++loop;
        samples -= loopSize;
    }
    return loop;
}

void MusicPlayer::_setState(MusicPlayerState newState)
{
    if (_state == newState)
        return;
    //qDebug() << Q_FUNC_INFO << newState;
    MusicPlayerState oldState = _state;
    _state = newState;
    emit stateChanged(newState, oldState);
}

void MusicPlayer::_setLoop(uint newLoop)
{
    if (_loop == newLoop)
        return;
    //qDebug() << Q_FUNC_INFO << newLoop;
    _loop = newLoop;
    emit loopChanged(newLoop);
    if (_queue.size() > 0 && _loop == _queue[0].loop)
    {
        //qDebug() << Q_FUNC_INFO << "aboutToFinish";
        emit aboutToFinish();
    }
}

void MusicPlayer::_setSamples(qint64 newSamples)
{
    //qDebug() << Q_FUNC_INFO << newSamples;
    if (_samples == newSamples)
        return;
    qint64 realSamples = _samples = newSamples;
    int loop = _samplesToLoop(realSamples);
    _musicOver = (_samples >= _totalSamples);
    emit tick(newSamples);
    _setLoop(loop);
}

void MusicPlayer::_setTotalSamples(qint64 newTotalSamples)
{
    qDebug() << Q_FUNC_INFO << newTotalSamples;
    if (_totalSamples == newTotalSamples)
        return;
    _totalSamples = newTotalSamples;
    _musicOver = (_samples >= _totalSamples);
    emit totalSamplesChanged(newTotalSamples);
}

qreal MusicPlayer::volume() const
{
    //qDebug() << Q_FUNC_INFO;
    return _playerImpl.targetVolume;
}

void MusicPlayer::setVolume(qreal newVolume)
{
    //qDebug() << Q_FUNC_INFO;
    _playerImpl.targetVolume = newVolume;
}
