/**
 * @author Ali Mirmohammad
 * @file hcd.c
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
 * 	- Implements the HCD for the kernel.
*/

#include <alinix/kernel.h>
#include <alinix/usb.h>
#include <alinix/module.h>

MODULE_AUTHOR("Ali Mirmohammad")
MODULE_DESCRIPTION("HCD for the kernel")
MODULE_LICENSE("AGPL-3.0")
MODULE_VERSION("0.1")
// #include <alinix/spinlock.h>

#define EIDRM		36	/* Identifier removed */

static inline hcd_urb_unlink_lock(){};


/**
 * @brief Unlinks a URB (USB Request Block) from the USB host controller.
 *
 * This function unlinks a URB (USB Request Block) from the USB host controller.
 * It sets the status of the URB to the provided status value and notifies the
 * USB host controller driver to unlink the URB.
 *
 * @param urb A pointer to the URB to be unlinked.
 * @param status An integer value representing the status to be set on the URB.
 *
 * @return An integer value representing the result of the unlink operation.
 *         If the unlink operation is successful, the returned value is zero.
 *         Otherwise, a negative error code is returned.
 *
 * @note The function assumes that the URB is associated with a USB device.
 *
 * @example
 * struct urb *urb = ...; // Example URB
 * int status = -ECONNRESET; // Example status value
 *
 * int retval = usb_hcd_unlink_urb(urb, status);
 * if (retval < 0) {
 *     // Handle error
 * }
 */
int usb_hcd_unlink_urb (struct urb *urb, int status){
    // struct usb_hcd		*hcd;
	struct usb_device	*udev = urb->dev;
	int			retval = -EIDRM;
	unsigned long		flags;

    // spin_lock_irqsave(&hcd_urb_unlink_lock, flags);

}


