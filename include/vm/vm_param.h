#ifndef _VM_VM_PARAM_H_
#define _VM_VM_PARAM_H_

/* stolen from 4.4BSD-Lite2 */
struct vmtotal {
	int16_t t_rq;           /* length of the run queue */
	int16_t t_dw;           /* jobs in ``disk wait'' (neg priority) */
	int16_t t_pw;           /* jobs in page wait */
	int16_t t_sl;           /* jobs sleeping in core */
	int16_t t_sw;           /* swapped out runnable/short block jobs */
	int32_t t_vm;           /* total virtual memory */
	int32_t t_avm;          /* active virtual memory */
	int32_t t_rm;           /* total real memory in use */
	int32_t t_arm;          /* active real memory */
	int32_t t_vmshr;        /* shared virtual memory */
	int32_t t_avmshr;       /* active shared virtual memory */
	int32_t t_rmshr;        /* shared real memory */
	int32_t t_armshr;       /* active shared real memory */
	int32_t t_free;         /* free memory pages */
};


#endif
