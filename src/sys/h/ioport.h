#ifndef _SYS_IOPORT_H_
#define _SYS_IOPORT_H_

#define LINK_CONTROL (*(volatile unsigned char *)0x60000c)
#define LC_TRIGRX    0x01 /* trigger auto-int 4 on byte in receive buffer */
#define LC_TRIGTX    0x02 /* on transmit buffer empty */
#define LC_TRIGANY   0x04 /* on any link activity (internal?) */
#define LC_TRIGERR   0x08 /* on error (timeout or protocol violation) */
#define LC_TODISABLE 0x20 /* disable link timeout */
#define LC_DIRECT    0x40 /* disable byte sender/receiver and interrupts */
#define LC_AUTOSTART 0x80 /* enable autostart */

#define LINK_STATUS (*(volatile unsigned char *)0x60000d)
#define LS_ACTIVITY 0x08 /* link activity */
#define LS_INTPEND  0x10 /* always set in AI4 */
#define LS_RXBYTE   0x20 /* byte is in receive buffer */
#define LS_TXEMPTY  0x40 /* transmit buffer is empty */
#define LS_ERROR    0x80 /* error */

#define LINK_DIRECT (*(volatile unsigned char *)0x60000e)
#define LD_D0ACTIVE 0x01
#define LD_D1ACTIVE 0x02
#define LD_D0STATUS 0x04
#define LD_D1STATUS 0x08

#define LINK_BUFFER (*(volatile unsigned char *)0x60000f)

#endif
