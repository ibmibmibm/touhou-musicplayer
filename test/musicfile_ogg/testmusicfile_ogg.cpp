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
#include "musicfile_ogg.h"
class TestMusicFileOgg : public QObject
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

void TestMusicFileOgg::initTestCase()
{
    QCOMPARE(MusicFileFactory::registerMusicFile(".ogg", MusicFile_Ogg::createFunction), 1);
}

void TestMusicFileOgg::size()
{
    QCOMPARE(_musicFile->sampleSize(), Q_INT64_C(2));
}

void TestMusicFileOgg::open()
{
    MusicData musicData("test.ogg", "title", "artist", "album", 1, 2, ".ogg", 52, true, 0, 1);
    _musicFile = MusicFileFactory::createMusicFile(musicData);
    QVERIFY(_musicFile);
    QVERIFY(_musicFile->open(MusicFile::ReadOnly));
    QCOMPARE(_musicFile->fileName(), QString("test.ogg"));
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

void TestMusicFileOgg::read()
{
    qint16 buffer[4];
    QVERIFY(_musicFile->sampleSeek(0));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 2), Q_INT64_C(2));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(2));
    QCOMPARE(buffer[0], static_cast<qint16>(17458));
    QCOMPARE(buffer[1], static_cast<qint16>(17458));
    QCOMPARE(buffer[2], static_cast<qint16>(-16828));
    QCOMPARE(buffer[3], static_cast<qint16>(-16828));
}

void TestMusicFileOgg::seek()
{
    qint16 buffer[2];
    QVERIFY(_musicFile->sampleSeek(1));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(1));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(1));
    QCOMPARE(buffer[0], static_cast<qint16>(-16828));
    QCOMPARE(buffer[1], static_cast<qint16>(-16828));
    QVERIFY(_musicFile->sampleSeek(2));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(2));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(0));
    QVERIFY(_musicFile->sampleSeek(0));
    QCOMPARE(_musicFile->samplePos(), Q_INT64_C(0));
    QCOMPARE(_musicFile->sampleRead(reinterpret_cast<char*>(buffer), 1), Q_INT64_C(1));
    QCOMPARE(buffer[0], static_cast<qint16>(17458));
    QCOMPARE(buffer[1], static_cast<qint16>(17458));
}

void TestMusicFileOgg::cleanupTestCase()
{
    delete _musicFile;
}

QTEST_MAIN(TestMusicFileOgg)
#include "testmusicfile_ogg.moc"

