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
win32 {
    TOOLS_TREE = G:/QT/4.4.1/bin/
}

defineReplace(prependAll) {
    prepend = $$1
    arglist = $$2
    append  = $$3
    for(a,arglist) {
        result += $${prepend}$${a}$${append}
    }
    return ($$result)
}

LUPDATE = $${TOOLS_TREE}lupdate -noobsolete
LRELEASE = $${TOOLS_TREE}lrelease
TS = zh_TW

ts-touhou_musicplayer.target = $$prependAll(translations/touhou_musicplayer_,$$TS,.ts)
ts-touhou_musicplayer.commands = $$LUPDATE src -ts $$prependAll(translations/touhou_musicplayer_,$$TS,.ts)
ts-touhou_musicplayer.depends = sub-src
qm-touhou_musicplayer.target = $$prependAll(translations/touhou_musicplayer_,$$TS,.qm)
qm-touhou_musicplayer.commands = $$LRELEASE $$prependAll(translations/touhou_musicplayer_,$$TS,.ts)
qm-touhou_musicplayer.depends = sub-src ts-touhou_musicplayer

ts.depends = ts-touhou_musicplayer
qm.depends = qm-touhou_musicplayer

QMAKE_EXTRA_TARGETS += ts-touhou_musicplayer \
                       qm-touhou_musicplayer \
                       ts qm

