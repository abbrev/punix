/*
 * PedroM - Operating System for Ti-89/Ti-92+/V200.
 * Copyright (C) 2003 PpHd
 * 
 * $Id$
 * 
 * 2005-04-05 Chris Williams
 *   changed routines to work with `size_t' (as the standards dictate), where
 *   `size_t' is 32 bits wide (two words)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

| Memory move / copy
| String functions

| 0. Can't use movem due to HW1 Archive bug (Or?)
| 1. Check for Src unaligned (Copy one byte if so)
| 2. Check for Dest unaligned (Slow version)
| 3. Fast copy
| 4. Last Byte ?

.section _st1, "x"

.global memcpy
.global memcpy_reg
| void *memcpy(void *dest, const void *src, size_t count);
memcpy:
	move.l	4(%a7),%a0	| dest
	move.l	8(%a7),%a1	| src
	move.l	12(%a7),%d0	| count
	beq.s	memend
memcpy_reg:
0:		move.b	(%a1)+,(%a0)+
		subq.l	#1,%d0
		bne.s	0b
memend:	move.l	4(%a7),%a0
	rts

.global memmove
.global memmove_reg
| void *memmove(void *dest, const void *src, size_t count);
memmove:
	move.l	4(%a7),%a0	| dest
	move.l	8(%a7),%a1	| src
	move.l	12(%a7),%d0	| count
	beq.s	memend
memmove_reg:
	cmp.l	%a0,%a1		| Src <= dest
	bls.s	1f
0:		move.b	(%a1)+,(%a0)+
		subq.l	#1,%d0
		bne.s	0b
	bra.s	memend
1:
	add.l	%d0,%a0
	add.l	%d0,%a1
0:		move.b	-(%a1),-(%a0)
		subq.l	#1,%d0
		bne.s	0b
	bra.s	memend

.global memset
| void *memset(void *s, int c, size_t count);
memset:
	move.l	4(%a7),%a0	| s
	move.b	9(%a7),%d2	| c
	move.l	10(%a7),%d0	| count
	beq.s	memend
0:		move.b	%d2,(%a0)+
		subq.l	#1,%d0
		bne.s	0b
	bra.s	memend

.global memchr, memchr_reg
| void *memchr(const void *s, int c, size_t n); 
memchr:
	move.l	4(%a7),%a0	| s
	move.b	9(%a7),%d2	| c
	move.l	10(%a7),%d0	| n
memchr_reg:
	beq.s	9f
	bra.s	1f
0:		cmp.b	(%a0)+,%d2
1:		subq.l	#1,%d0
		bne.s	0b
	subq.l	#1,%a0
	beq.s	1f
9:	suba.l	%a0,%a0		| is this quicker/smaller than "clr.l %a0"?
1:	rts

.global memcmp, memcmp_reg
| int memcmp(const void *s1, const void *s2, size_t count); 
memcmp:
	move.l	4(%sp),%a0		| s1
	move.l	8(%sp),%a1		| s2
	move.l	12(%sp),%d0		| count
memcmp_reg:
	tst.l	%d0
	beq	2f
0:	cmp.b	(%a1)+,(%a0)+
	bne	1f
	subq.l	#1,%d0
	bne	0b
1:	move.b	-(%a0),%d0
	sub.b	-(%a1),%d0
	ext.w	%d0
2:	rts

.global strchr
| char *strchr(const char *s, int c);
strchr:
	move.l	4(%sp),%a0	| s
	move	8(%sp),%d0	| c
0:
	move.b	(%a0),%d1
	cmp.b	%d1,%d0
	beq	1f		| *s == c?
	tst.b	(%a0)+
	bne	0b		| '\0'?
	sub.l	%a0,%a0	| return NULL
1:	rts

.global strlen, strlen_reg
| size_t strlen(const char *s);
| Side Effect:
|	Doesn't destroy %a1/%d1/%d2
strlen:
	move.l	4(%a7),%a0	| s
strlen_reg:
	move.l	%a0,%d0
0:		tst.b	(%a0)+
		bne.s	0b
	sub.l	%a0,%d0
	not.l	%d0		| (not x) == (-x - 1)
	rts

.global strcat
| char *strcat(char *dest, const char *src);
strcat:
	move.l	4(%a7),%a0	| dest
	move.l	8(%a7),%a1	| src
	move.l	%a0,%d0
0:		tst.b	(%a0)+
		bne.s	0b
	subq.l	#1,%a0
0:		move.b	(%a1)+,(%a0)+
		bne.s	0b
	move.l	%d0,%a0
	rts

.global strcpy, strcpy_reg
| char *strcpy(char *dest, const char *src);
strcpy:
	move.l	4(%a7),%a0	| dest
	move.l	8(%a7),%a1	| src
strcpy_reg:
	move.l	%a0,%d0
0:		move.b	(%a1)+,(%a0)+
		bne.s	0b
	move.l	%d0,%a0
	rts

.global strcmp
.global strcmp_reg
| int strcmp(const char *s1, const char *s2);
strcmp:
	move.l	4(%a7),%a0	| s1
	move.l	8(%a7),%a1	| s2
strcmp_reg:
	clr	%d0
	bra	1f
0:	sub.b	(%a1)+,%d0
	bne	0f		| same?
1:	move.b	(%a0)+,%d0
	bne	0b		| '\0'?

	sub.b	(%a1),%d0
0:	ext.w	%d0
	rts

/*
.global cmpstri	
| short cmpstri(const unsigned char *s1, const unsigned char *s2);
cmpstri:
	move.l	4(%a7),%a0
	move.l	8(%a7),%a1
	bra.s	2f
0:		cmpi.b	#'A'-1,%d0
		bls.s	1f
		cmpi.b	#'Z',%d0
		bhi.s	1f
			addi.b	#'a'-'A',%d0
1:		cmpi.b	#'A'-1,%d1
		bls.s	1f
		cmpi.b	#'Z',%d1
		bhi.s	1f
			addi.b	#'a'-'A',%d1
1:		cmp.b	%d0,%d1
		beq.s	2f
			moveq	#1,%d0
			rts
2:		move.b	(%a0)+,%d0
		move.b	(%a1)+,%d1
		bne.s	0b
	ext.w	%d0
	rts
*/

.global strncmp
| int strncmp(const char *s1, const char *s2, size_t n);
strncmp:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%a1	| s2
	move.l	12(%sp),%d1	| n
strncmp_reg:
	clr	%d0

	tst.l	%d1
	bne	1f
	rts
0:	sub.b	(%a1)+,%d0
	bne	0f		| same?
	subq	#1,%d1
	beq	0f		| n != 0?
1:	move.b	(%a0)+,%d0
	bne	0b		| '\0'?
	sub.b	(%a1),%d0
0:	ext.w	%d0
	rts

.global strncpy	
| char *strncpy(char *dest, const char *src, size_t n);
strncpy:
	move.l	4(%sp),%a0	| dest
	move.l	8(%sp),%a1	| src
	move.l	12(%sp),%d0	| n
strncpy_reg:
	move.l	%a0,%d1		| save dest
	addq.l	#1,%d0
0:		subq	#1,%d0
1:		beq	9f	| n == 0?
		move.b	(%a1)+,(%a0)+
		bne	0b	| '\0'?
	| pad dest string with NUL bytes
	tst.l	%d0
	bra	1f
0:		clr.b	(%a0)+
		subq.l	#1,%d0
1:		bne	0b	| n == 0?
9:	move.l	%d1,%a0		| dest
	rts

.global strncat
| char *strncat(char *dest, const char *src, size_t n);
strncat:
	move.l	4(%sp),%a0	| dest
	move.l	8(%sp),%a1	| src
	move.l	12(%sp),%d0	| n
strncat_reg:
	tst.l	%d0
	beq	1f
	move.l	%a0,%d1		| save dest
0:		tst.b	(%a0)+
		bne	0b
	subq.l	#1,%a0
	| copy up to n bytes from src to dest
0:		move.b	(%a1)+,(%a0)+
		beq	0f	| '\0'?
		subq.l	#1,%d0
		bne	0b	| n == 0?

	clr.b	(%a0)		| nul-terminate the string
0:	move.l	%d1,%a0		| restore dest
1:	rts

/*
size_t strspn(const char *s1, const char *accept)
{
	const char *c;
	const char *s;
	for (s = s1; *s; ++s) {
		c = accept;
		while (*c)
			if (*s == *c++) continue;
		break;
	}
	return s - s1;
}

size_t strcspn(const char *s1, const char *reject)
{
	const char *c;
	const char *s;
	for (s = s1; *s; ++s) {
		c = reject;
		while (*c)
			if (*s == *c++) goto out;
	}
out:	return s - s1;
}
*/

.global strspn
| size_t strspn(const char *s1, const char *accept);
strspn:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%d1	| accept
	
	bra	1f
0:		move.l	%d1,%a1		| c = accept
3:			move.b	(%a1)+,%d2	| %d2 = *c++
			beq	5f		| *c == '\0'?
			cmp.b	%d2,%d0	
			bne	3b		| *s == *c++?
4:		addq.l	#1,%a0		| ++s
1:		move.b	(%a0),%d0
		bne	0b		| *s == '\0'?
5:	move.l	%a0,%d0
	sub.l	4(%sp),%d0	| s - s1
	rts

.global strcspn
| size_t strcspn(const char *s1, const char *reject);
strcspn:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%d1	| reject
	
	bra	1f
0:		move.l	%d1,%a1		| c = reject
		bra	3f
2:			cmp.b	%d2,%d0
			beq	5f		| *s != *c++?
3:			move.b	(%a1)+,%d2
			bne	2b		| *c == '\0'?
4:		addq.l	#1,%a0		| ++s
1:		move.b	(%a0),%d0
		bne	0b		| *s == '\0'?
5:	move.l	%a0,%d0
	sub.l	4(%sp),%d0	| s - s1
	rts

/*
char *strpbrk(const char *s1, const char *accept)
{
	const char *c;
	const char *s;
	for (s = s1; *s; ++s) {
		for (c = accept; *c; ++c) {
			if (*s == *c) return s;
	}
	return NULL;
}
*/

.global strpbrk
| char *strpbrk(const char *s1, const char *accept)
strpbrk:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%d2	| accept
	
	bra	1f
0:		move.l	%d2,%a1
		bra	3f
2:			cmp.b	%d1,%d0
			beq	4f		| *s == *c?
3:			move.b	(%a1)+,%d1
			bne	2b		| *c == '\0'?
1:		move.b	(%a0)+,%d0
		bne	0b
	sub.l	%a0,%a0		| return NULL
	rts
4:	subq.l	#1,%a0		| return s
	rts

/*
.global strrchr
| char *strrchr(const char *str, short c);
strrchr:
	move.l	4(%sp),%d1
	move.l	%d1,%a0
	move.b	9(%sp),%d0
\TheEnd:	tst.b	(%a0)+
		bne.s	\TheEnd
\Loop:		cmp.b	-(%a0),%d0
		beq.s	\Found
		cmp.l	%a0,%d1
		bne.s	\Loop
	suba.l	%a0,%a0
\Found:	rts

.global strstr
| char *strstr(const char *s1, const char *s2);
strstr:
	move.l	4(%sp),%a0
	move.l	8(%sp),%a1
\Loop1:		moveq	#0,%d2
\Loop2:			| Scan starting from %a0 the string str2 
			move.b	0(%a0,%d2.l),%d1
			move.b	0(%a1,%d2.l),%d0
			beq.s	\Exit
			addq.l	#1,%d2
			cmp.b	%d0,%d1
			beq.s	\Loop2
		tst.b	(%a0)+
		bne.s	\Loop1
	suba.l	%a0,%a0
\Exit:	rts

.global strerror
| char *strerror(int errnum);
strerror:
	move	4(%a7),%d0
	lea	StrError_msg_str(pc),%a0
	cmpi	#21,%d0
	bhi.s	\end
\loop			tst.b	(%a0)+
			bne.s	\loop
		dbf	%d0,\loop
\end:	rts

.global strtok
| char *strtok(char *s1, const char *s2); 
strtok:
	move.l	4(%a7),%d0	| s1
	beq.s	\NotNull
		move.l	%d0,STRTOK_PTR	| Save Ptr
\NotNull:
	tst.l	STRTOK_PTR
	beq.s	\Error
	
	move.l	STRTOK_PTR,%a1
\Loop		move.b	(%a1)+,%d1		| Next char
		beq.s	\EndOfString		| End of string
		move.l	8(%a7),%a0		| s2: Token Chars
\CharLoop		move.b	(%a0)+,%d0	| Read next Token Char
			beq.s	\Loop		| End of Token string ? Next char in string
			cmp.b	%d1,%d0		| Cmp 2 chars
			beq.s	\TokenCharFound	| Yes, found
			bra.s	\CharLoop	| No next Token
\EndOfString
	move.l	STRTOK_PTR,%a0
	clr.l	STRTOK_PTR
	rts
\TokenCharFound
	move.l	STRTOK_PTR,%a0
	clr.b	-1(%a1)		| Clear Token
	move.l	%a1,STRTOK_PTR
	rts	
\Error	suba.l	%a0,%a0
	rts
*/
