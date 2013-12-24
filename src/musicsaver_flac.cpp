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
#include <QFile>
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>
#include "musicsaver_flac.h"
#include "loopmusicfile.h"

namespace {
    FLAC__StreamEncoderWriteStatus _write(const FLAC__StreamEncoder* /*encoder*/,
                                          const FLAC__byte buffer[],
                                          size_t bytes, unsigned /*samples*/,
                                          unsigned /*current_frame*/, void *client_data)
    {
        QFile* file = reinterpret_cast<QFile*>(client_data);
        if (file->write(reinterpret_cast<const char*>(buffer), bytes))
            return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }

    FLAC__StreamEncoderSeekStatus _seek(const FLAC__StreamEncoder* /*encoder*/,
                                        FLAC__uint64 absolute_byte_offset,
                                        void *client_data)
    {
        QFile* file = reinterpret_cast<QFile*>(client_data);
        if (file->seek(absolute_byte_offset))
            return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
        return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    }

    FLAC__StreamEncoderTellStatus _tell(const FLAC__StreamEncoder* /*encoder*/,
                                        FLAC__uint64 *absolute_byte_offset,
                                        void *client_data)
    {
        QFile* file = reinterpret_cast<QFile*>(client_data);
        *absolute_byte_offset = file->pos();
        return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }
}

bool MusicSaver_Flac::save(const QString& filename, MusicData musicData, uint loop)
{
    //qDebug() << Q_FUNC_INFO;

    LoopMusicFile musicFile(musicData, loop);

    if (!musicFile.open(QIODevice::ReadOnly))
    {
        setErrorString(musicFile.errorString());
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        setErrorString(file.errorString());
        return false;
    }

    quint64 totalSize = musicFile.sampleSize() * musicFile.blockwidth();
    if (totalSize > (Q_UINT64_C(4294967295) - Q_UINT64_C(36)))
    {
        setErrorString(QObject::tr("Repeat value is too large."));
        return false;
    }

    struct StreamEncoder
    {
        FLAC__StreamEncoder *v;
        StreamEncoder(): v(FLAC__stream_encoder_new()) {}
        ~StreamEncoder() { FLAC__stream_encoder_delete(v); }
    } encoder;

    if (!encoder.v)
        return false;

    if (!FLAC__stream_encoder_set_verify(encoder.v, true) ||
        !FLAC__stream_encoder_set_compression_level(encoder.v, 1) ||
        !FLAC__stream_encoder_set_channels(encoder.v, musicFile.channels()) ||
        !FLAC__stream_encoder_set_bits_per_sample(encoder.v, musicFile.bytewidth() << 3) ||
        !FLAC__stream_encoder_set_sample_rate(encoder.v, musicFile.samplerate()) ||
        !FLAC__stream_encoder_set_total_samples_estimate(encoder.v, musicFile.sampleSize()))
    {
        return false;
    }

    FLAC__StreamMetadata_VorbisComment_Entry entry;

    struct MetaData
    {
        FLAC__StreamMetadata *v;
        MetaData(): v(FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {}
        ~MetaData() { FLAC__metadata_object_delete(v); }
    } metadata;

    if (metadata.v == NULL)
    {
        return false;
    }

    if (!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", musicData.title().toUtf8().data()) ||
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata.v, entry, false) ||
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", musicData.album().toUtf8().data()) ||
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata.v, entry, false) ||
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", musicData.artist().toUtf8().data()) ||
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata.v, entry, false) ||
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", QString::number(musicData.trackNumber()).toUtf8().data()) ||
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata.v, entry, false) ||
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKTOTAL", QString::number(musicData.totalTrackNumber()).toUtf8().data()) ||
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata.v, entry, false) ||
        !FLAC__stream_encoder_set_metadata(encoder.v, &metadata.v, 1))
    {
        return false;
    }

    FLAC__StreamEncoderInitStatus init_status = FLAC__stream_encoder_init_stream(encoder.v, _write, _seek, _tell, NULL, &file);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
        //qDebug() << Q_FUNC_INFO << FLAC__StreamEncoderInitStatusString[init_status];
        return false;
    }

    const qint64 bufferSample = 65536;

    struct PCM
    {
        FLAC__int32 *v;
        PCM(size_t size): v(new FLAC__int32[size]) {}
        ~PCM() { delete [] v; }
    } pcm(bufferSample * musicFile.channels());

    if (pcm.v == NULL)
        return false;

    struct Buffer
    {
        char *v;
        Buffer(size_t size): v(new char[size]) {}
        ~Buffer() { delete [] v; }
    } buffer(bufferSample * musicFile.blockwidth());

    //qDebug() << Q_FUNC_INFO << "bufferSample * musicFile.blockwidth() = " << bufferSample * musicFile.blockwidth();

    if (buffer.v == NULL)
        return false;

    musicFile.sampleSeek(0);
    size_t need;
    qint16 *outBuffer = reinterpret_cast<qint16*>(buffer.v);
    while (need = musicFile.sampleRead(buffer.v, bufferSample), need > 0)
    {
        for (size_t i = 0; i < need * musicFile.channels(); ++i)
        {
            pcm.v[i] = outBuffer[i];
        }
        if (!FLAC__stream_encoder_process_interleaved(encoder.v, pcm.v, need))
        {
            return false;
        }
    }
    FLAC__stream_encoder_finish(encoder.v);
    return true;
}
