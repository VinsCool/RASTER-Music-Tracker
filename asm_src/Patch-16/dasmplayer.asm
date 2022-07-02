;************************************************;
;* Simple RMT Player                            *;
;* For SAP and XEX music exports from RMT	*;
;* Recreation from disassembled code            *;
;* Original version by Raster/C.P.U., 2003-2004 *;
;* Recreation by VinsCool                       *;
;* Version 5, 25-02-2022                        *;
;************************************************;

;---------------------------------------------------------------------------------------------------------------------------------------------;

;* start of dasmplayer definitions...

; Export format, Atari Executable (XEX/OBX) or SAP

EXPORTXEX	equ 1
EXPORTSAP	equ 0

	ERT [EXPORTXEX+EXPORTSAP]=0	;* No format defined!
	ERT [EXPORTXEX+EXPORTSAP]=2	;* Only 1 format can be defined!

; starting line for songs when loaded, useful for playing from different lines or subtunes

STARTLINE	equ 0

; playback speed will be adjusted accordingly in the other region

REGIONPLAYBACK	equ 1		; 0 => PAL
				; 1 => NTSC

; Stereo mode must be defined here instead of 'rmt_feat.a65'

STEREOMODE	equ 0
				;* 0 => compile RMTplayer for 4 tracks mono
				;* 1 => compile RMTplayer for 8 tracks stereo
				;* 2 => compile RMTplayer for 4 tracks stereo L1 R2 R3 L4
				;* 3 => compile RMTplayer for 4 tracks stereo L1 L2 R3 R4
				;* 4 => compile RMTplayer for 8 tracks Dual Mono LR1 LR2 LR3 LR4

; screen line for synchronization, important to set with a good value to get smooth execution

;VLINE		equ 7		; 16 is the default according to Raster's example player

; The target is to get the first rmtplay occuring on scanline 16 (NTSC) or ?? (PAL) for stability
; VLINE cannot be higher than 155! Otherwise, it will never work, and stay in an endless loop!
;
; NTSC 	->	VLINE must be: 	7 for 1x, 85 for 2x,
; PAL 	->	VLINE must be:	??
;
; NTSC2PAL ->	VLINE must be:	7 for 1x, 7 for 2x,
; PAL2NTSC ->	VLINE must be:	??

VLINE		equ 10		; nice round numbers fit well with multiples of 8 for every xVBI...

	ERT VLINE>155		; VLINE cannot be higher than 155!

; rasterbar colour

RASTERBAR	equ $69		; $69 is a nice purpleish hue

; VU Meter decay rate and speed

RATE		equ 1		; set the amount of volume decay is done, 0 is no decay, 15 is instant
SPEED		equ 1		; set the speed of decay rate, 0 is no decay, 255 is the highest amount of delay (in frames) 

; some memory addresses

DISPLAY equ $FE			; zeropage address of the Display List indirect memory address

MODUL	equ $4000		; address of RMT module
DOSVEC	equ $000A
RTCLOK	equ $0012 		; Real Time Clock
VVBLKI	equ $0222		; Vertical Blank Immediate (VBI) Register
SDMCTL	equ $022F 		; Shadow Direct Memory Access Control address
SDLSTL	equ $0230
COLOR0	equ $02C4
COLOR1	equ $02C5
COLOR2	equ $02C6
COLOR3	equ $02C7
COLOR4	equ $02C8
CH1	equ $02F2
CH	equ $02FC
PAL	equ $D014
COLPF0	equ $D016
COLPF1	equ $D017
COLPF2	equ $D018
COLPF3	equ $D019
COLBK	equ $D01A
KBCODE	equ $D209
SKSTAT	equ $D20F
VCOUNT	equ $D40B
rCHBASE equ $d409 		; character gfx address
WSYNC	equ $d40A 		; wait for hblank
NMIEN	equ $D40E

FONT    equ $bc00       	; custom font location

;* end of dasmplayer definitions...

;---------------------------------------------------------------------------------------------------------------------------------------------;

;* start of SAP format...

	IFT EXPORTSAP
	opt h- 
	icl 'sap.asm' 		; SAP plaintext data 
	opt h+ 

; assemble rmtplayr here... 

	icl 'rmtplayr.a65'	; execution address is $3400

; assemble SAP init here... used for region adjustment, seems exclusive to hardware(?) and Altirra emulator for now

	org $3E00		; same address used with Simple RMT Player because why not
	tax			; somehow the subtune is loaded in the accumulator at init
	lda subtune,x		; load a subtune based on the song line, indexed by x
	pha
region_init			; 50 Hz or 60 Hz?
	lda #0
	sta vcount		; reset the counter
	ldx #156		; default value for all regions
region_loop	
	lda vcount
	beq check_region	; vcount = 0, go to check_region and compare values
	tay			; backup the value in index y
	bne region_loop 	; repeat
check_region
	cpy #$9B		; compare index y to 155
	IFT REGIONPLAYBACK==0	; if the player region defined for PAL...
	bpl region_done		; negative result means the machine runs at 60hz
	ldx #131		; NTSC is detected, adjust the speed from PAL to NTSC (for some reason #130 skips a bit)
	ELI REGIONPLAYBACK==1	; else, if the player region defined for NTSC...
	bmi region_done		; positive result means the machine runs at 50hz
	ldx #187		; PAL is detected, adjust the speed from NTSC to PAL
	EIF			; endif
region_done
	stx $0490		; memory location used for screen synchronisation(?) in Altirra
	ldx #<MODUL		; low byte of RMT module to X reg
	ldy #>MODUL		; hi byte of RMT module to Y reg
	pla
	jsr rmt_init
	tay			; use the instrument speed as an offset
	lda tabpp-1,y		; load from the line counter spacing table
	sta $0406		; lines between each play(?) in Altirra
	rts
tabpp       
	dta 156,78,52,39	
subtune
;	dta $00,$0A,$20		; subtune line, can be as many as wanted, however, the SAP specs limit at 32
	EIF
	
;* end of SAP format...

;---------------------------------------------------------------------------------------------------------------------------------------------;

;* start of XEX format...

	IFT EXPORTXEX
	
; assemble rmtplayr here... 

	icl 'rmtplayr.a65'	; execution address is $3400
	
;-----------------

; assemble Simple RMT Player here... 
	
	org $3A00		; at some point this will no longer fit...
start       
	ldx #0			; disable playfield and the black colour value
	stx SDMCTL		; write to Shadow Direct Memory Access Control address
	jsr wait_vblank		; wait for vblank before continuing
	stx COLOR4		; Shadow COLBK (background colour), black
	stx COLOR2		; Shadow COLPF2 (playfield colour 2), black
	mwa #dlist SDLSTL	; Start Address of the Display List
	mva #>FONT rCHBASE      ; load the font address into the character register, same things apply to it
	mwa #line_0 DISPLAY	; initialise the Display List indirect memory address for later

set_colours
	lda #74
	sta COLOR3
	lda #223
	sta COLOR1
	lda #30
	sta COLOR0
	
;-----------------
		
	jsr module_init 	; first initialise the player variables, initialisation returns instrument speed
	tay			; use the instrument speed as an offset
	
;-----------------
	
adjust_check
	beq adjust_check_a	; Y is 0 if equal, this is invalid!
	bpl adjust_check_b	; Y = 1 to 127, however, 16 is the maximum supported
adjust_check_a
	ldy #1			; if Y is 0, nagative, or above 16, this will be bypassed!
	sty v_instrspeed	; failsafe, instrument speed of 1 is forced
	sty v_ainstrspeed 	; both values need to be overwritten for the fix to work correctly
adjust_check_b
	cpy #17			; Y = 17?
	bcs adjust_check_a	; everything equal or above 17 is invalid! 
adjust_check_c
	sty instrspeed		; print later ;)
	cpy #5
	bcc do_speed_init	; all values between 1 to 4 don't need adjustments
	beq adjust_5vbi
	cpy #7
	beq adjust_7vbi
	cpy #8
	beq adjust_8vbi
	cpy #9
	beq adjust_9vbi
	cpy #10
	beq adjust_10vbi	
	cpy #11
	beq adjust_7vbi
	cpy #14
	beq adjust_7vbi
	cpy #15
	beq adjust_10vbi 
adjust_9vbi			; 16 is the maximal number supported, and uses the 9xVBI fix
	lda #153		; fixes 9xVBI, 16xVBI
	bne do_vbi_fix
adjust_5vbi
	lda #155		; fixes 5xVBI
	bne do_vbi_fix
adjust_7vbi
	lda #154		; fixes 7xVBI, 11xVBI, 14xVBI
	bne do_vbi_fix
adjust_8vbi
	lda #152		; fixes 8xVBI
	bne do_vbi_fix
adjust_10vbi
	lda #150		; fixes 10xVBI, 15xVBI
do_vbi_fix
	sta onefiftysix
	
;-----------------
	
do_speed_init
	lda tabpp-1,y		; load from the line counter spacing table
	sta acpapx2		; lines between each play

	lda SKSTAT		; Serial Port Status
	and #$08		; SHIFT key being held?
	beq no_dma		; yes, skip the next 2 instructions
	
	ldx #$22		; DMA enable, normal playfield
	stx SDMCTL		; write to Shadow Direct Memory Access Control address
	
no_dma
	ldx #100		; load into index x a 100 frames buffer
wait_init   
	jsr wait_vblank		; wait for vblank => 1 frame
	
	mva #>FONT rCHBASE	
	
	dex			; decrement index x
	bne wait_init		; repeat until x = 0, total wait time is ~2 seconds
region_init			; 50 Hz or 60 Hz?
	stx vcount		; x = 0, use it here
	ldx #156		; default value for all regions
;	ldx #$A0		; used for a tune ;)
onefiftysix equ *-1		; adjustments
region_loop	
	lda vcount
	beq check_region	; vcount = 0, go to check_region and compare values
	tay			; backup the value in index y
	bne region_loop 	; repeat
	
;-----------------
	
check_region
	cpy #$9B		; compare index y to 155
	IFT REGIONPLAYBACK==0	; if the player region defined for PAL...
	bpl region_done		; negative result means the machine runs at 60hz
	ldx #130		; NTSC is detected, adjust the speed from PAL to NTSC
	
	ELI REGIONPLAYBACK==1	; else, if the player region defined for NTSC...
	bmi region_done		; positive result means the machine runs at 50hz

	ldx #186		; PAL is detected, adjust the speed from NTSC to PAL
	lda instrspeed		; what value did RMT return again?
	cmp #1
	beq subonetotiming	; 1xVBI is stable if 1 is subtracted from the value, 186 must be used!
	cmp #2
	beq subfourtotiming	; 2xVBI is stable if 4 is subtracted from the value, 185 must be used!
	cmp #3
	beq region_done		; 3xVBI is stable without a subtraction
	cmp #4
	beq subtwototiming	; 4xVBI is stable if 2 is subtracted from the value, 185 must be used!

subfourtotiming
;	ldx #185
	dec acpapx2		; stabilise NTSC timing in PAL mode

subthreetotiming
	dec acpapx2		; stabilise NTSC timing in PAL mode
	
subtwototiming
	ldx #185
	dec acpapx2		; stabilise NTSC timing in PAL mode	
subonetotiming
	dec acpapx2		; stabilise NTSC timing in PAL mode
	
	EIF			; endif
	
region_done
	sty region_byte		; set region flag to print later
	stx ppap		; value used for screen synchronisation
	sei			; Set Interrupt Disable Status
	mwa VVBLKI oldvbi       ; vbi address backup
	mwa #vbi VVBLKI		; write our own vbi address to it	
	mva #$40 NMIEN		; enable vbi interrupts
	
	mva #>FONT rCHBASE
	
;-----------------

; print instrument speed, done once per initialisation

	ldy #4			; 4 characters buffer 
	lda #0
instrspeed 	equ *-1
	jsr printhex_direct
	lda #0
	dey			; Y = 4 here, no need to reload it
	sta (DISPLAY),y 
	mva:rne txt_VBI-1,y line_0+5,y-
	
	lda MODUL+3		; RMT4 or RMT8?
	cmp #$34
	beq is_rmt4		; RMT4 if equal, nothing to do
	
is_rmt8
	ldy #8			; 8 characters buffer
	mva:rne txt_VBI+4,y line_0+10,y-
is_rmt4
	
;-----------------
	
; print region, done once per initialisation

	ldy #4			; 4 characters buffer 
	lda #0
region_byte	equ *-1
	cmp #$9B
	bmi is_NTSC
is_PAL
	ldx #50
	mva:rne txt_PAL-1,y line_0-1,y-
	beq is_DONE
is_NTSC
	ldx #60
	mva:rne txt_NTSC-1,y line_0-1,y-
is_DONE				
	sty v_second		; Y is 0, reset the timer with it
	sty v_minute
	stx v_frame		; X is either 50 or 60, defined by the region initialisation
	stx framecount
ready_to_play
	ldy #7			; enough characters buffer, used to set the PLAY status display 
	mva:rne txt_PLAY-1,y line_0e1-1,y- 

;------------------
	
wait_sync
	lda VCOUNT		; current scanline 
	cmp #VLINE		; will stabilise the timing if equal
	bcc wait_sync		; nope, repeat

;-----------------

; main loop, code runs from here ad infinitum after initialisation

loop
	ldy #RASTERBAR		; custom rasterbar colour
rasterbar_colour equ *-1
acpapx1
	lda spap
	ldx #0
cku	equ *-1
	bne keepup
	lda VCOUNT		; vertical line counter synchro
	tax
	sub #VLINE
lastpap	equ *-1
	scs:adc #$ff
ppap	equ *-1
	sta dpap
	stx lastpap
	lda #0
spap	equ *-1
	sub #0
dpap	equ *-1
	sta spap
	bcs acpapx1
keepup
	adc #$ff
acpapx2	equ *-1
	sta spap
	ldx #0
	scs:inx
	stx cku
play_loop
	sty WSYNC		; horizontal sync 
check_play_flag	
	lda is_playing_flag 	; 0 -> is playing, else it is either stopped or paused, and must not run into rmtplay again 
	beq check_rasterbar_flag
	bpl stop_loop 		; 1 -> is stopped, loop goes there instead 
	jmp check_play_flag 	; repeat this until the flag is changed back to 0 
check_rasterbar_flag
	lda #0			; lda #$80 to display the rasterbar by default, 0 sets it hidden otherwise 
rasterbar_toggler equ *-1
	bpl do_rmtplay		; a positive value means the rasterbar is not displayed 
	sty COLBK		; background colour
	sty COLPF2		; playfield colour 2
do_rmtplay
	jsr rmt_play		; setpokey + 1 play
	ldy #$00		; black colour value
	sty WSYNC		; horizontal sync
	sty COLBK		; background colour
	sty COLPF2		; playfield colour 2 
	beq loop                ; unconditional

;-----------------

stop_loop
	lda is_playing_flag 	; 0 -> is playing, else it is either stopped 
	bne stop_loop 		; loop infinitely, until the play flag is set to 0 again 
	jsr module_init 	; reset the player to its initial state 
	jmp loop		; continue like normal from here 

;-----------------

; VBI loop

vbi
	sta WSYNC		; horizontal sync, so we're always on the exact same spot
	ldy KBCODE		; Keyboard Code  
	ldx <line_4		; line 4 of text
	lda SKSTAT		; Serial Port Status
	and #$08		; SHIFT key being held?
	bne set_line_4		; nope, skip the next ldx
	ldx <line_5		; line 5 of text (toggled by SHIFT) 
	tya
	eor #$40		; invert the SHIFT key flag so it will be ignored later
	tay
set_line_4  
	stx txt_toggle		; write to change the text on line 4 
	
check_key_pressed 	
	lda SKSTAT		; Serial Port Status
	and #$04		; last key still pressed?
	beq check_key_pressed_b	; yes if equal
check_key_pressed_a
	jmp continue		; if not, nothing else to do here 
check_key_pressed_b 
	lda #0 
held_key_flag equ *-1
	bpl check_keys
	jmp continue_b		; a key is being held... skip ahead immediately 

check_keys
	; check all keys that have a purpose here... 
	ldx button_selection_flag
check_key_left
	cpy #$06		; Left/Plus key?
	beq dec_index_selection ; yes => decrement the index by 1
check_key_right
	cpy #$07 		; Right/Asterisk key? 
	beq inc_index_selection ; yes => increment the index by 1 
check_key_enter	
	cpy #$0C		; Enter?
	bne check_key_space	
	jmp do_button_selection	; yes => process the menu input code 	
check_key_space
	cpy #$21		; Spacebar? 
	bne check_key_esc 	; JMP continue on that line if not equal 
	jmp toggle_rasterbar 	; yes => toggle the rasterbar display 
check_key_esc 
	cpy #$1C		; ESCape key? 
	bne check_key_pressed_a	; nope => loop 
	jmp stopmusic		; else, it's over 

;-----------------

; index_selection 

dec_index_selection
	dex 				; decrement the index
	bpl done_index_selection	; if the value did not underflow, done 
	ldx #0				; if it went past the boundaries, load 0 as a failsafe
	beq done_index_selection	; unconditional
inc_index_selection
	inx				; increment the index
	cpx #7				; compare to the maximum of 7 button indexes
	bcc done_index_selection	; if below 7, everything is good
	ldx #6 				; else, load 6 as a failsafe 
done_index_selection
	stx button_selection_flag 	; overwrite the index value
	jmp set_held_key_flag 		; done 
	
;-----------------

; RMT Play/Pause

play_pause_toggle 
	lda #0
is_playing_flag equ *-1 
	beq set_pause		; 0 -> currently playing, else, it was either paused or stopped 
set_play 
	ldx #0			; will set the play flag to 0, and also the offset for the PLAY characters 
	stx is_playing_flag
	beq play_pause_button_toggle_a
set_pause 
	dec is_playing_flag	; 00 -> FF
	jsr stop_pause_reset	; pause the player and clear the AUDC registers 
play_pause_button_toggle 
	ldx #8			; offset by 8 for PAUSE characters	
play_pause_button_toggle_a 
	ldy #7			; 7 character buffer is enough 
	mwa #line_0e1 DISPLAY	; move the position to the correct line
	mwa #txt_PLAY infosrc	; set the pointer for the text data to this location
	jsr printinfo 		; write the new text in this location 
	ldx line_0e1		; the play/pause/stop character
	cpx #$7B		; is it the STOP character?
	bne play_pause_button_toggle_b ; if not, overwrite the character in the buttons display with either PLAY or PAUSE
	inx			; else, make sure PLAY is loaded, then write it in memory 
play_pause_button_toggle_b	
	stx b_play 		; overwrite the Play/Pause character
	jmp set_held_key_flag 	; done 

;-----------------

; RMT Stop, similar to pause, but then Play will start from the beginning, just like if it was initialised again 

stop_toggle
	jsr stop_pause_reset		; pause the player and clear the AUDC registers 
	ldy #1
	sty is_playing_flag		; 1 -> player stopped
	dey				; Y = 0, reset the timer, and order/row counter
	sty v_second			; reset the seconds counter
	sty v_minute			; reset the minutes counter
	sty v_ord			; reset the order counter
	sty v_abeat 			; reset the rows counter
	lda framecount			; number of frames defined at initialisation  
	sta v_frame			; reset the frames counter 
	ldx #16				; offset by 16 for STOP characters
	bne play_pause_button_toggle_a ; finish in the play_pause code from here, unconditional 

;-----------------

; songline_seek

dec_songline_seek
	ldy MODUL+15		; songline pointer MSB
	ldx MODUL+14		; songline pointer LSB 
	lda p_song
	sub #TRACKS+TRACKS	; once -> same line plays again, so subtract twice this number!
	sta p_song 
	scs:dec p_song+1	; in case the boundary is crossed, the pointer MSB will increment as well
dec_songline_seek_a
	cpy p_song+1		; compare the p_song MSB to the one from the module
	bcs dec_songline_seek_b	; if the module value is lower, everything is mostly fine... do an extra check before!
	sty p_song+1		; else, revert the changes to it 
dec_songline_seek_b
	cpx p_song		; compare the p_song LSB to the one from the module
	bcc dec_songline_seek_c	; if the module value is lower, everything is fine
	stx p_song 		; overwrite the earlier changes to it	
dec_songline_seek_c
	; uhhh..... extra check necessary here! Things appear to work ok but this is not perfect, it could jump to garbage! 
inc_songline_seek
	jsr GetSongLine		; hopefully this will work... 
	jmp set_held_key_flag 	; done 

;----------------- 
	
toggle_rasterbar 
	lda rasterbar_toggler	; rasterbar flag, a negative value means the rasterbar display is active 
	eor #$FF		; invert bits 
	sta rasterbar_toggler	; overwrite the rasterbar flag, execution continues like normal from here 
	
;-----------------

set_held_key_flag 
	ldx #$FF
	bmi continue_a		; skip ahead and set the held key flag! 
continue			; do everything else during VBI after the keyboard checks 
	ldx #0			; reset the held key flag! 
continue_a 			; a new held key flag is set when jumped directly here
	stx held_key_flag
continue_b 			; a key was detected as held when jumped directly here

;-----------------

calculate_time 
	lda is_playing_flag 
	bne notimetolose	; not playing -> no time counter increment  
	dec v_frame		; decrement the frame counter
	bne notimetolose	; not 0 -> a second did not yet pass
	lda #0
framecount equ *-1		; 50 or 60, defined by the region initialisation
	sta v_frame		; reset the frame counter
	bne addasecond		; unconditional
	nop
v_frame equ *-1			; the NOP instruction is overwritten by the frame counter
	
addasecond
	sed			; set decimal flag first
	lda #0
v_second equ *-1
	clc			; clear the carry flag first, the keyboard code could mess with this part now...
	adc #1			; carry flag is clear, add 1 directly
	sta v_second
	cmp #$60		; 60 seconds, must be a HEX value!
	bne cleardecimal 	; if not equal, no minute increment
	ldy #0			; will be used to clear values quicker
	
addaminute
	lda #0
v_minute equ *-1
	adc #0			; carry flag is set above, adding 0 will add 1 instead
	sta v_minute
	sty v_second		; reset the second counter
cleardecimal 
	cld			; clear decimal flag 
notimetolose
	
;-----------------
	
; get the right screen position
	mwa #line_0a DISPLAY 

; print minutes
	ldy #8
	lda v_minute
	jsr printhex_direct
	
; print seconds
	ldy #10
	ldx v_second
	txa
	and #1
	beq no_blink 
	lda #0
	beq blink
no_blink 
	lda #":"-$20
blink
	sta (DISPLAY),y 
	iny 
done_blink
	txa
	jsr printhex_direct

	IFT !FEAT_CONSTANTSPEED
; print speed
	ldy #20
	lda v_speed
	jsr printhex_direct 
	EIF

; print order	
	ldy #28
	lda #0
v_ord	equ *-1
	jsr printhex_direct
	
; print row
	ldy #36
	lda v_abeat
	jsr printhex_direct
	
	
;;;;
;
; print decay rate and speed, DEBUG CODE!!!!
;	ldy #28
;	lda #RATE
;	jsr printhex_direct
;	
;	ldy #38
;	lda #SPEED
;	jsr printhex_direct 
;
;;;;	
	
;-----------------
	
; draw the volume blocks
	
; index ORA
; #$00 -> COLPF0
; #$40 -> COLPF1 (could be exploited since the font seems to only change brightness), use on numbers and green bars level
; #$80 -> COLPF2 cannot be used!! conflicts with rasterbar, unless I used a DLI
; #$C0 -> COLPF3
	
; current order: red, green (2x), yellow, and numbers in green again...
; line 1: pf3
; line 2-3: pf1, use also on numbers below line 5
; line 4: pf0 

; rambles...
; LSR @ 2x from the volume values lead to this observation:

; F to C => 3
; B to 8 => 2
; 7 to 4 => 1
; 3 to 0 => 0

; now... how could this actually helps me...?
; hmmm... 

; one way to imagine this is, using these values as JMPs, I would land on the first line that does have characters to draw
; but there I still cannot exactly know where I am really supposed to be, so that is still going to be a problem...

; another idea is to make a table of JMP/Branches, but then I get other problems... this is tricky.
; I could also go on a column by column basis, where X is the index for variables, and Y handles the screen index?
; but then again that still gets messy... argh.

; ok here's an idea:
; use AND operations as BIT, make jumps based on them, through the necessary LSR needed to get the values
; that will remove the necessity from using X for line index, and maybe save the CPU this time... I hope

; line 1 => AND with #$0C, BEQ draw blank tile, remaining values are LSR twice, then branched to the appropriate tile
; line 2 => AND with #$08, BEQ draw blank tile, remaining values are LSR once, then branched to the appropriate tile
; line 3 => AND with #$04, BEQ draw blank tile, remaining values are used directly to branch to the appropriate tile

; ...
; ... no that won't work, bleh.
; too tired for today.
; maybe I could do with subtractions?
; heh I need sleep

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
	
;-----------------

set_highlight 
	ldx #6				; 7 buttons to index
set_highlight_a
	txa 				; transfer to accumulator
	asl @				; multiply by 2
	tay 				; transfer to Y, use to index the values directly
	lda b_handler,y			; load the chacacter from this location
	bpl set_highlight_b		; positive -> no highlight, skip overwriting it
	eor #$80 			; invert the character
	sta b_handler,y			; overwrite, no highlight to see again 
set_highlight_b
	dex 				; decrease the index and load the next character using it
	bpl set_highlight_a		; as long as X is positive, do this again until all characters were reset 
set_highlight_c 
	lda button_selection_flag	; load the button flag value previously set in memory
	asl @				; multiply it by 2 for the index 
	tay				; transfer to Y, use it to index the character directly
	lda b_handler,y 		; load the character in memory 
	eor #$80 			; invert the character, this will now define it as "highlighted"
	sta b_handler,y 		; write the character in memory, it is now selected, and will be processed again later 
	
;-----------------

return_from_vbi
	pla			; since we're in our own vbi routine, pulling all values manually is required
	tay
	pla
	tax
	pla
	sta WSYNC		; horizontal sync, this seems to make the timing more stable
	rti			; return from interrupt

;-----------------

; wait for vblank subroutine

wait_vblank 
	lda RTCLOK+2		; load the real time frame counter to accumulator
wait        
	cmp RTCLOK+2		; compare to itself
	beq wait		; equal means it vblank hasn't began
	rts

;-----------------

; print text from data tables, useful for many things 

printinfo 
	sty charbuffer
	ldy #0
do_printinfo
        lda $ffff,x
infosrc equ *-2
	sta (DISPLAY),y
	inx
	iny 
	cpy #0
charbuffer equ *-1
	bne do_printinfo 
	rts

;-----------------

; print hex characters for several things, useful for displaying all sort of debugging infos
	
printhex
	ldy #0
printhex_direct     ; workaround to allow being addressed with y in different subroutines
	pha
	:4 lsr @
	;beq ph1    ; comment out if you want to hide the leftmost zeroes
	tax
	lda hexchars,x
ph1	
        sta (DISPLAY),y+
	pla
	and #$f
	tax
	mva hexchars,x (DISPLAY),y
	rts
hexchars 
        dta d"0123456789ABCDEF"
        
;-----------------

; stop RMT and quit

stopmusic 
	jsr rmt_silence		; stop RMT and reset the POKEY registers
	mwa oldvbi VVBLKI	; restore the old vbi address
	ldx #$00		; disable playfield 
	stx SDMCTL		; write to Direct Memory Access (DMA) Control register
	dex			; underflow to #$FF
	stx CH			; write to the CH register, #$FF means no key pressed
	jsr wait_vblank		; wait for vblank before continuing
	jmp (DOSVEC)		; return to DOS, or Self Test by default

;----------------- 

; initialise the player, also reset the player if called from full stop

module_init	
	ldx #<MODUL		; low byte of RMT module to X reg
	ldy #>MODUL		; hi byte of RMT module to Y reg
	lda #STARTLINE		; starting song line 0-255 to A reg
	jsr rmt_init		; Init returns instrument speed (1..4 => from 1/screen to 4/screen)
	rts

;-----------------

; pause RMT and reset the registers

stop_pause_reset
	jsr rmt_silence		; pause the player 
	lda #0			; default values
	ldy #TRACKS-1		; number of channels to reset 
stop_pause_reset_a 
	sta trackn_audc,y	; clear the AUDC values in memory
	dey 
	bpl stop_pause_reset_a	; repeat until all channels were cleared 
	rts

;----------------- 

; menu input handler

do_button_selection   
	lda #2			; by default, the PLAY/PAUSE button 
button_selection_flag equ *-1
	asl @
	asl @ 
	sta b_index+1
b_index	bcc *
	jmp continue 		; #0 => seek reverse (no code yet) 
	nop
	jmp dec_songline_seek	; #1 => fast reverse (no code yet) 
	nop
	jmp play_pause_toggle	; #2 => play/pause 
	nop
	jmp inc_songline_seek 	; #3 => fast forward (no code yet) 
	nop
	jmp continue 		; #4 => seek forward (no code yet) 
	nop
	jmp stop_toggle 	; #5 => stop
	nop
	jmp stopmusic 		; #6 => eject 
	
;-----------------

; some plaintext data used in few spots
        
txt_NTSC
        dta d"NTSC"*
txt_PAL
        dta d"PAL"*,d" "
txt_VBI
	dta d"xVBI (Stereo)"
	
txt_PLAY
	dta $7C,$00 		; PLAY button
	dta d"PLAY  "
txt_PAUSE
	dta $7D,$00 		; PAUSE button
	dta d"PAUSE "
txt_STOP
	dta $7B,$00 		; STOP button
	dta d"STOP  "
        
;-----------------
        
; Display list

dlist       
	:6 dta $70		; 6 empty lines
	dta $42,a(line_0)	; ANTIC mode 2, for the first line of infos drawn on the screen 
	dta $70			; 1 empty line
	:4 dta $06		; ANTIC mode 6, 4 lines, for the volume bars
	:7 dta $02		; all the middle part is back to mode 2, for all the infos and song buttons display 
	dta $70			; 1 empty line
	:3 dta $02		; 3 lines of user input text from RMT
	dta $42			
txt_toggle
	dta a(line_4)		; memory address set to line_4 by default, or line_5 when SHIFT is held
	:3 dta $70		; 3 empty lines
	dta $42,a(line_6)	; 1 final line of mode 2, must have its own addressing or else the SHIFT toggle affects it 
	dta $41,a(dlist)	; Jump and wait for vblank, return to dlist
	
;-----------------

; line counter spacing table for instrument speed from 1 to 16

tabpp       
	dta 156,78,52,39,31,26,22,19,17,15,14,13,12,11,10,9 

;-----------------

oldvbi	
	dta a(0)		; vbi address backup
	
;-----------------

; text strings, each line holds 40 characters, line 5 is toggled with the SHIFT key

	org $A000		; must be at this memory address 

; DEBUG DATA!!!
;
;line_0	dta d"                Decay Rate: 00 Speed: 00"

; topmost line, displays region and speed 

line_0	dta d"                                        "

; volume bars, mode 6, 4 lines

mode_6	dta d"                    "
mode_6a	dta d"                    "
mode_6b	dta d"                    "
mode_6c	dta d"                    "

; topmost border, under the volume bars, back to mode 2

mode_2d dta $43,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45
	dta $45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$41 

; timer, order, row, etc display

line_0a	dta $44 
	dta d" Time: 00:00  Spd: 00 Ord: 00 Row: 00 "
	dta $44

; top border

line_0b dta $44,$43,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45
	dta $45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$41,$44

; middle playback progress line

line_0c dta $44,$44,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45
	dta $45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$44,$44

; bottom border

line_0d dta $44,$42,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45
	dta $45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$40,$44

; subtunes display, and control buttons 

line_0e	dta $44
	dta d" Tune: 01/01   "
line_0e1	
	dta $7B,$00 			; STOP button, will be overwritten 
	dta d"STOP   "			; STOP text, will be overwritten 

; buttons for music player display

b_handler				; index for the buttons handler
b_seekr	dta $58,$00			; 0, Seek Reverse
b_fastr	dta $7F,$00 			; 1, Fast Reverse
b_play	dta $7C,$00 			; 2, PLAY or PAUSE, it will be overwritten when needed! 
b_fastf	dta $7E,$00 			; 3, Fast Forward
b_seekf	dta $57,$00 			; 4, Seek Forward
b_stop	dta $7B,$00 			; 5, Stop
b_eject	dta $56,$00 			; 6, Eject, will act as a fancy "Exit" button for now... 
	dta $44

; bottomest border

line_0f dta $42,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45
	dta $45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$45,$40

; currently below the volume bars, mode 2, 5 lines, where 1 of them is swapped using the SHIFT key

line_1	dta d"Line 1                                  "
line_2	dta d"Line 2                                  "
line_3	dta d"Line 3                                  "
line_4	dta d"Line 4 (hold SHIFT to toggle)           "
line_5	dta d"Line 5 (SHIFT is being held right now)  "

; version and credit

line_6	dta d"VUPlayer by VinsCool                "
	dta d"v0.1"* 

;-----------------

; load font into memory, this was put at the very end to avoid overwriting other data

        org FONT                ; characters set memory location
        ins "font.fnt"          ; some cool looking font

;-----------------

; set run address

	run start
	EIF 
	
;* end of XEX format...
	
;---------------------------------------------------------------------------------------------------------------------------------------------;

; insert actual .rmt module

	opt h-			; RMT module is standard Atari binary file already
	ins "music.rmt"		; include music RMT module

;---------------------------------------------------------------------------------------------------------------------------------------------;

; and that's all :D

