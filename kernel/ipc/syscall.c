/**
 * @author Ali Mirmohammad
 * @file syscall.c
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


/**
 * @abstraction:
 *  - System call for the kernel implemented here.
*/


/*
+---+--------------------+
| r |    Register(s)     |
+---+--------------------+
| a |   %eax, %ax, %al   |
| b |   %ebx, %bx, %bl   |
| c |   %ecx, %cx, %cl   |
| d |   %edx, %dx, %dl   |
| S |   %esi, %si        |
| D |   %edi, %di        |
+---+--------------------+
*/

#include <alinix/syscall.h>
#include <alinix/module.h>


MODULE_AUTHOR("Ali Mirmohammad")
MODULE_DESCRIPTION("System call for the kernel implemented here.")
MODULE_LICENSE("AGPLv3")
MODULE_VERSION("0.1")


/**
 * @brief Perform a system call with the specified interrupt number and arguments.
 *
 * This function invokes a system call by executing the `int $0x80` instruction with the provided interrupt number and arguments.
 * The system call number is passed in the `intNum` parameter, and the arguments are passed in the `arg1`, `arg2`, `arg3`, `arg4`, and `arg5` parameters.
 * The function returns the value stored in the `a` register after the system call.
 *
 * @param intNum The interrupt number of the system call.
 * @param arg1 The first argument of the system call.
 * @param arg2 The second argument of the system call.
 * @param arg3 The third argument of the system call.
 * @param arg4 The fourth argument of the system call.
 * @param arg5 The fifth argument of the system call.
 *
 * @return The value stored in the `a` register after the system call.
 *
 * @throws None
 */
int DoSyscall(unsigned int intNum, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, unsigned int arg5)
{
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (intNum), "b" ((int)arg1), "c" ((int)arg2), "d" ((int)arg3), "S" ((int)arg4), "D" ((int)arg5));
    return a;
}