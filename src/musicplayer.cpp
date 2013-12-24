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
#include <QtDebug>
#include <QSettings>
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
        MusicPlayer* hook;
        _MusicPlayerImpl() :
            portaudioInitialized(false),
            portaudioError(paNoError),
            stream(NULL),
            volume(1.0),
            targetVolume(1.0),
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
        }
        int streamCallback(const void * /*inputBuffer*/, void *outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* /*timeInfo*/,
            PaStreamCallbackFlags /*statusFlags*/)
        {
            //qDebug() << Q_FUNC_INFO << "framesPerBuffer" << framesPerBuffer;
            Q_ASSERT(hook->_file != NULL);
            memset(outputBuffer, 0, framesPerBuffer * hook->_file->blockwidth());
            size_t bufferSamples = hook->_file->sampleRead(static_cast<char*>(outputBuffer), framesPerBuffer);
            //qDebug() << Q_FUNC_INFO << "bufferSize" << bufferSize;
            if (bufferSamples == 0)
                return paComplete;
            qint16 *outBuffer = static_cast<qint16*>(outputBuffer);
            if (volume == 1.0 && targetVolume == 1.0)
                return paContinue;
            for (unsigned long i = 0; i < bufferSamples; ++i)
            {
                for (size_t j = 0; j < hook->_file->channels(); ++j)
                {
                    *outBuffer *= volume;
                    ++outBuffer;
                }
                if (volume == targetVolume)
                    continue;
                if (volume < targetVolume)
                {
                    if (volume <= targetVolume - 0.0078125)
                        volume += 0.0078125;
                    else
                        volume = targetVolume;
                }
                else if (volume > targetVolume)
                {
                    if (volume >= targetVolume + 0.0078125)
                        volume -= 0.0078125;
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

MusicPlayer::MusicPlayer() :
    _file(NULL),
    _tickInterval(100)
{
    Q_ASSERT(_playerImpl.hook == NULL);
    _playerImpl.hook = this;
    connect(this, SIGNAL(finish()), this, SLOT(_next()));
    connect(&_timer, SIGNAL(timeout()), this, SLOT(_tick()));
    _setState(StoppedState);
}

MusicPlayer::~MusicPlayer()
{
    //qDebug() << Q_FUNC_INFO;
    pause();
    _unload();
    _playerImpl.hook = NULL;
}

void MusicPlayer::_load()
{
    //qDebug() << Q_FUNC_INFO;
    if (!_file)
    {
        _file = new ThreadMusicFile(_queue.first().musicData, _queue.first().loop);
        if (_file == NULL)
            return;
        if (!_file->open(QIODevice::ReadOnly))
        {
            delete _file;
            _file = NULL;
            return;
        }
        _loop = _file->loop();
        emit totalSamplesChanged(_file->sampleSize());
        emit loopChanged(_loop);
        _emitAboutToFinish = false;
    }
}

void MusicPlayer::_unload()
{
    //qDebug() << Q_FUNC_INFO;
    if (_file)
    {
        delete _file;
        _file = NULL;
    }
}

void MusicPlayer::_next()
{
    //qDebug() << Q_FUNC_INFO;
    pause();
    _unload();
    _queue.removeFirst();
    if (_queue.size() == 0)
        return;
    _load();
    play();
    currentMusicChanged(_queue.first().musicData);
}

void MusicPlayer::play()
{
    MusicPlayerState s = state();
    if (s != StoppedState && s != PausedState)
        return;
    if (_queue.size() == 0)
        return;
    //qDebug() << Q_FUNC_INFO;
    //qDebug() << Q_FUNC_INFO << "FileName" << _file->fileName();
    _setState(BufferingState);
    PaDeviceIndex deviceIndex;
    {
        QSettings settings;
        settings.beginGroup("Playback");
        deviceIndex = settings.value("Output Device", Pa_GetDefaultOutputDevice()).toInt();
        settings.endGroup();
    }
    //qDebug() << Q_FUNC_INFO << "Device name" << QString::fromLocal8Bit(Pa_GetDeviceInfo(deviceIndex)->name);
    if (!_file)
        return;
//    Q_ASSERT(_file != NULL);
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
            _file->samplerate(),
            0,
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
    _timer.start(1000/30);
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
    _timer.stop();
}

void MusicPlayer::stop()
{
    //qDebug() << Q_FUNC_INFO;
    MusicPlayerState s = state();
    if (s != PlayingState && s != PausedState)
        return;
    if (s == PlayingState)
    {
        PaError err = Pa_CloseStream(_playerImpl.stream);
        if (err != paNoError)
        {
            _playerImpl.portaudioError = err;
            _setState(ErrorState);
            return;
        }
    }
    _file->sampleSeek(0);
    _setState(StoppedState);
    _timer.stop();
}

void MusicPlayer::seek(qint64 samples)
{
    //qDebug() << Q_FUNC_INFO;
    MusicPlayerState s = state();
    if (s == PlayingState || s == PausedState || s == StoppedState)
        _file->sampleSeek(samples);
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
    pause();
    _unload();
    _setState(LoadingState);
    _queue.clear();
    _queue << QueuedMusic(musicData, loop);
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
    pause();
    _queue.clear();
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

void MusicPlayer::_tick()
{
    if (_file == NULL)
        return;
    qint64 samplePos = _file->samplePos();
    //qDebug() << Q_FUNC_INFO << _file->bufferSize();
    qint64 remainSample = _file->sampleSize() - samplePos;
    if (remainSample <= 1024 && !_emitAboutToFinish)
    {
        _emitAboutToFinish = true;
        emit aboutToFinish();
    }
    emit tick(samplePos);
    uint loop = _file->loop();
    if (_loop != loop)
    {
        _loop = loop;
        emit loopChanged(_loop);
    }
    // _file maybe NULL after emit finish, so we do it finally.
    if (remainSample == 0)
    {
        emit finish();
    }
}

qreal MusicPlayer::volume() const
{
    //qDebug() << Q_FUNC_INFO;
    return _playerImpl.targetVolume;
}

void MusicPlayer::setVolume(qreal newVolume)
{
    _playerImpl.targetVolume = newVolume;
}

int MusicPlayer::deviceCount() const
{
    return Pa_GetDeviceCount();
}

int MusicPlayer::defaultDevice() const
{
    return Pa_GetDefaultOutputDevice();
}

QString MusicPlayer::device(int id) const
{
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(id);
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    return QString("%1 (%2)").arg(QString::fromLocal8Bit(deviceInfo->name)).arg(QString::fromLocal8Bit(apiInfo->name));
}
