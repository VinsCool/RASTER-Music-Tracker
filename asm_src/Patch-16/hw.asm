; a8 hardware registers

RTCLOK  equ $0012 ; Real Time Clock

CHBAS   equ $02f4 ; Character Base Register

; CTIA/GTIA
; read
rM0PF   equ $d000 ; missile-playfield colls
rM1PF   equ $d001
rM2PF   equ $d002
rM3PF   equ $d003
rP0PF   equ $d004 ; player-playfield colls
rP1PF   equ $d005
rP2PF   equ $d006
rP3PF   equ $d007
rM0PL   equ $d008 ; missile-player colls
rM1PL   equ $d009
rM2PL   equ $d00a
rM3PL   equ $d00b
rP0PL   equ $d00c ; player-player colls
rP1PL   equ $d00d
rP2PL   equ $d00e
rP3PL   equ $d00f
rTRIG0  equ $d010 ; joystick trigger buttons
rTRIG1  equ $d011
rTRIG2  equ $d012
rTRIG3  equ $d013
rPAL    equ $d014 ; PAL/NTSC
; write
rHPOSP0 equ $d000 ; player hpos
rHPOSP1 equ $d001
rHPOSP2 equ $d002
rHPOSP3 equ $d003
rHPOSM0 equ $d004 ; missile hpos
rHPOSM1 equ $d005
rHPOSM2 equ $d006
rHPOSM3 equ $d007
rSIZEP0 equ $d008 ; player size
rSIZEP1 equ $d009
rSIZEP2 equ $d00a
rSIZEP3 equ $d00b
rSIZEM  equ $d00c ; missile size
rGRAFP0 equ $d00d ; player graphics
rGRAFP1 equ $d00e
rGRAFP2 equ $d00f
rGRAFP3 equ $d010
rGRAFM  equ $d011 ; missile graphics
rCOLPM0 equ $d012 ; player+missile colors
rCOLPM1 equ $d013
rCOLPM2 equ $d014
rCOLPM3 equ $d015
rCOLPF0 equ $d016 ; playfield colors
rCOLPF1 equ $d017
rCOLPF2 equ $d018
rCOLPF3 equ $d019
rCOLBK  equ $d01a ; background color
rPRIOR  equ $d01b ; player-playfield priority
rVDELAY equ $d01c ; vertical delay
rGRACTL equ $d01d ; graphic control
rHITCLR equ $d01e ; collision clear
; read/write
rCONSOL equ $d01f ; console switch port

; POKEY
; read
rPOT0   equ $d200 ; potentiometer values
rPOT1   equ $d201
rPOT2   equ $d202
rPOT3   equ $d203
rPOT4   equ $d204
rPOT5   equ $d205
rPOT6   equ $d206
rPOT7   equ $d207
rALLPOT equ $d208 ; potentiometer port state
rKBCODE equ $d209 ; keyboard code
rRANDOM equ $d20a ; rng
rSERIN  equ $d20d ; serial in
rIRQST  equ $d20e ; IRQ status
rSKSTAT equ $d20f ; serial port 4 key status
; write
rAUDF1  equ $d200 ; ch1 frequency
rAUDC1  equ $d201 ; ch1 control
rAUDF2  equ $d202 ; ch2 frequency
rAUDC2  equ $d203 ; ch2 control
rAUDF3  equ $d204 ; ch3 frequency
rAUDC3  equ $d205 ; ch3 control
rAUDF4  equ $d206 ; ch4 frequency
rAUDC4  equ $d207 ; ch4 control
rAUDCTL equ $d208 ; audio control
rSTIMER equ $d209 ; start timer
rSKRES  equ $d20a ; reset skstat
rPOTGO  equ $d20b ; start pot scan sequence
rSEROUT equ $d20c ; serial out
rIRQEN  equ $d20e ; IRQ enable
rSKCTL  equ $d20f ; serial port 4 key control

; PIA
; read/write
rPORTA  equ $d300 ; port a data/direction
rPORTB  equ $d301 ; port b data/direction (memory/led control in XL/XE)
rPACTL  equ $d302 ; port a control
rPBCTL  equ $d303 ; port b control (unused in XL/XE)

; ANTIC
; read
rVCOUNT equ $d40b ; vertical line counter
rPENH   equ $d40c ; light pen x
rPENV   equ $d40d ; light pen y
rNMIST  equ $d40f ; NMI status
; write
rDMACTL equ $d400 ; DMA control
rCHACTL equ $d401 ; character control
rDLISTL equ $d402 ; display list address
rDLISTH equ $d403
rHSCROL equ $d404 ; scroll x
rVSCROL equ $d405 ; scroll y
rPMBASE equ $d407 ; player/missile gfx address
rCHBASE equ $d409 ; character gfx address
rWSYNC  equ $d40a ; wait for hblank
rNMIEN  equ $d40e ; NMI enable
rNMIRES equ $d40f ; NMI reset
; read/write
ATASCII equ $e000 ; Atari characters set (up to $e3ff)

; 6502 interrupt vectors
rNMI    equ $fffa ; NMI vector
rRESET  equ $fffc ; reset vector
rIRQ    equ $fffe ; IRQ vector
