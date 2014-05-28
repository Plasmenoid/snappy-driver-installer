/*
This file is part of Snappy Driver Installer.

Snappy Driver Installer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Snappy Driver Installer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Snappy Driver Installer.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef _WIN64
#include <cfg.h>
#include <newdev.h>
#define CR_SUCCESS (0x00000000)
#define CR_NO_SUCH_DEVICE_INTERFACE (0x00000037)
#define CR_NO_SUCH_VALUE (0x00000025)
#define CR_NO_SUCH_DEVNODE (0x0000000D)
#define CR_NO_SUCH_DEVINST CR_NO_SUCH_DEVNODE
#else
#include <ddk\cfgmgr32.h>
#include <ddk\newdev.h>
#endif
