#ifndef _H_SIGNAL_H_
#define _H_SIGNAL_H_


/*
 * Signal properties and actions.
 * The array below categorizes the signals and their default actions
 * according to the following properties:
 */
#define SA_KILL         0x01            /* terminates process by default */
#define SA_CORE         0x02            /* ditto and coredumps */
#define SA_STOP         0x04            /* suspend process */
#define SA_TTYSTOP      0x08            /* ditto, from tty */
#define SA_IGNORE       0x10            /* ignore by default */
#define SA_CONT         0x20            /* continue if suspended */

#define SAS_OLDMASK  0x01 /* need to restore mask before pause */
#define SAS_ALTSTACK 0x02 /* have alternate signal stack */

#define SIG_CATCH ((sighandler_t)2)

#define CONTSIGMASK     (sigmask(SIGCONT))
#define STOPSIGMASK     (sigmask(SIGSTOP) | sigmask(SIGTSTP) | \
                         sigmask(SIGTTIN) | sigmask(SIGTTOU))
#define SIGCANTMASK     (sigmask(SIGKILL) | sigmask(SIGSTOP))

#endif
