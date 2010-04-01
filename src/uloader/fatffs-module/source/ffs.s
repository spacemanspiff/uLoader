/* FFS/ES Interface (c) 2010 Hermes / www.elotrolado.net */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*************************************************************************************/	
/* FFS input/output                                                                  */
/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_unk
addr_ffs_unk:
	ldr     r0, = old_ffs_tab_jump
	ldr	r0, [r0]
	nop
	ldr	r0, [r0, #0]
	nop
	mov	pc, r0

/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_open
addr_ffs_open:
	add	r0, r4, #0
	push	{r1-r7}
	push    {r0}
	mov	r1, sp
	bl	ffs_open
	pop     {r3}
	cmp     r3, #0
	bne	addr_ffs_exit	
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #4]
	pop	{r1-r7}
	mov	pc, r0


/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_close
addr_ffs_close:
	add	r0, r4, #0
	push	{r1-r7}
	bl	ffs_close

	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #8]
	pop	{r1-r7}
	mov	pc, r0

/*************************************************************************************/	

addr_ffs_exit:
	pop	{r1-r7}
	add     r1, r0, #0
	ldr     r0, =old_ffs_reentry
	ldr	r0, [r0]
	mov	pc, r0

/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_read
addr_ffs_read:
	add	r0, r4, #0
	push	{r1-r7}
	bl	ffs_read
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #12]
	pop	{r1-r7}
	mov	pc, r0

/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_write
addr_ffs_write:
	add	r0, r4, #0
	push	{r1-r7}
	bl	ffs_write	
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #16]
	pop	{r1-r7}
	mov	pc, r0

/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_seek
addr_ffs_seek:
	add	r0, r4, #0
	push	{r1-r7}
	bl	ffs_seek
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #20]
	pop	{r1-r7}
	mov	pc, r0

/*************************************************************************************/	

	.align 4
	.code 16
	.thumb
	.global addr_ffs_ioctl
addr_ffs_ioctl:
	add	r0, r4, #0
	push	{r1-r7}
	push    {r0}
	mov	r1, sp
	bl	ffs_ioctl
	pop     {r3}
      
	
	cmp     r3, #0
	bne	addr_ffs_exit	
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #24]
	pop	{r1-r7}
	mov	pc, r0

/*************************************************************************************/

	.align 4
	.code 16
	.thumb
	.global addr_ffs_ioctlv
addr_ffs_ioctlv:
	add	r0, r4, #0
	push	{r1-r7}
	push    {r0}
	mov	r1, sp
	bl	ffs_ioctlv
	pop	{r3}

	cmp     r3, #0
	bne	addr_ffs_exit	
	cmp     r0, #0
	bge	addr_ffs_exit

// original FFS jump
	ldr     r0, = old_ffs_tab_jump
	ldr	r3, [r0]
	ldr	r0, [r3, #28]
	pop	{r1-r7}
	mov	pc, r0


/*************************************************************************************/	
/* SYSCALL OPEN                                                                      */
/*************************************************************************************/	

/* NOTE: the original vector works in thumb mode, but i use a bx pc and ldr pc, =addr  to jump without modify any register
   Then i needs to change to the thub mode again correctly
*/
	.align 4
	.code 32
	.arm
	.global in_syscall_open
in_syscall_open:
	//push	{r4-r7,lr}
	stmfd	sp!, {r4-r7,lr}
	stmfd	sp!, {r8-r11}
	stmfd	sp!, {r2-r3}
	nop
	ldr	r4, =in_syscall_open_thumb+1
	bx	r4

	.align 4
	.code 16
	.thumb
in_syscall_open_thumb:
	
        bl	syscall_open_func
        ldr     r2, = syscall_open_mode
	ldr     r1, [r2]
	pop	{r2-r3}
	ldr	r4,= out_syscall_open_thumb
	ldr	r4,[r4]
	nop
	mov	pc, r4
	
/*************************************************************************************/	
/* ES ioctlv input/output                                                            */
/*************************************************************************************/	

	.align 4
	.code 16
	.global in_ES_ioctlv
	.thumb_func
in_ES_ioctlv:
	
	push	{r2-r6}
	push	{lr}
	bl	ES_ioctlv
	pop	{r1}
	pop	{r2-r6}
	bx	r1

	.global out_ES_ioctlv
	.thumb_func
out_ES_ioctlv:
	push	{r4-r6,lr}
	sub	sp, sp, #0x20
	ldr	r5, [r0,#8]
	add	r1, r0, #0
	ldr	r3, = 0x201000D5
	bx	r3
