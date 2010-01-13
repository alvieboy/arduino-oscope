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
        
	lds r20,_ZL6params+6		; Load flags
	lds r19,121                     ; Load ADCH
	
        lds r24,_ZL13dataBufferPtr      ; Load databuffer
	lds r25,(_ZL13dataBufferPtr)+1  ; pointer.
	
        ; Fast storedata path
        sbrc r20,4 			; Storing data ?
        rjmp .STOREDATA			; Yes, jump to store
	
        sbrs r20,6                      ; Start conversion ?
	rjmp .NOSTOREDATA                        ; No, jump
	
        ;sbiw r24,0                      ; Subtract zero. is dataBufferPtr 0?
	;brne .L4                        ; No, not zero, we're sampling
	;ori r20,lo8(16)                 ; Set STOREDATA

.STOREDATA:
	lds r30,_ZL10dataBuffer		; Load dataBuffer in Z
	lds r31,(_ZL10dataBuffer)+1    
	add r30,r24                     ; Add dataBufferPtr
	adc r31,r25                    
	st Z,r19                        ; Store ADC value
.L6:
	movw r18,r24
	subi r18,lo8(-(1))
	sbci r19,hi8(-(1))              ; Increment dataBufferPtr ?
	lds r24,_ZL6params+4            ; r24:r25 <- numSamples
	lds r25,(_ZL6params+4)+1
	cp r24,r18                      ; Compare lower
	cpc r25,r19                     ; and higher with carry
	brsh .L7                        ; greater than numSamples ? No, jump
	sbrs r20,4                      ; Storing data ?
	rjmp .NOSTOREDATA               ; No, jump
	ori r20,lo8(32)                 ; Yes, set conversion done
	andi r20,lo8(-65)               ; and clear startconversion

.NOSTOREDATA: ; .L8
	andi r20,lo8(-17)               ; Clear storedata
	sts (_ZL13dataBufferPtr)+1,__zero_reg__ ; Reset databufferptr
	sts _ZL13dataBufferPtr,__zero_reg__    
	sts _ZL6params+6,r20                    ; Store back flags

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

.STOREADCVALUE: ;.L4:
	;sbrs r18,4
	;rjmp .L6
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
