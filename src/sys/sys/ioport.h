#ifndef _SYS_IOPORT_H_
#define _SYS_IOPORT_H_

#define LINK_CONTROL (*(volatile unsigned char *)0x60000c)
#define LC_TRIGRX    0001 /* trigger auto-int 4 on byte in receive buffer */
#define LC_TRIGTX    0002 /* on transmit buffer empty */
#define LC_TRIGANY   0004 /* on any link activity (internal?) */
#define LC_TRIGERR   0010 /* on error (timeout or protocol violation) */
#define LC_TODISABLE 0040 /* disable link timeout */
#define LC_DIRECT    0100 /* disable byte sender/receiver and interrupts */
#define LC_AUTOSTART 0200 /* enable autostart */

#define LINK_STATUS (*(volatile unsigned char *)0x60000d)
#define LS_ACTIVITY 0010 /* link activity */
#define LS_INTPEND  0020 /* always set in AI4 */
#define LS_RXBYTE   0040 /* byte is in receive buffer */
#define LS_TXEMPTY  0100 /* transmit buffer is empty */
#define LS_ERROR    0200 /* error */

#define LINK_DIRECT (*(volatile unsigned char *)0x60000e)
#define LD_D0ACTIVE 0001
#define LD_D1ACTIVE 0002
#define LD_D0STATUS 0004
#define LD_D1STATUS 0010

#define LINK_BUFFER (*(volatile unsigned char *)0x60000f)

#endif
