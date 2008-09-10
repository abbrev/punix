/* make sure this agrees with signal.h */

.equ	SIGHUP,1 		/* Hangup detected on controlling terminal
				   or death of a controlling process */
.equ	SIGINT,2 		/* Interrupt from keyboard */
.equ	SIGQUIT,3		/* Quit from keyboard */
.equ	SIGILL,4 		/* Illegal instruction */
.equ	    ILL_RESAD_FAULT,	0x0	/* reserved addressing fault */
.equ	    ILL_PRIVIN_FAULT,	0x1	/* privileged instruction fault */
.equ	    ILL_RESOP_FAULT,	0x2	/* reserved operand fault */
.equ	SIGTRAP,5		/* Trace/breakpoint trap */	/* [XSI] */
.equ	SIGABRT,6 		/* Abort signal from abort */
.equ	SIGIOT,SIGABRT		/* IOT trap. A synonym for SIGABRT */
/*.equ	SIGBUS,7*/ 		/* Bus error */
.equ	SIGFPE,8 		/* Floating point exception */
.equ	    FPE_INTOVF_TRAP,	0x1	/* integer overflow */
.equ	    FPE_INTDIV_TRAP,	0x2	/* integer divide by zero */
.equ	    FPE_FLTOVF_TRAP,	0x3	/* floating overflow */
.equ	    FPE_FLTDIV_TRAP,	0x4	/* floating/decimal divide by zero */
.equ	    FPE_FLTUND_TRAP,	0x5	/* floating underflow */
.equ	    FPE_DECOVF_TRAP,	0x6	/* decimal overflow */
.equ	    FPE_SUBRNG_TRAP,	0x7	/* subscript out of range */
.equ	    FPE_FLTOVF_FAULT,	0x8	/* floating overflow fault */
.equ	    FPE_FLTDIV_FAULT,	0x9	/* divide by zero floating fault */
.equ	    FPE_FLTUND_FAULT,	0xA	/* floating underflow fault */
.equ	SIGKILL,9		/* Kill signal */
.equ	SIGBUS,10		/* Bus error */
.equ	SIGSEGV,11		/* Invalid memory reference */
.equ	SIGSYS,12		/* Bad argument to system call */	/* [XSI] */
.equ	SIGPIPE,13		/* Broken pipe: write to pipe with no readers */
.equ	SIGALRM,14		/* Timer signal from alarm */
.equ	SIGTERM,15		/* Termination */
.equ	SIGURG,16		/* Urgent condition on IO channel */
.equ	SIGSTOP,17		/* Stop process */
.equ	SIGTSTP,18		/* Keyboard stop */
.equ	SIGCONT,19		/* Continue process */
.equ	SIGCHLD,20		/* Child has terminated or stopped */
.equ	SIGCLD,SIGCHLD
.equ	SIGTTIN,21		/* */
.equ	SIGTTOU,22		/* */
.equ	SIGIO,23		/* Input/output is possible */

.equ	SIGXCPU,24	/* exceeded CPU time limit */	/* [XSI] */
.equ	SIGXFSZ,25	/* exceeded file size limit */	/* [XSI] */

.equ	SIGVTALRM,26	/* Virtual time alarm */	/* [XSI] */

.equ	SIGPROF,27	/* profiling time alarm */	/* [XSI] */
.equ	SIGWINCH,28	/* window size changes */

.equ	SIGUSR1,30 		/* User-defined 1 */
.equ	SIGUSR2,31		/* User-defined 2 */
