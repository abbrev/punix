/* really simple copyin and copyout functions.
 * these should be implemented to catch invalid memory ranges.
 * since the MC68000 doesn't have an MMU, we can only do so much, as far
 * as memory protection and fault detection go. :(
 */

.global copyout
.global copyin

.section _st1, "x"

/* int copyout(void *dest, void *src, size_t count) */
copyout:

/* int copyin(void *dest, void *src, size_t count) */
copyin:
	move.l	4(%sp),%a0	| dest
	move.l	8(%sp),%a1	| src
	move.l	12(%sp),%d0	| count
	beq.b	1f
0:	move.b	(%a1)+,(%a0)+
	subq.l	#1,%d0
	bne.s	0b
1:	rts
