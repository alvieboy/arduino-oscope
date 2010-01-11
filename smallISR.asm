	push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	clr __zero_reg__
	push r18
	push r19
	push r20
	push r21
	push r24
	push r25
	push r30
	push r31
/* prologue: Signal */
/* frame size = 0 */
	lds r20,_ZL6params+6
	lds r19,121
	lds r24,_ZL13dataBufferPtr
	lds r25,(_ZL13dataBufferPtr)+1
	mov r18,r20
	sbrs r20,6
	rjmp .L4
	sbiw r24,0
	brne .L4
	ori r20,lo8(16)
.L5:
	lds r30,_ZL10dataBuffer
	lds r31,(_ZL10dataBuffer)+1
	add r30,r24
	adc r31,r25
	st Z,r19
.L6:
	movw r18,r24
	subi r18,lo8(-(1))
	sbci r19,hi8(-(1))
	lds r24,_ZL6params+4
	lds r25,(_ZL6params+4)+1
	cp r24,r18
	cpc r25,r19
	brsh .L7
	sbrs r20,4
	rjmp .L8
	ori r20,lo8(32)
	andi r20,lo8(-65)
.L8:
	andi r20,lo8(-17)
	sts (_ZL13dataBufferPtr)+1,__zero_reg__
	sts _ZL13dataBufferPtr,__zero_reg__
	sts _ZL6params+6,r20

	pop r31
	pop r30
	pop r25
	pop r24
	pop r21
	pop r20
	pop r19
	pop r18
	pop r0
	out __SREG__,r0
	pop r0
	pop __zero_reg__
	reti
.L4:
	sbrs r18,4
	rjmp .L6
	lds r30,_ZL10dataBuffer
	lds r31,(_ZL10dataBuffer)+1
	add r30,r24
	adc r31,r25
	st Z,r19
	rjmp .L6
.L7:
	sts (_ZL13dataBufferPtr)+1,r19
	sts _ZL13dataBufferPtr,r18
	rjmp .L9
