/*
 * Punix, Puny Unix kernel
 * Copyright 2005 Chris Williams
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

/*
 * Copyright (c) 1987,1997, Prentice Hall
 * All rights reserved.
 * 
 * Redistribution and use of the MINIX operating system in source and
 * binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 * 
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 * 
 *    * Neither the name of Prentice Hall nor the names of the software
 *      authors or contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PRENTICE HALL OR ANY AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TYPE_H_
#define _TYPE_H_

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;

typedef u32_t reg_t;		/* machine register */

/* The name and fields of this struct were chosen for PC compatibility. */
struct stackframe_s {
	reg_t retreg;		/* d0 */
	reg_t d1;
	reg_t d2;
	reg_t d3;
	reg_t d4;
	reg_t d5;
	reg_t d6;
	reg_t d7;
	reg_t a0;
	reg_t a1;
	reg_t a2;
	reg_t a3;
	reg_t a4;
	reg_t a5;
	reg_t fp;		/* also known as a6 */
	reg_t sp;		/* also known as a7 */
	reg_t pc;
	u16_t psw;
	u16_t dummy;		/* make size multiple of reg_t for system.c */
};

struct fsave {
  struct cpu_state {
	u16_t i_format;
	u32_t i_addr;
	u16_t i_state[4];
  } cpu_state;
  struct state_frame {
	u8_t frame_type;
	u8_t frame_size;
	u16_t reserved;
	u8_t frame[212];
  } state_frame;
  struct fpp_model {
	u32_t fpcr;
	u32_t fpsr;
	u32_t fpiar;
	struct fpN {
		u32_t high;
		u32_t low;
		u32_t mid;
	} fpN[8];
  } fpp_model;
};

#endif /* _TYPE_H_ */
