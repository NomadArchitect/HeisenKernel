/**
 ** This file is part of AliNix.

**AliNix is free software: you can redistribute it and/or modify
**it under the terms of the GNU Affero General Public License as published by
**the Free Software Foundation, either version 3 of the License, or
**(at your option) any later version.

**AliNix is distributed in the hope that it will be useful,
**but WITHOUT ANY WARRANTY; without even the implied warranty of
**MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**GNU Affero General Public License for more details.

**You should have received a copy of the GNU Affero General Public License
**along with AliNix. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __ALINIX__LOG_H
#define __ALINIX__LOG_H

#include <alinix/types.h>

#define LOG_SHOW_MS 1


//? Use typedef?
enum LogLevel
{
    Info,
    Warning,
    Error
};

void Log(enum LogLevel level, const char* __restrict__ format, ...);
void Print(const char* data, uint32_t length);

#endif /*__ALINIX__LOG_H*/