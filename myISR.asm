
	;push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	;clr __zero_reg__
	push r18
	push r19
	push r20
	push r21
	push r22
	push r23
	push r24
	push r25
	push r30
	push r31

; prologue: Signal 

	lds r20,_ZL6gflags				    ; Load gflags into r20
	lds r0, 121 						; Load ADHC value

	lds r24,holdoff   ; Load holdoff value
	tst r24								; Greater than zero ?
	brne .PERFORM_HOLDOFF;.L25          ; Yes, perform holdoff
	; No holdoff needed.
	sbrc r20,7							; Test if we're already triggered
	rjmp .TRIGGERED2 ;.L6                ; Yes, jump.
	lds r21,_ZL12triggerLevel			; Not triggered, load trigger level
	tst r21                             ; If triggerlevel is not zero then
	brne .CHECKTRIGGER ;.L26            ; must check trigger

.TRIGGERED: ;.L6:
	ori r20, lo8(-128)						; Set trigger flag. Might already be
    									; defined
.TRIGGERED2:                                        
	sts last,r0      ; Store in "last" 

	; Alvie2 ; sbrc r20,7							; Check again if triggered
	rjmp .ANALYSE_DATA ;.L27            ; Yes, perform analysis
;.L12:
.EXIT_SAVE_FLAGS:
	sts _ZL6gflags,r20					; Store flags back
;.L21:
.EXIT_NO_SAVE_FLAGS:
; epilogue start 

	pop r31
	pop r30
	pop r25
	pop r24
	pop r23
	pop r22
	pop r21
	pop r20
	pop r19
	pop r18
	pop r0
	out __SREG__,r0
	pop r0
	;pop __zero_reg__
	reti

;.L25:
.PERFORM_HOLDOFF:
	subi r24,lo8(-(-1))					; Holdoff in r24, decrement
	sts holdoff,r24   ; Store in memory, then 
	rjmp .EXIT_NO_SAVE_FLAGS;.L21       ; exit

;.L26:
.CHECKTRIGGER:

	lds r22,_ZL13autoTrigCount			; Load autotrigger into r22
	tst r22								; See if is zero
	breq .NO_AUTOTRIGGER ;.L7        	; Zero, no need to check auto
	lds r24,_ZL15autoTrigSamples		; Not zero, get autotrigsamples
	cp r22,r24 							; Compare, if count > samples then
	brsh .TRIGGERED ;.L6				; We are triggered
    
.NO_AUTOTRIGGER: ; L7:                  ; This one is pretty much stupid
	mov r24,r20     					; r24 <= gflags
	ldi r25,lo8(0)                      ; r25 == 0
	movw r18,r24                        ; r18,r19 = r24,r25.
	andi r18,lo8(1)                     ; and with 1 (INVERT)
	andi r19,hi8(1)                     ; and 0 (INVERT???)
	sbrc r20,0                          ; See if we have inverted trigger
	rjmp .TRIGGER_INVERTED ;.L9         ; Yes, inverted trigger
	; Alvie2 ; lds r24,121    						; Load ADCH into r24
	cp r0,r21 ; Alvie2 ;r24,r21                          ; compare with r21 (triggerLevel)
	brlo .+2                            ; Lower ? skip...
	rjmp .CHECKTRIGPOS; .L28            ; No, high or equal, branch


.CHECKAUTOTRIGGER: ;.L10:									; ... here
	lds r24,_ZL15autoTrigSamples        ; Load autoTriggerSamples
	tst r24								; Zero ?
	breq .NO_AUTOTRIGGER2 ; .L11        ; Yes, no need to compute autotrigger
	subi r22,lo8(-(1))                  ; Increment trigcount
	sts _ZL13autoTrigCount,r22			; Store it.

.NO_AUTOTRIGGER2: ;.L11:
	; Alvie2 ; lds r24,121							; Load ADCH into r24
	sts last, r0 ; Alvie2 ;r24      ; Store last
	rjmp .EXIT_SAVE_FLAGS 	 ;.L12      ; And bye bye.


.ANALYSE_DATA: ;.L27:
	mov r24,r20                         ; r24 <= gflags?
	ldi r25,lo8(0)                      ; r25 = 0
	sbrc r20,6                          ; START_CONVERSION requested ?
	rjmp .STARTCONVERSION ;.L23       ; Yes, check validity

	lds r18,dataBufferPtr          ; Load dataBufferPtr
	lds r19,(dataBufferPtr)+1	    ; into r19:r18

.CHECKSTOREDATA: ;.L13:
	;movw r22,r24                        ; ??
	;andi r22,lo8(16)                    ; ??  bit 4
	;andi r23,hi8(16)                    ; ??  zero
	sbrs r20,4                          ; awkward test: STORE_DATA set?
	rjmp .NOSTORE ;.L14                           ; No, no store data
    
	sbrc r20,1                          ; r24 was gflags... stupid gcc
	rjmp .DUALCHANNEL; .L29             ; Jump if dual channel

	lds r24,124							; Load ADMUX
	andi r24, lo8(-2)                      ; Clear channel
	sts 124,r24                         ; And save ADMUX

.DOSTOREDATA: ;.L16:
	; Alvie2 ; lds r24,121							; Load ADCH
	lds r30,_ZL10dataBuffer             ; Load data pointer into
	lds r31,(_ZL10dataBuffer)+1         ; r30 and r31
	add r30,r18                         ; add r19:r18(databuffer)
	adc r31,r19                        
	st Z, r0 ; Alvie2 ;r24                            ; Store ADCH into buffer
	lds r18,dataBufferPtr          ; Load pointer again
	lds r19,(dataBufferPtr)+1      ; into r19:r18
                                        ; This is stupid, is a non-increment
                                        ; op, should be st Z+ if we're to 
                                        ; refetch pointer.
                                        ; Or we miss correct op here.
.NOSTORE: ;.L14:
	subi r18,lo8(-(1))                  ; Increment bufptr
	sbci r19,hi8(-(1))                 

	sts (dataBufferPtr)+1,r1		; And store again
	sts dataBufferPtr,r18			; into memory
    
	lds r24,_ZL10numSamples             ; Load numsamples
	lds r25,(_ZL10numSamples)+1         ; into r25:r24
	cp r24,r18                          ; compare numsamples
	cpc r25,r19                         ; with bufferptr
	brlo .+2
	rjmp .EXIT_SAVE_FLAGS; :.L12        ; Not yet filled. Bye bye

	;or r22,r23                          ; Stupid. flags&BYTE_FLAG_STOREDATA
        sbrs r20,4
	breq .FINALIZE_STORE ; .L17

	ori r20,lo8(32)						; Set conversion_done
	andi r20,lo8(-65)                   ; Clear start_conversion

.FINALIZE_STORE: ;.L17:
	andi r20,lo8(111)                   ; Clear store_data adn triggered
	lds r24,124                         ; load ADMUX
	andi r24,lo8(-2)                    ; Clear channel
	sts 124,r24                         ; Store ADMUX
    
	lds r24,_ZL14holdoffSamples			; restore holdoff
	sts holdoff,r24   ; to samples
        
        clr r24 ; Alvie1
	sts _ZL13autoTrigCount,r24 ; Alvie1 ; __zero_reg__ ; Zeroe autotrigcount
	sts (dataBufferPtr)+1,r24 ; Alvie1 ;__zero_reg__ ; and
	sts dataBufferPtr,r24 ; Alvie1 ;__zero_reg__     ; databufferptr
	sbrs r20,0                          ; Is invert trigger ?
	rjmp .SET_LAST_HIGH ;.L18                           ; Yes, last must be 0xff
	sts last,r24 ; Alvie1 ; __zero_reg__  ; otherwise reset last
	rjmp .EXIT_SAVE_FLAGS ;.L12

.TRIGGER_INVERTED: ;.L9:
	tst r18
	brne .+2                            ; Is not trigger inverted?
	rjmp .CHECKAUTOTRIGGER; .L10                           ; no.... duh! you already checked.
	; Alvie2 ; lds r24,121                         ; Load ADCH
	cp r21,r0; Alvie2; r24                          ; Compare with triggerLevel
	brsh .+2                            ; >= ?
	rjmp .CHECKAUTOTRIGGER;.L10                           ; no, no need to trigger
	lds r24,last      ; Load last.
	cp r21,r24                          ; Is lower than last value ?
	brlo .+2                            ; Yes, lower... 
	rjmp .CHECKAUTOTRIGGER;.L10                               ; No, no trigger
	rjmp .TRIGGERED ;.L6                ; .. than last, go and trigger


.STARTCONVERSION: ; .L23:

	lds r18,dataBufferPtr	    ; We can only start conversion
	lds r19,(dataBufferPtr)+1      ; if we ended last buffer
        ; Alvie1
        clr r22
	cp r18,r22 ; Alvie1 ;__zero_reg__                
	cpc r19,r22 ; Alvie1 ;,__zero_reg__               
	breq .+2                            ; Yes, we did end
	rjmp .CHECKSTOREDATA ;.L13          ; No we did not end conversion

	ori r20,lo8(16)			    ; Lets start conv. Set STOREDATA
	mov r24,r20                         ; ?
	ldi r25,lo8(0)                      ; ?
	rjmp .CHECKSTOREDATA                ; .L13

.SET_LAST_HIGH: ;.L18:
	ldi r24,lo8(-1)				; Load 0xFF
	sts last,r24		; store in last
	rjmp .EXIT_SAVE_FLAGS ;.L12             ; and bye

.DUALCHANNEL: ;.L29:
	lds r24,124				; Load ADMUX
	ldi r25,lo8(1)                          ; Load 1
	eor r24,r25                             ; XOR
	sts 124,r24                             ; And store ADMUX
	rjmp .DOSTOREDATA ;.L16


.CHECKTRIGPOS: ;.L28:
	lds r24,last		; Compare with
	cp r24,r21				; last value. 
	brlo .+2                                ; less or equal ?
	rjmp .CHECKAUTOTRIGGER;.L10                               ; No...
	rjmp .TRIGGERED;.L6                     ; Yes.. trigger
