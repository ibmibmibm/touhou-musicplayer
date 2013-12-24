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

QT          += testlib
TEMPLATE     = app
TARGET       =
INCLUDEPATH += ../../include
HEADERS     += ../../include/musicfile.h \
               ../../include/musicfile_ogg.h
SOURCES     += testmusicfile_ogg.cpp \
               ../../src/musicfile.cpp \
               ../../src/musicfile_ogg.cpp

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
    PKGCONFIG += portaudio-2.0 vorbisfile
}

