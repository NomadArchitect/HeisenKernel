/**
 * @author Ali Mirmohammad
 * @file rpi.c
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
 * 	- Provides util over the raspberry PI.
*/

#include <alinix/types.h>
#include <alinix/kernel.h>
#include <alinix/init.h>
#include <alinix/module.h>

#include "rpi.h"

MODULE_AUTHOR("Ali Mirmohammad")
MODULE_DESCRIPTION("Raspberry PI abstraction")
MODULE_LICENSE("AGPL")
MODULE_VERSION("0.1")



static uint32_t MMIO_BASE;

/**
 * @brief Initializes the MMIO base address based on the Raspberry Pi model.
 *
 * This function initializes the MMIO base address based on the Raspberry Pi model.
 * It uses a switch statement to set the MMIO base address based on the value of the `raspi` parameter.
 * The function sets the MMIO base address to different values depending on the Raspberry Pi model.
 *
 * @param raspi The Raspberry Pi model number.
 */
static inline void mmio_init(int raspi){
    switch(raspi){
        case 2:
        case 3:  MMIO_BASE = 0x3F000000; break; // for raspi2 & 3
        case 4:  MMIO_BASE = 0xFE000000; break; // for raspi4
        default: MMIO_BASE = 0x20000000; break; // for raspi1, raspi zero etc.
    }
}

/*Memory-Mapped I/O output*/

/**
 * @brief Writes data to the specified MMIO register.
 *
 * This function writes the `data` to the specified MMIO register.
 * It uses the `MMIO_BASE` address and the `reg` parameter to calculate the memory address and writes the `data` to that address.
 *
 * @param reg The register offset from the MMIO base address.
 * @param data The data to write to the register.
 */
static inline void mmio_write(uint32_t reg, uint32_t data){
    *(volatile uint32_t*)(MMIO_BASE+reg) = data;
}

/**
 * @brief Reads data from the specified MMIO register.
 *
 * This function reads data from the specified MMIO register.
 * It uses the `MMIO_BASE` address and the `reg` parameter to calculate the memory address and reads the data from that address.
 *
 * @param reg The register offset from the MMIO base address.
 * @return The data read from the register.
 */
static inline uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t*)(MMIO_BASE + reg);
}

// #ifdef __i386__
// // Loop <delay> times in a way that the compiler won't optimize away
// static inline void delay(int32_t count)
// {
// 	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
// 		 : "=r"(count): [count]"0"(count) : "cc");
// }
// #endif
static inline void delay(int32_t count){
    RET;
}


/**
 * @brief Enum util for rpi.
*/
enum
{
    // The offsets for reach register.
    GPIO_BASE = 0x200000,
 
    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD = (GPIO_BASE + 0x94),
 
    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),
 
    // The base address for UART.
    UART0_BASE = (GPIO_BASE + 0x1000), // for raspi4 0xFE201000, raspi2 & 3 0x3F201000, and 0x20201000 for raspi1
 
    // The offsets for reach register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
 
    // The offsets for Mailbox registers
    MBOX_BASE    = 0xB880,
    MBOX_READ    = (MBOX_BASE + 0x00),
    MBOX_STATUS  = (MBOX_BASE + 0x18),
    MBOX_WRITE   = (MBOX_BASE + 0x20)
};


// A Mailbox message with set clock rate of PL011 to 3MHz tag
volatile unsigned int  __attribute__((aligned(16))) mbox[9] = {
    9*4, 0, 0x38002, 12, 8, 2, 3000000, 0 ,0
};


/**
 * @brief Initializes the UART (Universal Asynchronous Receiver/Transmitter) for communication.
 *
 * This function initializes the UART for communication.
 * It performs the following steps:
 * - Initializes the MMIO base address using the `mmio_init` function.
 * - Disables UART0.
 * - Sets up the GPIO pin 14 and 15.
 * - Disables pull-up/down for all GPIO pins and delays for 150 cycles.
 * - Disables pull-up/down for pin 14 and 15 and delays for 150 cycles.
 * - Writes 0 to GPPUDCLK0 to make the changes take effect.
 * - Clears pending interrupts.
 * - Sets the integer and fractional part of the baud rate.
 * - Enables the FIFO and 8-bit data transmission (1 stop bit, no parity).
 * - Masks all interrupts.
 * - Enables UART0, receive, and transmit parts of the UART.
 *
 * @param raspi The Raspberry Pi model number.
 */
void uart_init(int raspi)
{
	mmio_init(raspi);
 
	// Disable UART0.
	mmio_write(UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.
 
	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	mmio_write(GPPUD, 0x00000000);
	delay(150);
 
	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
 
	// Write 0 to GPPUDCLK0 to make it take effect.
	mmio_write(GPPUDCLK0, 0x00000000);
 
	// Clear pending interrupts.
	mmio_write(UART0_ICR, 0x7FF);
 
	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// Baud = 115200.
 
	// For Raspi3 and 4 the UART_CLOCK is system-clock dependent by default.
	// Set it to 3Mhz so that we can consistently set the baud rate
	if (raspi >= 3) {
		// UART_CLOCK = 30000000;
		unsigned int r = (((unsigned int)(&mbox) & ~0xF) | 8);
		// wait until we can talk to the VC
		while ( mmio_read(MBOX_STATUS) & 0x80000000 ) { }
		// send our message to property channel and wait for the response
		mmio_write(MBOX_WRITE, r);
		while ( (mmio_read(MBOX_STATUS) & 0x40000000) || mmio_read(MBOX_READ) != r ) { }
	}
 
	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	mmio_write(UART0_IBRD, 1);
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	mmio_write(UART0_FBRD, 40);
 
	// Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
 
	// Mask all interrupts.
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

/**
 * @brief Sends a character over UART.
 *
 * This function sends a character over UART.
 * It waits for the UART to become ready to transmit and then writes the character to the UART data register.
 *
 * @param c The character to send over UART.
 */
void uart_putc(unsigned char c)
{
	// Wait for UART to become ready to transmit.
	while ( mmio_read(UART0_FR) & (1 << 5) ) { }
	mmio_write(UART0_DR, c);
}

/**
 * @brief Reads a character from UART.
 *
 * This function reads a character from UART.
 * It waits for the UART to have received something and then reads the character from the UART data register.
 *
 * @return The character read from UART.
 */
unsigned char uart_getc()
{
    // Wait for UART to have received something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}

/**
 * @brief Sends a null-terminated string over UART.
 *
 * This function sends a null-terminated string over UART.
 * It iterates over each character in the string and sends it using the `uart_putc` function.
 *
 * @param str The null-terminated string to send over UART.
 */
void uart_puts(const char* str)
{
	for (size_t i = 0; str[i] != '\0'; i ++)
		uart_putc((unsigned char)str[i]);
}
 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif /*__cplusplus*/
 
#ifdef AARCH64
// arguments for AArch64
void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
#else
// arguments for AArch32
void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
#endif /*AARCH64*/
{
	// initialize UART for Raspi2
	uart_init(2);
	uart_puts("Hello, kernel World!\r\n");
 
	while (1)
		uart_putc(uart_getc());
}