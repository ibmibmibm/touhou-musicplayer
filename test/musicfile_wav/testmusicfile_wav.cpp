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
#include <QtTest/QtTest>
#include "musicfile_wav.h"
class TestMusicFileWav : public QObject
{
    Q_OBJECT
    private slots:
        void initTestCase();
        void open();
        void size();
        void seek();
        void read();
        void cleanupTestCase();
    private:
        MusicFile* _musicFile;
};

void TestMusicFileWav::initTestCase()
{
    QCOMPARE(MusicFileFactory::registerMusicFile(".wav", MusicFile_Wav::createFunction), 1);
}

void TestMusicFileWav::size()
{
    QCOMPARE(_musicFile->sampleSize(), Q_INT64_C(2));
}

void TestMusicFileWav::open()
{
    MusicData musicData("test.wav", "title", "artist", "album", 1, 2, ".wav", 52, true, 0, 1);
    _musicFile = MusicFileFactory::createMusicFile(musicData);
    QVERIFY(_musicFile);
    QVERIFY(_musicFile->open(MusicFile::ReadOnly));
    QCOMPARE(_musicFile->fileName(), QString("test.wav"));
    QCOMPARE(_musicFile->title(), QString("title"));
    QCOMPARE(_musicFile->album(), QString("album"));
    QCOMPARE(_musicFile->loopBegin(), Q_INT64_C(0));
    QCOMPARE(_musicFile->loopEnd(), Q_INT64_C(1));
    QVERIFY(_musicFile->channels() == 2);
    QVERIFY(_musicFile->samplerate() == 44100);
    QVERIFY(_musicFile->bytewidth() == 2);
    QVERIFY(_musicFile->blockwidth() == 4);
    QCOMPARE(_musicFile->pos(), Q_INT64_C(0));
}

void TestMusicFileWav::read()
{
    qint16 buffer[4];
    QVERIFY(_musicFile->sampleSeek(0));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 2), Q_INT64_C(2));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(2));
    QCOMPARE(buffer[0], static_cast<qint16>(32767));
    QCOMPARE(buffer[1], static_cast<qint16>(32767));
    QCOMPARE(buffer[2], static_cast<qint16>(-32768));
    QCOMPARE(buffer[3], static_cast<qint16>(-32768));
}

void TestMusicFileWav::seek()
{
    qint16 buffer[2];
    QVERIFY(_musicFile->sampleSeek(1));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(1));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(1));
    QCOMPARE(buffer[0], static_cast<qint16>(-32768));
    QCOMPARE(buffer[1], static_cast<qint16>(-32768));
    QVERIFY(_musicFile->sampleSeek(2));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(2));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(0));
    QVERIFY(_musicFile->sampleSeek(0));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(0));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(1));
    QCOMPARE(buffer[0], static_cast<qint16>(32767));
    QCOMPARE(buffer[1], static_cast<qint16>(32767));
}

void TestMusicFileWav::cleanupTestCase()
{
    delete _musicFile;
}

QTEST_MAIN(TestMusicFileWav)
#include "testmusicfile_wav.moc"

