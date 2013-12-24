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
#ifndef MUSICSAVER_WAV_H
#define MUSICSAVER_WAV_H
#include "musicsaver.h"

class MusicSaver_Wav : public MusicSaver
{
    public:
        virtual bool save(const QString& filename, MusicData musicData, uint loop);
        virtual QString suffix() { return ".wav"; }
        static QString filterString() { return QObject::tr("Uncompressed PCM (*.wav)"); }
        static MusicSaver* createFunction() { return new MusicSaver_Wav(); }
};

#endif // MUSICSAVER_WAV_H