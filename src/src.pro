# This file is part of Touhou Music Player.
#
# Touhou Music Player is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Touhou Music Player is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Touhou Music Player.  If not, see <http://www.gnu.org/licenses/>.

TARGET        = touhou-musicplayer
DESTDIR       = ..
CONFIG       += debug_and_release
INCLUDEPATH  += ../include
HEADERS      += ../include/mainwindow.h \
                ../include/configdialog.h \
                ../include/pluginloader.h \
                ../include/musicplayer.h \
                ../include/playlistmodel.h \
                ../include/spinboxdelegate.h \
                ../include/musicsaver.h \
                ../include/musicsaver_wav.h \
                ../include/musicsaver_flac.h \
                ../include/musicfile.h \
                ../include/musicfile_wav.h \
                ../include/musicfile_ogg.h \
                ../include/loopmusicfile.h \
                ../include/threadmusicfile.h \
                ../include/musicdata.h \
                ../include/loaderinterface.h
SOURCES      += main.cpp \
                mainwindow.cpp \
                pluginloader.cpp \
                musicplayer.cpp \
                playlistmodel.cpp \
                spinboxdelegate.cpp \
                musicsaver.cpp \
                musicsaver_wav.cpp \
                musicsaver_flac.cpp \
                musicfile.cpp \
                musicfile_wav.cpp \
                musicfile_ogg.cpp \
                loopmusicfile.cpp \
                threadmusicfile.cpp \
                configdialog.cpp

TRANSLATIONS = ../translations/touhou_musicplayer_zh_TW.ts

win32 {
    LIBS        += -LC:\portaudio\lib -lportaudio_x86 -LC:\libogg\lib -logg -lvorbis -lvorbisfile
    INCLUDEPATH += C:\portaudio\include C:\libogg\include
    CONFIG(debug, debug|release) {
        CONFIG += console
    }
}

unix {
    #QMAKE_CXXFLAGS += -pg
    #QMAKE_LFLAGS += -pg
    CONFIG    += link_pkgconfig
    PKGCONFIG += portaudio-2.0 vorbisfile flac
}

