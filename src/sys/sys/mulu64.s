.if 0
(1000a+100b+10c+1d)*(1000e+100f+10g+1h) =
1000000ae + 100000af + 10000ag + 1000ah +
 100000be +  10000bf +  1000bg +  100bh +
  10000ce +   1000cf +   100cg +   10ch +
   1000de +    100df +    10dg +    1dh


ae 00
af 11
ag 22
ah 33
be 44
bf 55
bg 66
bh 77
ce 88
cf 99
cg aa
ch bb
de cc
df dd
dg ee
dh ff

HAHAHA! Sweet optimization note: any two longs (as a result of multiplying two words) can be added without carry if they are staggered. The largest product of two 16-bit unsigned values is 0xFFFE0001, so staggering the adds like so will never carry:

FFFE00010000
0000FFFE0001
------------
FFFEFFFF0001

These can be chained like so without carry.

0055ddff
.4433ee.
.1199bb.
...66...
........
........
........
........
........
......ff
.....bb.
.....ee.
....77..
....aa..
....dd..
...33...
...66...
...99...
...cc...
..22....
..55....
..88....
.11.....
.44.....
00......



0088....
.44cc...

.1199...
..55aa..

..2277..
...66ee.

...33bb.
....ddff


   ah
   bg
   cf
   de
  ag  dh
  bf ch
  ce dg
 af bh
 be cg
ae  df
00000000

.endif

| this routine currently uses the following partial product layout:
|exploiting the no-carry trick noted above:
|first partial product:
|0055ddff +
|.44ccee.
|second partial product:
|.1166bb. +
|..88aa..
|third partial product:
|..2277.. +
|...99...
|fourth partial product:
|...33...

	.long	0xcafebabe
| multiply unsigned 64-bit integers
| return unsigned 128-bit integer
| input:
|  x = %d0:%d1
|  y = %d2:%d3
| output:
|  x*y = %d0:%d1:%d2:%d3
| 2326 cycles (74 cycles per mulu)
mulu64:
	| save registers
	movem.l	%d4/%a0-%a1,-(%sp)

|// first partial product
|// 0055ddff
|push:
|ff << 0  dh
|dd << 2  df
|55 << 4  bf
|00 << 6  ae

	| ab:cd d0:d1
	| ef:gh d2:d3
	
	| dh
	move.w	%d1,%d4
	mulu.w	%d3,%d4		| dh
	move.l	%d4,-(%sp)

	| df
	move.w	%d1,%d4
	mulu.w	%d2,%d4		| df
	move.l	%d4,-(%sp)

	| bf
	move.w	%d0,%d4
	mulu.w	%d2,%d4		| bf
	move.l	%d4,-(%sp)

	| ae
	swap	%d2
	| ab:cd d0:d1
	| fe:gh d2:d3
	move.l	%d0,%d4
	swap	%d4
	mulu.w	%d2,%d4		| ae
	move.l	%d4,-(%sp)

	|bra	9f

|
|// .44ccee.
|add without carry:
|ee << 1  dg
|cc << 3  de
|44 << 5  be

	| dg
	move.w	%d1,%d4
	swap	%d3
	| ab:cd d0:d1
	| fe:hg d2:d3
	mulu.w	%d3,%d4		| dg
	add.l	%d4,5*2(%sp)

	| de
	move.w	%d1,%d4
	mulu.w	%d2,%d4		| de
	add.l	%d4,3*2(%sp)

	| be
	move.w	%d0,%d4
	mulu.w	%d2,%d4		| be
	add.l	%d4,1*2(%sp)


|
|// second partial product
|// .1166bb.
|push:
| 0x0 << 0  // don't really need this
|bb << 1  ch
|66 << 3  bg
|11 << 5  af
| 0x0 << 7

	| ch
	swap	%d1
	| ab:dc d0:d1
	| fe:hg d2:d3
	move.l	%d3,%d4
	swap	%d4
	mulu.w	%d1,%d4		| ch
	move.l	%d4,-(%sp)

	| bg
	move.w	%d0,%d4
	mulu.w	%d3,%d4		| bg
	move.l	%d4,-(%sp)

	| af
	swap	%d0
	| ba:dc d0:d1
	| fe:hg d2:d3
	move.l	%d2,%d4
	swap	%d4
	mulu.w	%d0,%d4		| af
	move.l	%d4,-(%sp)

	clr.w	-(%sp)

|
|// ..88aa..
|add without carry:
|aa << 2  cg
|88 << 4  ce

	| cg
	move	%d1,%d4
	mulu.w	%d3,%d4		| cg
	add.l	%d4,4*2(%sp)

	| ce
	move	%d1,%d4
	mulu.w	%d2,%d4		| ce
	add.l	%d4,2*2(%sp)
|
|add with carry (upper 7 words or 3.5 longs)

	lea	7*2(%sp),%a1		| second partial product
	lea	7*2+7*2(%sp),%a0	| result
	addx.l	-(%a1),-(%a0)
	addx.l	-(%a1),-(%a0)
	addx.l	-(%a1),-(%a0)
	addx.w	-(%a1),-(%a0)

| pop off our second partial product
	lea	7*2(%sp),%sp

|
|// third partial product
|// ..2277..
|push:
|0x00 << 0  // don't really need this
|77 << 2  bh
|22 << 4  ag
|0x00 << 6

	| bh
	move.l	%d0,%d4
	swap	%d4
	swap	%d3
	| ba:dc d0:d1
	| fe:gh d2:d3
	mulu.w	%d3,%d4		| bh
	move.l	%d4,-(%sp)

	| ag
	swap	%d3
	| ba:dc d0:d1
	| fe:hg d2:d3
	move.w	%d0,%d4
	mulu.w	%d3,%d4		| ag
	move.l	%d4,-(%sp)

	clr.l	-(%sp)

|
|// ...99...
|add without carry:
|99 << 3  cf

	| cf
	move.w	%d1,%d4
	swap	%d2
	| ba:dc d0:d1
	| ef:hg d2:d3
	mulu.w	%d2,%d4		| cf
	add.l	%d4,3*2(%sp)


|
|add with carry (upper 6 words or 3 longs)
	lea	6*2(%sp),%a1		| third partial product
	lea	6*2+6*2(%sp),%a0	| result
	addx.l	-(%a1),-(%a0)
	addx.l	-(%a1),-(%a0)
	addx.l	-(%a1),-(%a0)

| pop off the third partial product
	lea	6*2(%sp),%sp

|
|// fourth partial product
|// ...33...
|push:
|0x00 << 0  // don't really need this
|0x0 << 2   // or this
|33 << 3  ah
|0x0 << 5
|0x00 << 6

	| ah
	move	%d0,%d4
	swap	%d3
	| ba:dc d0:d1
	| ef:gh d2:d3
	mulu.w	%d3,%d4		| ah
	move.l	%d4,-(%sp)

	clr.w	-(%sp)
	clr.l	-(%sp)

|
|add with carry (upper 5 words or 2.5 longs)
	lea	5*2(%sp),%a1		| fourth partial product
	lea	5*2+5*2(%sp),%a0	| result
	addx.l	-(%a1),-(%a0)
	addx.l	-(%a1),-(%a0)
	addx.w	-(%a1),-(%a0)

| pop off fourth partial product
	lea	5*2(%sp),%sp

9:
	| pop off result and saved registers
	movem.l	(%sp)+,%d0-%d4/%a0-%a1
	rts

.global mul64
| void mul64(int64 *a, int64 *b, int128 *result);
mul64:
	| save
	move.l	%d3,-(%sp)

	move.l	4+4(%sp),%a0
	move.l	4+4+4(%sp),%a1
	movem.l	(%a0),%d0-%d1
	movem.l	(%a1),%d2-%d3

	|jbsr	mulu64
	|jbsr	mulu64b
	movem.l	%d4-%d7,-(%sp)
	jbsr	mulu64c
	movem.l	(%sp)+,%d4-%d7

	move.l	4+4+4+4(%sp),%a0
	movem.l	%d0-%d3,(%a0)

	| restore
	move.l	(%sp)+,%d3
	rts



| this version is like mulu64 above, but it stores the result in registers
| instead of the stack for speed

| calculate these first:
| ag ah bg bh cg ch dg dh
| this will free up %d3 to use as a scratch register
| 00000000
|       dh \
|      ch  |
|      dg  |
|     bh    >-- free up %d3
|     cg   |
|    ah    |
|    bg    |
|   ag     /
| 00000000
|     df   \
|    cf     >-- free up %d1
|    de    |
|   ce     /
|   bf
|  af
|  be
| ae
| 00000000
|
| %d4 %d5 %d6 %d7
|              dh
|          bh
|            dg    no carry
|                  clear %d5
|        bg        no carry
| %d4 %d5 %d6 %d7
|            ch    carry up to %d5
|                  if needed, we can move %d7 to %a0 and use it as a temporary
|        ah        carry up to %d5
|          cg      carry up to %d5
|                  compute ag in %d3 and clear %d4
|      ag          carry into %d4
| --second half--
| %d4 %d5 %d6 %d7
|  ae
|    be            carry up to %d4
|    af            carry up to %d4
|      bf          carry up to %d4
|                  %d0 is now scratch
| %d4 %d5 %d6 %d7
|      ce          carry up to %d4
|        de        carry up to %d4
|        cf        carry up to %d4
|          df      carry up to %d4



.if 0
	| add staggered long:
	| %d4 = dg
	swap	%d4	| gd
	add.w	%d4,%d6
	moveq.w	#0,%d4
	add.l	%d4,%d7

	| %d4 = bg
	swap	%d4	| gb
	add.w	%d4,%d5
	moveq.w	#0,%d4
	add.l	%d4,%d6
.endif

| multiply unsigned 64-bit integers
| return unsigned 128-bit integer
| input:
|  x = %d0:%d1
|  y = %d2:%d3
| output:
|  x*y = %d0:%d1:%d2:%d3
mulu64b:
	| save registers
	movem.l	%d4-%d7,-(%sp)

	move.l	%d2,%a0		| d2 is scratch
	moveq.l	#0,%d4
	| ab:cd d0:d1
	| ef:gh a0:d3
	
|                  clear %d5
	move.l	%d4,%d5

| %d4 %d5 %d6 %d7
|              dh
	move.w	%d1,%d7		| d
	mulu.w	%d3,%d7		| dh

| %d4 %d5 %d6 %d7
|          bh
	move.w	%d0,%d6		| b
	mulu.w	%d3,%d6		| bh

| %d4 %d5 %d6 %d7
|            dg    no carry
	swap	%d3
	| ab:cd d0:d1
	| ef:gh a0:d3
	move	%d3,%d2
	mulu	%d1,%d2
	swap	%d2
	add.w	%d2,%d6
	move.w	%d4,%d2		| clear low word
	add.l	%d2,%d7		| XXX this may carry!
	addx.w	%d2,%d6		| add 0 + carry


| %d4 %d5 %d6 %d7
|        bg        no carry
	move.l	%d3,%d2
	swap	%d2
	mulu	%d0,%d2
	swap	%d2
	move.w	%d2,%d5
	move.w	%d4,%d2		| clear low word
	add.l	%d2,%d6		| XXX this may carry!
	addx.w	%d2,%d5		| add 0 + carry

| %d4 %d5 %d6 %d7
|            ch    carry up to %d5
	move.l	%d1,%d2
	swap	%d2
	mulu	%d3,%d2
	swap	%d2
	move.w	%d2,%d4		| d4 is high word (goes into d6)
	clr.w	%d2		| d2 is low word (goes into d7)
	add.l	%d2,%d7
	addx.l	%d4,%d6		| carry
	addx.w	%d2,%d5		| carry (%d2.w=0) (only affects low word in %d5)

|                  if needed, we can move %d7 to %a0 and use it as a temporary
	move.l	%d7,%a0		| %d7 is now scratch

| %d4 %d5 %d6 %d7
|        ah        carry up to %d5
	swap	%d0
	| ba:cd d0:d1
	| ef:gh a0:d3
	move	%d0,%d2
	mulu	%d3,%d2
	swap	%d2
	move.w	%d2,%d4		| d4 is high word (goes into d5)
	clr.w	%d2		| d2 is low word (goes into d6)
	add.l	%d2,%d6
	addx.l	%d4,%d5		| carry
	
| %d4 %d5 %d6 %d7
|          cg      carry up to %d5
	swap	%d1
	swap	%d3
	| ba:dc d0:d1
	| ef:hg a0:d3
	move	%d1,%d4
	mulu	%d3,%d4
	moveq.l	#0,%d2
	add.l	%d4,%d6
	addx.l	%d2,%d5		| carry

| %d4 %d5 %d6 %d7
|                  clear %d4
	move.l	%d2,%d4

| %d4 %d5 %d6 %d7
|                  compute ag in %d3
|      ag          carry into %d4
	mulu	%d0,%d3
	add.l	%d3,%d5
	addx.w	%d2,%d4		| carry

|                  %d3 is now available as a temporary
| --second half--
| %d4 %d5 %d6 %d7
|  ae
	move.l	%a0,%d2
	| ba:dc d0:d1
	| ef:XX d2:XX
	swap	%d2
	| ba:dc d0:d1
	| fe:XX d2:XX
	move	%d0,%d3
	mulu	%d2,%d3
	add.l	%d3,%d4

| %d4 %d5 %d6 %d7
|    be            carry up to %d4
	move.l	%d0,%d3
	swap	%d3
	mulu	%d2,%d3
	swap	%d3
	move.w	%d3,%d7		| d7 is high word (goes into d4)
	clr.w	%d3		| d3 is low word (goes into d5)
	add.l	%d3,%d5
	addx.l	%d7,%d4		| carry

| %d4 %d5 %d6 %d7
|    af            carry up to %d4
	swap	%d2
	| ba:dc d0:d1
	| ef:XX d2:XX
	move	%d2,%d3
	mulu	%d2,%d3
	swap	%d3
	move.w	%d3,%d7		| d7 is high word (goes into d4)
	clr.w	%d3		| d3 is low word (goes into d5)
	add.l	%d3,%d5
	addx.l	%d7,%d4		| carry


| %d4 %d5 %d6 %d7
|      bf          carry up to %d4
	swap	%d0
	| ab:dc d0:d1
	| ef:XX d2:XX
	move	%d0,%d3
	mulu	%d2,%d3
	moveq.l	#0,%d7
	add.l	%d3,%d5
	addx.w	%d7,%d4		| carry

|                  %d0 is now scratch
| %d4 %d5 %d6 %d7
|      ce          carry up to %d4
	swap	%d2
	| ab:dc d0:d1
	| fe:XX d2:XX
	move	%d2,%d3
	mulu	%d1,%d3
	add.l	%d3,%d5
	addx.l	%d7,%d4		| carry

| %d4 %d5 %d6 %d7
|        de        carry up to %d4
	swap	%d1
	| ab:cd d0:d1
	| fe:XX d2:XX
	move	%d1,%d3
	mulu	%d2,%d3
	swap	%d3
	move.l	%d7,%d0		| 0
	move.w	%d3,%d7		| d7 is high word (goes into d5)
	move.w	%d0,%d3		| d3 is low word (goes into d6)
	add.l	%d3,%d6
	addx.l	%d7,%d5		| carry
	addx.l	%d0,%d4		| carry

| %d4 %d5 %d6 %d7
|          df      carry up to %d4
	swap	%d2
	| ab:cd d0:d1
	| ef:XX d2:XX
	move	%d2,%d3
	mulu	%d1,%d3
	add.l	%d3,%d5
	addx.l	%d0,%d4		| carry

| %d4 %d5 %d6 %d7
|        cf        carry up to %d4
	swap	%d1
	| ab:dc d0:d1
	| ef:XX d2:XX
	move	%d1,%d3
	mulu	%d2,%d3
	swap	%d3
	move.l	%d0,%d7		| 0
	move.w	%d3,%d7		| d7 is high word (goes into d5)
	move.w	%d0,%d3		| d3 is low word (goes into d6)
	add.l	%d3,%d6
	addx.l	%d7,%d5		| carry
	addx.l	%d0,%d4		| carry

	move.l	%d4,%d0
	move.l	%d5,%d1
	move.l	%d6,%d2
	move.l	%a0,%d3

	| restore registers
	movem.l	(%sp)+,%d4-%d7
	rts


| version 3 optimizations!
| idea brought up by Lionel Debroux:
| add up aligned products and unaligned products separately:
| * on the one side:
|0055ddff +
|..88aa.. +
|..2277..
| * on the other side:
|.44ccee. +
|.1166bb. +
|...99...
|...33... 
|

| ae af ag ah     00 11 22 33
| be bf bg bh     44 55 66 77
| ce cf cg ch     88 99 aa bb
| de df dg dh     cc dd ee ff

| %d4 %d5 %d6 %d7
|     be  de  dg
|   * af  bg  ch  carry in %d4
|         cf
|         ah

| %d4 %d5 %d6 %d7
|  ae  bf  df  dh
|      ce  cg
|      ag  bh

| destroys %d4-%d7, %a0-%a1
mulu64c:
	| ab:cd  d0:d1
	| ef:gh  d2:d3

| %d4 %d5 %d6 %d7
|              dg
	swap	%d3
	| ab:cd  d0:d1
	| ef:hg  d2:d3
	move	%d3,%d7
	mulu	%d1,%d7

| %d4 %d5 %d6 %d7
|          de
	swap	%d2
	| ab:cd  d0:d1
	| fe:hg  d2:d3
	move	%d2,%d6
	mulu	%d1,%d6
	
| %d4 %d5 %d6 %d7
|      be
	move	%d0,%d5
	mulu	%d2,%d5

| %d4 %d5 %d6 %d7
|              ch carry through d4
	swap	%d1
	swap	%d3
	move.l	%d0,%a0		| save %d0
	| ab:dc  a0:d1
	| fe:gh  d2:d3
	moveq.l	#0,%d0
	move	%d1,%d4
	mulu	%d3,%d4
	add.l	%d4,%d7
	addx.l	%d0,%d6
	addx.l	%d0,%d5
	addx.l	%d0,%d0
	move.l	%d0,%d4

| %d4 %d5 %d6 %d7
|          cf     carry through d4
	move.l	%d7,%a1		| save %d7
	swap	%d2
	| ab:dc  a0:d1
	| ef:gh  d2:d3
	move	%d1,%d7
	mulu	%d2,%d7
	moveq.l	#0,%d0
	add.l	%d7,%d6
	addx.l	%d0,%d5
	addx.l	%d0,%d4

| %d4 %d5 %d6 %d7
|          bg     carry through d4
	move.l	%a0,%d0		| restore %d0
	move.l	%d1,%a0		| save %d1
	moveq.l	#0,%d1
	move.l	%d3,%d7
	swap	%d7
	| ab:dc  d0:a0
	| ef:gh  d2:d3
	mulu	%d0,%d7
	add.l	%d7,%d6
	addx.l	%d1,%d5
	addx.l	%d1,%d4

| %d4 %d5 %d6 %d7
|          ah     carry through d4
	swap	%d0
	| ba:dc  d0:a0
	| ef:gh  d2:d3
	move	%d0,%d7
	mulu	%d3,%d7
	add.l	%d7,%d6
	addx.l	%d1,%d5
	addx.l	%d1,%d4

| %d4 %d5 %d6 %d7
|      af         carry through d4
	move	%d0,%d7
	mulu	%d2,%d7
	add.l	%d7,%d5
	addx.l	%d1,%d4

| save unaligned part to stack
	move.w	%d1,-(%sp)	| push a zero word to align this part
	movem.l	%d4-%d6/%a1,-(%sp)	| %a1 is %d7
	addq	#2,%sp

| aligned part
| %d4 %d5 %d6 %d7
|              dh
	move.l	%a0,%d1		| restore %d1
	swap	%d1
	| ba:cd  d0:d1
	| ef:gh  d2:d3
	move	%d1,%d7
	mulu	%d3,%d7

| %d4 %d5 %d6 %d7
|          df
	move	%d1,%d6
	mulu	%d2,%d6

| %d4 %d5 %d6 %d7
|      bf
	move.l	%d0,%d5
	swap	%d5
	mulu	%d2,%d5

| %d4 %d5 %d6 %d7
|  ae
	move	%d0,%d4
	swap	%d2
	| ba:cd  d0:d1
	| fe:gh  d2:d3
	mulu	%d2,%d4

| %d4 %d5 %d6 %d7
|          cg
	move.l	%d7,%a1		| save %d7
	swap	%d1
	swap	%d3
	move.l	%d2,%a0		| save %d2
	moveq.l	#0,%d2
	| ba:dc  d0:d1
	| fe:hg  a0:d3
	move	%d1,%d7
	mulu	%d3,%d7
	add.l	%d7,%d6
	addx.l	%d2,%d5
	addx.l	%d2,%d4

| %d4 %d5 %d6 %d7
|      ce
	move	%a0,%d7
	mulu	%d1,%d7
	add.l	%d7,%d5
	addx.l	%d2,%d4

| %d4 %d5 %d6 %d7
|      ag
	move	%d0,%d7
	mulu	%d3,%d7
	add.l	%d7,%d5
	addx.l	%d2,%d4

| %d4 %d5 %d6 %d7
|          bh
	swap	%d0
	swap	%d3
	| ab:dc  d0:d1
	| fe:gh  a0:d3
	move	%d0,%d7
	mulu	%d3,%d7
	add.l	%d7,%d6
	addx.l	%d2,%d5
	addx.l	%d2,%d4

| restore unaligned part and add it to aligned part
	movem.l	(%sp)+,%d0-%d3
	add.l	%a1,%d3		| %d7
	addx.l	%d6,%d2
	addx.l	%d5,%d1
	addx.l	%d4,%d0

| result is in %d0-%d3

	rts
