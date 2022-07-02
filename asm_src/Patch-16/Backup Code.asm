; BACKUP OF CODE FOR HIJACKED TABLES
; DO NOT DELETE

;* here is the starting point of the new hijack code here only if AUDCTLMANUALSET is also enabled, otherwise, skip it entirely!
;* major improvements: use of BIT instructions from a small lookup table
;* much better structure, cleaner code, etc	
;* potential improvement? a lot... But this will be for another day, hehehe

	IFT FEAT_FULL_TABLES_HIJACK||FEAT_AGGRESSIVE_INIT			; this should be for the entire block here...
	
	IFT FEAT_FULL_TABLES_HIJACK&&!FEAT_AGGRESSIVE_INIT			; 6
get_audctl	

	IFT TRACKS>4		; stereo mode					; 3
	cpx #4			; are we in the right POKEY channels?
	bcc get_audctl_mono	; if x is lower than 4, we are not
get_audctl_stereo	
	lda v_audctl2		; load the right POKEY AUDCTL otherwise
	
	IFT FEAT_FULL_16BIT||FEAT_BASS16					; 1
	cpx #7			; channel 4?
	beq check_join_34
	EIF									; 1
	
	cpx #6			; channel 3?
	beq check_ch3_179
	
	IFT FEAT_FULL_16BIT||FEAT_BASS16					; 2
	cpx #5			; channel 2?
	beq check_join_12
	bne newp16start		; x == 4, unconditional
	ELS
	cpx #4			; channel 1?
	beq newp16start
	jmp check_clock15	; if not channel 1, skip to the 15khz check
	EIF									; 2
	
	EIF									; 3
	
get_audctl_mono
	lda v_audctl		; left POKEY AUDCTL, by default
	
	IFT FEAT_FULL_16BIT||FEAT_BASS16					; 4
	cpx #3			; channel 4?
	beq check_join_34
	EIF									; 4
	
	cpx #2			; channel 3?
	beq check_ch3_179
	
	IFT FEAT_FULL_16BIT||FEAT_BASS16					; 5
	cpx #1			; channel 2?
	beq check_join_12
	ELS
	cpx #0			; channel 1?
	beq newp16start
	jmp check_clock15	; if not channel 1, skip to the 15khz check
	EIF									; 5
	
	EIF									; 6
	
	
;---------------------------------------------------------------------------------;

; if x is equal to 0 or 4, it will always be channel 1, a redundant index check is skipped due to this.	

newp16start
check_ch1_179
	bit CH1_179		; bit 6, or #$40, 1.79mhz mode?
;	beq check_clock15	; nope, skip to the 15khz checks
	bne check_ch1_179_a
	jmp check_clock15
check_ch1_179_a
	IFT FEAT_FULL_16BIT||FEAT_BASS16
	bit JOIN_12		; bit 4, or #$10
;	beq hijack_179a		; all good, load the 1.79mhz tables and run the channel 1 specific code for Sawtooth checks
	bne ch1_179_failsafe
	jmp hijack_179a
ch1_179_failsafe
	jmp hellno		; failsafe, 16-bit shouldn't output sound in this channel!
	ELS
	bne hijack_179a
	EIF
	
check_ch3_179	
	bit CH3_179		; bit 5, or #$20, 1.79mhz mode?
;	beq check_clock15	; nope, skip to the 15khz checks
	bne check_ch3_179_a
	jmp check_clock15
check_ch3_179_a
	IFT FEAT_FULL_16BIT||FEAT_BASS16
	bit JOIN_34		; bit 3, or #$08
;	beq hijack_179b		; all good, load the 1.79mhz tables, also skipping all the channel 1 code here
	bne ch3_179_failsafe
	jmp hijack_179b
ch3_179_failsafe
	jmp hellno		; failsafe, 16-bit shouldn't output sound in this channel!
	ELS
	bne hijack_179b
	EIF
	
	IFT FEAT_FULL_16BIT||FEAT_BASS16
check_join_12
	IFT FEAT_BASS16
	cpy #$06		; are we using Distortion 6 specifically?
	IFT FEAT_FULL_16BIT
	bne check_join_12a
	ELS
	bne check_clock15
	EIF
	bit HPF_CH2
	bne do_bass16_12	; always process BASS16 if the filter CH2+4 is enabled	
	pha 			; backup the AUDCTL value to the stack first
	lda tmp			; volume value backup
	beq abort_bass16	; if not zero, continue, else, no 16-bit hijacking
	pla
do_bass16_12
	ora #$50
	bne bass16_audctl_done	; unconditional
	EIF
check_join_12a	
	IFT FEAT_FULL_16BIT
	bit JOIN_12		; bit 4, or #$10
	beq check_clock15	; nope, skip to the 15khz checks
	bit CH1_179		; bit 6, or #$40, 1.79mhz mode?
	beq check_clock15	; nope, skip to the 15khz checks
	jmp hijack_16hi		; yes, load the MSB 16-bit tables
	EIF
	
check_join_34
	IFT FEAT_BASS16
	cpy #$06		; are we using Distortion 6 specifically?
	IFT FEAT_FULL_16BIT
	bne check_join_34a
	ELS
	bne check_clock15
	EIF
	bit HPF_CH2
	bne do_bass16_34	; always process BASS16 if the filter CH2+4 is enabled
	pha 			; backup the AUDCTL value to the stack first
	lda tmp			; volume value backup
	beq abort_bass16	; if not zero, continue, else, no 16-bit hijacking
	pla
do_bass16_34
	ora #$28
	bne bass16_audctl_done	; unconditional
	EIF
check_join_34a
	IFT FEAT_FULL_16BIT
	bit JOIN_34		; bit 3, or #$08
	beq check_clock15	; nope, skip to the 15khz checks
	bit CH3_179		; bit 5, or #$20, 1.79mhz mode?
	beq check_clock15	; nope, skip to the 15khz checks
	jmp hijack_16hi		; yes, load the MSB 16-bit tables
	EIF
	
	IFT FEAT_BASS16
abort_bass16
	pla			; get the AUDCTL backup from the stack
	jmp check_clock15	; go test the 15khz bit instead 
bass16_audctl_done
	IFT TRACKS>4		; stereo mode
	cpx #4			; are we in the right POKEY channels?
	bcc bass16_mono		; if x is lower than 4, we are not	
bass16_stereo
	sta v_audctl2		; right POKEY
	jmp hijack_16hi
	EIF
bass16_mono
	sta v_audctl		; left POKEY
	jmp hijack_16hi
	EIF
	EIF
	
check_clock15
	bit CLOCK15		; bit 0, or #$01, 15khz mode?
	bne hijack_15		; yes, load the 15khz tables
	jmp no_hijack		; all checks returned false, 64khz tables by default
	
hijack_15
	lda tabbeganddistor_15khz+1,y	; new method to fetch tables between entries, since they use steps of 2 bytes
	sta nr 
	lda #>frqtab_15khz
	sta nr+1
	jmp pp2syes
	
hijack_179a
	IFT FEAT_FULL_SAWTOOTH
	lda tmp
	beq hijack_179b			; volume 0 is immediate skip of sawtooth code
	cpy #$0A			; are we using Distortion A?
	bne hijack_179b			; if not Distortion A, failsafe branching, no hijack will occur in this case
	lda trackn_audctl,x		; what is the instrument AUDCTL value?
	and #$60			; AUDCTL #$20 + #$40 = AUDCTL #$60 --> 1.79mhz mode CH1 and CH3
	cmp #$60			; both MUST be enabled at the same time, in the same instrument, to prevent conflicts with the other channels!
	beq hijack_sawtooth		; if the AUDCTL value meets the requirements, branch to the sawtooth code, else, skip it entirely 
	EIF
	
hijack_179b
	lda tabbeganddistor_179mhz,y
	sta nr 
	lda #>frqtab_179mhz
	sta nr+1
	jmp pp2syes
	
hijack_sawtooth
	IFT FEAT_FULL_SAWTOOTH
	lda reg2
	bpl hijack_sawtooth_done	; AUTOFILTER command is off, skip overwriting the other things	
	eor #$80			; inverts the AUTOFILTER bit, so it does not overwrite things later
	sta reg2	
	IFT TRACKS>4			; stereo mode
	cpx #4				; are we in the right POKEY channels?
	bcc sawtooth_audctl_mono	; if x is lower than 4, we are not
	lda v_audctl2
	ora #4				; high pass filter, CH1
	sta v_audctl2	
	bne sawtooth_audctl_done	; unconditional
	EIF
sawtooth_audctl_mono	
	lda v_audctl
	ora #4				; high pass filter, CH1
	sta v_audctl
sawtooth_audctl_done
	IFT FEAT_COMMAND6		; Sawtooth CMD6 hack... could be optimised much better, or maybe moved into the commands? ---> 36 bytes saved if not used
	lda reg2
	and #$70
	cmp #$60			; CMD6
	bne hijack_sawtooth_done	; skip if the checks failed
	lda reg3
	beq hijack_sawtooth_done	; immediately skip if the value is 0, nothing will be changed
	bmi sawtooth_reverse		; negative values (#$80 to #$FF) will reverse the tables pointers, positive values (#$01 to #$7F) will set the tables pointers to normal
sawtooth_normal
	lda #<frqtabsawtooth_ch1
	sta saw_ch1+1
	lda #<frqtabsawtooth_ch2
	sta saw_ch2+1	
	jmp hijack_sawtooth_done
sawtooth_reverse
	lda #<frqtabsawtooth_ch1
	sta saw_ch2+1
	lda #<frqtabsawtooth_ch2
	sta saw_ch1+1	
	EIF
hijack_sawtooth_done
	lda #>frqtab_extra
	sta nr+1
	sta sawtoothtables+1		; for the other channel later, also used as a check if it is enabled
saw_ch1
	lda #<frqtabsawtooth_ch1
	sta nr 
saw_ch2	
	lda #<frqtabsawtooth_ch2
	sta sawtoothtables
	jmp pp2syes	
	EIF
	
hijack_16hi	
	IFT FEAT_FULL_16BIT||FEAT_BASS16
	IFT FEAT_COMMAND1
checksmd1_16b
	lda reg2
	and #$70
	cmp #$10			; CMD1
	bne do16bit			; continue if the checks passed
	dex
	lda #0				; force the LSB channel to have empty values as a failsafe
	sta trackn_audc,x
	sta trackn_audf,x
	inx
	jmp pp2syes			; skip the tables checks
	EIF
do16bit

	IFT FEAT_FULL_16BIT
	lda tabbeganddistor_16bit_hi,y
	sta nr 
	lda tabbeganddistor_16bit_lo+1,y	
	sta pointer16bitlo	
	ELS
	IFT FEAT_BASS16
	lda #<frqtabpure_hi
	sta nr
	lda #<frqtabpure_lo
	sta pointer16bitlo
	EIF
	EIF
		
	IFT FEAT_FULL_16BIT
	cpy #$0B			; what page should be used?
	bcc page_A_2_hi			; if y is lower than #$0B, we are using any Distortion between 0 to A , else, Distortion C or E are used
page_C_E_hi
	lda #>frqtab_16bit_C_E
	bcs page_store_hi		; carry flag still set, unconditional
page_A_2_hi	
;	cpy #$06			; are we using Distortion 6 specifically? This is a workaround code to make it use the correct data when used, as a fallback
;	beq page_C_E_hi			; it is so go back a little bit and get the correct LSB pointer 
page_A_2_hi_a
	lda #>frqtab_16bit_A_2
page_store_hi
	ELS
	IFT FEAT_BASS16
	lda #>frqtab_16bit_A_2
	EIF
	EIF
	sta nr+1
	sta pointer16bitlo+1		; this value can also be used to identify if 16-bit mode is active or not, since it will never be 0
	lda #0				
	sta trackn_audc-1,x		; update the next channel's AUDC early, it will always be volume 0, and Distortion won't matter
	beq pp2syes			; unconditional
hellno
	lda #0
	sta trackn_audc,x
	sta trackn_audf,x
	jmp ppnext			; skip this channel entirely	
	EIF	
	ELS
	
	IFT FEAT_BASS16			; if BASS16 is enabled, but not FULL_TABLES_HIJACK, this is the code that gets assembled instead
	cpy #$06
	bne no_hijack			; no 16-bit output
	lda tmp				; volume backup, if it's 0, the 16-bit hijack is immediately skipped
	IFT FEAT_AUDCTLMANUALSET
	beq check_filter_24		; try to catch up if the filter ch2+4 is enabled
	ELS
	beq no_hijack			; no 16-bit output
	EIF
	IFT TRACKS>4			; stereo mode
	cpx #4				; are we in the right POKEY channels?
	bcc check_bass16_mono		; if x is lower than 4, we are not
check_bass16_stereo	
	lda v_audctl2			; right POKEY AUDCTL
do_bass16_anyway_stereo
	cpx #7				; channel 4?
	beq join34_bass16_stereo
	cpx #5				; channel 2?
	bne no_hijack			; no 16-bit output
join12_bass16_stereo
	ora #$50			; JOIN_12 + CH1_179 bits 
	bne bass16_audctl_stereo	; unconditional
join34_bass16_stereo
	ora #$28			; JOIN_34 + CH3_179 bits 
bass16_audctl_stereo
	sta v_audctl2
	bne do_bass16			; unconditional
	EIF
check_bass16_mono	
	lda v_audctl			; left POKEY AUDCTL
do_bass16_anyway_mono
	cpx #3				; channel 4?
	beq join34_bass16_mono
	cpx #1				; channel 2?
	bne no_hijack			; no 16-bit output
join12_bass16_mono
	ora #$50			; JOIN_12 + CH1_179 bits 
	bne bass16_audctl_mono		; unconditional
join34_bass16_mono
	ora #$28			; JOIN_34 + CH3_179 bits 
bass16_audctl_mono
	sta v_audctl
do_bass16
	lda #<frqtabpure_hi
	sta nr
	lda #<frqtabpure_lo
	sta pointer16bitlo		
	lda #>frqtab_16bit_A_2
	sta nr+1
	sta pointer16bitlo+1		; this value can also be used to identify if 16-bit mode is active or not, since it will never be 0
	lda #0				
	sta trackn_audc-1,x		; update the next channel's AUDC early, it will always be volume 0, and Distortion won't matter
	beq pp2syes			; unconditional
	IFT FEAT_AUDCTLMANUALSET
check_filter_24
	IFT TRACKS>4			; stereo mode
	cpx #4				; are we in the right POKEY channels?
	bcc get_audctl_bass16_mono	; if x is lower than 4, we are not
	lda v_audctl2	
	bit HPF_CH2
	bne do_bass16_anyway_stereo	; filter takes priority, do it anyway, else, no 16-bit output
	beq no_hijack			
	EIF
get_audctl_bass16_mono	
	lda v_audctl
	bit HPF_CH2
	bne do_bass16_anyway_mono	; filter takes priority, do it anyway, else, no 16-bit output
	EIF
	EIF
	EIF
	
	
;----------------------------------------------------------------------------------------------------------------------------------------------------------;

pp2sno
	lda reg2
	and #$0e
	tay
	
;* start of yet another method to hijack tables...
;* this should be much, much more efficient this time!

	IFT FEAT_BASS16
	cpy #$06			; distortion 6?
	IFT FEAT_AGGRESSIVE_INIT&&FEAT_FULL_TABLES_HIJACK
	bne hijack_start		; if no, skip the BASS16 code and condinue
	lda trackn_flags,x		; instrument flags
	cmp #$80			; 16-bit in valid channels ONLY
	bne test_bass16			; flag not set, so it may not be valid, so the index x test must be performed
	jmp do_bass16			; skip all the channels checks and process the BASS16 code immediately
test_bass16	
	lda tmp				; volume backup
	beq hijack_start		; volume 0 is immediate skip of BASS16 code
	jmp hijack_bass16		; process the remaining BASS16 code from here
	ELS 
	bne no_hijack			; if no, skip the BASS16 code and condinue
	EIF
	EIF
	 
hijack_start
	IFT FEAT_AGGRESSIVE_INIT&&FEAT_FULL_TABLES_HIJACK
	lda trackn_flags,x		; load the new aggressive flag in the accumulator
	beq test_none			; if 0, no flag set, branch to no hijack immediately
	bpl test_179			; if #$01-#$7F, branch to test 1.79mhz, sawtooth, then 15khz mode
	IFT FEAT_FULL_16BIT
	bmi test_16b			; if #$80-#$FF, branch to test 16-bit
	EIF
test_none
	jmp no_hijack			; if any comparison failed, jump from here to no hijack	
	IFT FEAT_FULL_16BIT
test_16b
	cmp #$80			
	beq hijack_16			; if #$80, 16-bit in VALID channels only, no further test necessary
	jmp hellno			; if #$FF, something went wrong, abort the channel immediately and process the next one
	EIF
test_179
	cmp #$7F			
	beq hijack_179			; if #$7F, 1.79mhz mode in VALID channels only, no further test necessary
test_saw
	IFT FEAT_FULL_SAWTOOTH	
	cmp #$64	
	beq hijack_sawtooth		; if #$64, Sawtooth/Triangle in VALID channels only, few extra tests will be done further
	EIF
test_15
	cmp #$01 
	bne test_none			; if #$01, 15khz mode in any channel, no further test necessary, else, branch to no hijack
hijack_15
	lda tabbeganddistor_15khz+1,y	; new method to fetch tables between entries, since they use steps of 2 bytes
	sta nr 
	lda #>frqtab_15khz
	sta nr+1
	jmp pp2syes	
hijack_sawtooth
	IFT FEAT_FULL_SAWTOOTH	
	lda tmp
	beq hijack_179			; volume 0 is immediate skip of sawtooth code
	cpy #$0A			; are we using Distortion A?
	beq hijack_sawtooth_a		; if the requirementa are met, branch to the sawtooth code, else, process 1.79mhz mode like normal
	EIF
hijack_179
	lda tabbeganddistor_179mhz,y
	sta nr 
	lda #>frqtab_179mhz
	sta nr+1
	jmp pp2syes	
hijack_16	
	IFT FEAT_FULL_16BIT	
	lda tabbeganddistor_16bit_hi,y
	sta nr 
	lda tabbeganddistor_16bit_lo+1,y	
	sta pointer16bitlo	
	cpy #$0B			; what page should be used?
	bcc page_A_2_hi			; if y is lower than #$0B, we are using any Distortion between 0 to A , else, Distortion C or E are used
page_C_E_hi
	lda #>frqtab_16bit_C_E
	bcs page_store_hi		; carry flag still set, unconditional
page_A_2_hi	
	lda #>frqtab_16bit_A_2
page_store_hi
	sta nr+1
	sta pointer16bitlo+1		; this value can also be used to identify if 16-bit mode is active or not, since it will never be 0
	lda #0				
	sta trackn_audc-1,x		; update the next channel's AUDC early, it will always be volume 0, and Distortion won't matter
	jmp pp2syes 	
	EIF	
hijack_sawtooth_a
	IFT FEAT_FULL_SAWTOOTH
	lda reg2
	bpl hijack_sawtooth_done	; AUTOFILTER command is off, skip overwriting the other things	
	IFT FEAT_FILTER
	eor #$80			; inverts the AUTOFILTER bit, so it does not overwrite things later
	sta reg2			; store back to reg2 for later
	EIF
	lda #4				; high pass filter, CH1+3
	IFT TRACKS>4			; stereo mode
	cpx #4				; are we in the right POKEY channels?
	bcc sawtooth_audctl_mono	; if x is lower than 4, we are not
	ora v_audctl2			; combine the existing AUDCTL value to it
	sta v_audctl2			; store the new AUDCTL value, right POKEY
	bne sawtooth_audctl_done	; unconditional
	EIF
sawtooth_audctl_mono	
	ora v_audctl			; combine the existing AUDCTL value to it
	sta v_audctl			; store the new AUDCTL value, left POKEY
sawtooth_audctl_done
	IFT FEAT_COMMAND6		; Sawtooth CMD6 hack... could be optimised much better, or maybe moved into the commands? 
	lda reg2
	and #$70
	cmp #$60			; CMD6
	bne hijack_sawtooth_done	; skip if the checks failed
	lda reg3
	beq hijack_sawtooth_done	; immediately skip if the value is 0, nothing will be changed
	bmi sawtooth_reverse		; negative values (#$80 to #$FF) will reverse the tables pointers, positive values (#$01 to #$7F) will set the tables pointers to normal
sawtooth_normal
	lda #<frqtabsawtooth_ch1
	sta saw_ch1+1
	lda #<frqtabsawtooth_ch2
	sta saw_ch2+1	
	jmp hijack_sawtooth_done
sawtooth_reverse
	lda #<frqtabsawtooth_ch1
	sta saw_ch2+1
	lda #<frqtabsawtooth_ch2
	sta saw_ch1+1	
	EIF
hijack_sawtooth_done
	lda #>frqtab_extra
	sta nr+1
	sta sawtoothtables+1		; for the other channel later, also used as a check if it is enabled
saw_ch1
	lda #<frqtabsawtooth_ch1
	sta nr 
saw_ch2	
	lda #<frqtabsawtooth_ch2
	sta sawtoothtables
	jmp pp2syes	
	EIF
hellno
	IFT FEAT_FULL_16BIT||FEAT_BASS16
	lda #0
	sta trackn_audc,x
	sta trackn_audf,x
	jmp ppnext			; skip this channel entirely
	EIF
	EIF				;;;aggressive code ends here!

hijack_bass16	
	IFT FEAT_BASS16	
	IFT TRACKS>4			; stereo mode
	cpx #4				; are we in the right POKEY channels?
	bcc test_bass16_ch4_mono	; if x is lower than 4, we are not
test_bass16_ch4_stereo
	lda v_audctl2			; right POKEY AUDCTL
	cpx #7				; channel 4?
	beq join34_bass16_stereo
	cpx #5				; channel 2?
	beq join12_bass16_stereo
	jmp hijack_start		; no valid channel detected
	EIF
test_bass16_ch4_mono	
	lda v_audctl
	cpx #3				; channel 4?
	beq join34_bass16_mono
test_bass16_ch2_mono
	cpx #1				; channel 2?
	beq join12_bass16_mono	
	jmp hijack_start		; no valid channel detected
	IFT TRACKS>4			; stereo mode
join12_bass16_stereo
	ora #$50			; JOIN_12 + CH1_179 bits 
	bne bass16_audctl_done_stereo	; unconditional
join34_bass16_stereo
	ora #$28			; JOIN_34 + CH3_179 bits 
bass16_audctl_done_stereo	
	sta v_audctl2
	bne do_bass16			; process the remaining BASS16 code from here
	EIF
join12_bass16_mono
	ora #$50			; JOIN_12 + CH1_179 bits 
	bne bass16_audctl_done_mono
join34_bass16_mono
	ora #$28			; JOIN_34 + CH3_179 bits 
bass16_audctl_done_mono	
	sta v_audctl	
do_bass16	
	IFT FEAT_COMMAND6&&FEAT_FULL_16BIT
	lda reg2			; Distortion and Commands
	and #$70			; leave only the Commands bits
	cmp #$60			; CMD6?
	bne do_bass16_a			; skip if not CMD6
	lda reg3			; XY parameter
	beq do_bass16_a			; immediately skip if the value is 0, nothing will be changed
	and #$0E			; strip away all unwanted bits, left nybble will not affect anything
do_bass16_cmd6
	tay				; transfer the new distortion value to index y for the next parts
	sta do_bass16_a+1		; PERMANENTLY changed the value until a new CMD6 value is read
	bne do_bass16_b			; unconditional
	EIF
do_bass16_a
	IFT FEAT_FULL_16BIT
	ldy #$0A			; Distortion A
do_bass16_b
	jmp hijack_16			; process the remaining of the 16-bit code from there
	ELS
	lda #>frqtab_16bit_A_2
	sta nr+1
	sta pointer16bitlo+1		; this value can also be used to identify if 16-bit mode is active or not, since it will never be 0
bass16_ch1	
	lda #<frqtabpure_hi
	sta nr
bass16_ch2
	lda #<frqtabpure_lo
	sta pointer16bitlo	
do_bass16_c	
	lda #0				
	sta trackn_audc-1,x		; update the next channel's AUDC early, it will always be volume 0, and Distortion won't matter
	lda tmp
bass16_dist
	ora #$A0			; Distortion A
	jmp pp2sdone
	EIF
	EIF	
	
no_hijack
	lda #>frqtab_64khz
	sta nr+1
	lda tabbeganddistor_64khz,y
	sta nr
pp2syes	
	lda tmp
	ora DISTORTIONS+1,y		; new method to fetch distortions between entries, since they use steps of 2 bytes
pp2sdone	
	sta trackn_audc,x
	
	EIF				; DONTUSEME!!!!!
	


;;;;;;;;;;;;;;;;;;;;;;;;;;;

	IFT FEAT_TABLE_MANUAL
	lda trackn_pointertable,x
	beq pp2sno
	tay
	and #$0F
	ora #$B0
	sta nr+1	;Bx00, where x is manipulated
	
; hack for Reverse-16 proof of concept, won't be used later		
	lda #0
	cpx #1
	bne stayzero
; end hack

	tya
	and #$C0
stayzero	
	sta nr		;B0x0, where is manipulated
	
; hack for Reverse-16 proof of concept, won't be used later	 
	cpx #0
	bne getdist
	lda tmp
	ora #$C0	
	bne pp2sdone	; unconditional
getdist	
; end of Reverse-16 hack, what is below should always be the same 

	lda reg2
	and #$0e
	tay
	jmp pp2syes
	EIF
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;

;* ORIGINAL

	IFT STEREOMODE==1		;* L1 L2 L3 L4 R1 R2 R3 R4
	lda trackn_audf+0+4
	ldx trackn_audf+0
	sta $d210
	stx $d200
	lda trackn_audc+0+4
	ldx trackn_audc+0
	sta $d211
	stx $d201
	lda trackn_audf+1+4
	ldx trackn_audf+1
	sta $d212
	stx $d202
	lda trackn_audc+1+4
	ldx trackn_audc+1
	sta $d213
	stx $d203
	lda trackn_audf+2+4
	ldx trackn_audf+2
	sta $d214
	stx $d204
	lda trackn_audc+2+4
	ldx trackn_audc+2
	sta $d215
	stx $d205
	lda trackn_audf+3+4
	ldx trackn_audf+3
	sta $d216
	stx $d206
	lda trackn_audc+3+4
	ldx trackn_audc+3
	sta $d217
	stx $d207
	lda #0
v_audctl2 equ *-1
	sty $d208
	sta $d218
	IFT FEAT_TWO_TONE
	lda #$03
v_skctl equ *-1	
	ldy #$03
v_skctl2 equ *-1
	sta $d20f	
	sty $d21f		
	EIF
	
;* OFFSET CHIPS

	IFT STEREOMODE==1		;* L1 L2 L3 L4 R1 R2 R3 R4
	lda trackn_audf+0
	ldx trackn_audc+0
	sta $d200
	stx $d201
	lda trackn_audf+1
	ldx trackn_audc+1
	sta $d200+2
	stx $d201+2
	lda trackn_audf+2
	ldx trackn_audc+2
	sta $d200+4
	stx $d201+4
	lda trackn_audf+3
	ldx trackn_audc+3
	sta $d200+6
	stx $d201+6
	sty $d208	
	IFT FEAT_TWO_TONE
	lda #$03
v_skctl equ *-1	
 	sta $d20f
 	EIF
	ldy #0
v_audctl2 equ *-1
	lda trackn_audf+0+4
	ldx trackn_audc+0+4
	sta $d210
	stx $d211
	lda trackn_audf+1+4
	ldx trackn_audc+1+4
	sta $d210+2
	stx $d211+2
	lda trackn_audf+2+4
	ldx trackn_audc+2+4
	sta $d210+4
	stx $d211+4
	lda trackn_audf+3+4
	ldx trackn_audc+3+4
	sta $d210+6
	stx $d211+6
	sty $d218
	IFT FEAT_TWO_TONE
	lda #$03
v_skctl2 equ *-1
	sta $d21f	
	EIF
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;* start of the tuning tables code... Should be much faster and efficient this time...
;* reg1 could be used for a temporary variable now...

do_tuning_tables
	lda reg2		; Distortion and Command
	and #$0E		; Only keep the Distortion bits
	tay			; will be used for the tables MSB pointer, and the Distortion itself	
	IFT FEAT_BASS16
	cpy #$06		; Distortion 6?
	bne check_flags		; no check if not equal
	lda tmp			; volume backup
	beq check_flags		; no check if equal
	jmp do_bass16		;* process the BASS16 code there
	EIF
check_flags
	lda trackn_flags,x	; flags previously set during the aggressive rmt_p3 initialisation, they are unconditional
	beq store_tables_lsb	; #$00 = 64khz flag, unconditional
	IFT FEAT_FULL_16BIT 
	bmi check_valid_16bit	; #$80 = Valid 16-bit flag, #$FF = Invalid 16-bit flag, unconditional
	EIF
	cmp #$40		; #$40 = Normal 1.79mhz flag
	beq store_tables_lsb	; the LSB pointer is also the flag value
	IFT FEAT_FULL_SAWTOOTH
	bcc check_15khz		; below #$40
	cpy #$0A		; Distortion A?
	beq go_saw		; if equal, Sawtooth!
no_saw
	lda #$40		; normal 1.79mhz flag
	bne store_tables_lsb	; the LSB pointer is also the flag value
go_saw
	lda tmp			; volume backup
	beq no_saw		; no sawtooth at volume 0!
	jmp do_sawtooth		;* process the Sawtooth code there
	EIF
check_15khz			; #$01 = 15khz flag, unconditional
	cpy #$0C		; are we in Distortion C/E?
	bcs check_15khz_b	; if equal or above, use the Distortion C 15khz table, else, use the Distortion A 15khz table
	lda #<frqtabpure_15khz	; Distortion A 15khz table LSB
	bcc check_15khz_c	; carry flag still set
check_15khz_b
	lda #<frqtabbuzzy_15khz	; Distortion C 15khz table LSB
check_15khz_c
	sta nr
	lda #>PAGE_EXTRA_0	; tables MSB pointer, it's the same one regardless of the table used
	bne store_tables_lsb_a	; unconditional 
	IFT FEAT_FULL_16BIT
check_valid_16bit
	cmp #$FF		; is the Invalid flag set?
	beq hellno		; if equal, everything is not fine!
valid_16bit 			; Valid 16-bit flag!
	IFT FEAT_BASS16
	cpy #$06		; Distortion 6?
	bne valid_16bit_a	; normal 16-bit mode
hellyes
	jmp process_bass16	; BASS16 happens even with volume 0!
	EIF
valid_16bit_a
	sta nr			; 16-bit hi LSB
	lda #$C0		; 64 bytes further for the 16-bit LSB table
	sta pointer16bitlo  	; 16-bit lo LSB
	lda #0			; no volume output, Distortion won't matter
	sta trackn_audc-1,x	; process the next channel's AUDC early, no sound is wanted  
	lda TABLES_MSB,y
	sta pointer16bitlo+1	; 16-bit lo MSB
	bne store_tables_lsb_a	; unconditional, finish in the middle of the 64khz/1.79mhz branch
	EIF
	IFT FEAT_FULL_16BIT||FEAT_BASS16
hellno				; Invalid 16-bit flag!
	lda #0			; clear the channel values
	sta trackn_audc,x
	sta trackn_audf,x
	jmp ppnext 		; skip this channel entirely 
	EIF
store_tables_lsb
	sta nr			; tables LSB pointer, divided in slices of 64 bytes each
	lda TABLES_MSB,y
store_tables_lsb_a
	sta nr+1		; tables MSB pointer, which is the memory page used for tables divided based on their Distortion
store_tables_lsb_b
	lda tmp			; volume value backup
	ora DISTORTIONS,y	; merge the Distortion and Volume values
store_tables_lsb_c
	sta trackn_audc,x	; update the channel's AUDC for the next SetPokey subroutine call
	
;* end of tuning tables code...

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;-----------------
	
; draw the volume blocks

begindraw
	mwa #vol_ch1 DISPLAY 
begindraw1	
	ldy #0
	ldx #0
ch_index equ *-1
	lda trackn_audc,x
	
	and #$0F
	tax
	bne getcolourfull
	beq drawloop
getcolourfull
	lda #255	; red?
	
	bne drawloop1
drawloop
	ldx #15
drawloop1
	sta (DISPLAY),y
	iny 
	cpy #15
	beq drawdone	
	dex
	bpl drawloop1
drawloop2
	cpy #15
	beq drawdone
	lda #0
drawloop3
	sta (DISPLAY),y
	iny 
	cpy #15
	bne drawloop3
drawdone
	inc ch_index
	lda #4
	cmp ch_index
	beq finishedloop
goloopagain
	lda DISPLAY
	add #20
	sta DISPLAY
	scc:inc DISPLAY+1
	jmp begindraw1 
finishedloop
	lda #0
	sta ch_index
	
;-----------------	

begindraw
	mwa #mode_6+2 DISPLAY	; set the position on screen, offset by 2 in order to be centered
	lda #$c0		; change the colour to red 
	sta colour_bar 
	ldx #TRACKS-1
	
;	; debug code!!!!
;	lda #0
;	ldy #80
;debug_draw
;	sta (DISPLAY),y 
;	iny
;	cpy #116
;	bne debug_draw 
;	; end debug!!!
	
begindraw1
	lda trackn_audc,x	; channel volume and distortion
	and #$0F
	beq reset_decay_a	; 0 = no volume to write into the buffer
	sta temp_volume		; self modifying code
	
begindraw2
	lda trackn_audf,x	; channel frequency
	eor #$FF		; invert the value, the pitch goes from lowest to highest from the left side
	:4 lsr @		; divide by 16
	tay			; transfer to Y

;	; more debug code!!!
;	:3 lsr @ 
;	pha
;	add #82
;	tay
;	lda hexchars,x 
;	sta (DISPLAY),y 
;	pla
;	lsr @
;	tay
;	; end debug!!!
	
begindraw3 
	lda #0
temp_volume equ *-1		; to hopefully speed up the operations without clogging more bytes
	cmp decay_buffer,y	; what is the volume level in memory?
	bcc reset_decay_a	; below the value in memory will be ignored
	beq reset_decay_a	; equal will also be ignored, no point using the same value twice 
reset_decay
	sta decay_buffer,y	; if above the buffer value, write the new value in memory, the decay is now reset for this column
reset_decay_a
	dex
	bpl begindraw1		; repeat until all channels are done 
	
do_index_line	
	inx 			; line index = 0, for a total of 4 lines 
	ldy #15			; 16 columns, including 0 
	
do_index_line_a
	lda decay_buffer,y	; volume value in the corresponding column 
	beq draw_nothing	; a value of 0 is immediately drawing a blank tile on screen 
	
do_index_line_b
	cpx #1
	bcc vol_12_to_15	; X = 0
	beq vol_8_to_11		; X = 1
	cpx #2
	beq vol_4_to_7		; X = 2, else, the last line is processed by default 

vol_0_to_3
	cmp #4
	bcs draw_4_bar 
	cmp #1			; must be equal or above
	beq draw_1_bar		; 1
	cmp #2
	beq draw_2_bar		; 2
	bne draw_3_bar
	
vol_4_to_7
	cmp #8
	bcs draw_4_bar 	
	cmp #5			; must be equal or above
	bcc draw_0_bar		; overwrite with a blank tile, always
	beq draw_1_bar		; 5
	cmp #6
	beq draw_2_bar		; 6
	bne draw_3_bar
	
vol_8_to_11
	cmp #12
	bcs draw_4_bar
	cmp #9			; must be equal or above
	bcc draw_0_bar		; overwrite with a blank tile, always
	beq draw_1_bar		; 9
	cmp #10
	beq draw_2_bar		; 10
	bne draw_3_bar
	
vol_12_to_15 
	cmp #15
	beq draw_3_bar
	cmp #13			; must be equal or above
	bcc draw_0_bar		; overwrite with a blank tile, always 
	beq draw_1_bar		; 13 

draw_2_bar
	lda #60
	bne draw_line1
draw_3_bar
	lda #27
	bne draw_line1
draw_4_bar			
	lda #5
	bne draw_line1
draw_0_bar
	lda #0
	beq draw_nothing
draw_1_bar
	lda #63 

draw_line1
	ora #0
colour_bar equ *-1 

draw_nothing
	sta (DISPLAY),y 
	dey
	bpl do_index_line_a	; continue until all columns were read
	cpx #3
	beq finishedloop	; all channels were done if equal 
	
goloopagain
	lda DISPLAY		; current memory address used for the process
	add #20			; mode 6 uses 20 characters 
	sta DISPLAY		; adding 20 will move the pointer to the next line
	scc:inc DISPLAY+1	; in case the boundary is crossed, the pointer MSB will increment as well
	
verify_line
	cpx #1
	bcc change_line23	; below 1 
change_line4
	lda #$40		; change the colour to green 
	bne colour_changed 
change_line23 
	lda #$00 		; change the colour to yellow 
colour_changed
	sta colour_bar		; new colour is set for the next line 
	jmp do_index_line 	; repeat the process for the next line until all lines were drawn  
	
decay_buffer
	:16 dta $00 
decay_speed
	dta SPEED		; set the speed of decay rate, 0 is no decay, 255 is the highest amount of delay (in frames) 

finishedloop
	ldy #0 			; reset value if needed
	ldx #15			; 16 columns index, including 0 
	
do_decay
	dec decay_speed
	bpl decay_done		; if value is positive, it's over, wait for the next frame 
reset_decay_speed
	lda #SPEED
	sta decay_speed		; reset the value in memory, for the next cycle
decay_again 
	lda decay_buffer,x
	beq decay_next		; 0 equals no decay 
	sub #RATE 
	bpl decay_again_a	; if positive, write the value in memory 
	tya
decay_again_a
	sta decay_buffer,x	; else, write 0 to it
decay_next
	dex			; next column index
	bpl decay_again		; repeat until all columns were done 
decay_done

;----------------


