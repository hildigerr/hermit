# Copyright (C) 2007-2010, Evgeny Ratnikov
# Copyright (C) 2018, Roberto Vergaray
#
# This file is part of hermit, forked from termit.
# termit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 
# as published by the Free Software Foundation.
# termit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with termit. If not, see <http://www.gnu.org/licenses/>.

SET(HERMIT_ETC termit/rc.lua termit/colormaps.lua termit/utils.lua termit/messenger-1st-image-mercury.jpg)

INSTALL(FILES ${HERMIT_ETC} DESTINATION ${HERMIT_XDG_DIR})

