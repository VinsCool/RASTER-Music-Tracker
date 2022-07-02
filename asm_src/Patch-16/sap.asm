;***************************************************************************************************************;
;* Simple RMT2SAP Creator                                                                                      *;
;* By VinsCool, 05-10-2021                                                                                     *;
;* Based on the SAP specs listed at http://asap.sourceforge.net/sap-format.html                                *;
;*                                                                                                             *;
;* Include with Simple RMT Player (dasmplayer.asm) to export .sap files instead of Atari executables (.obx)    *;
;* Edit the text with the infos you want, a maximum of 120 characters per argument is supported                *;
;* Use "<?>" for unknown values, although this is entirely optional, it's simply for the SAP format convention *;
;* SPACING is the CR/LF (Carriage Return, Line Feed) word, simply put this between each argument               *;
;***************************************************************************************************************;
	
SPACING	equ $0A0D

; SAP header

	org $0000		; XASM yells at me without an ORG here... 
SAP	dta c"SAP"		; SAP are the characters used to define the format at the start of every .sap file
	dta a(SPACING)		; IMPORTANT, must be at the end of EVERY argument to generate proper SAP exports

; SAP arguments, not all are included but they could be if wanted
; if text needs to be inside quotation marks, the ' character will be used for delimiting

AUTHOR	dta c"AUTHOR "
	dta c'"Raster"'
	dta a(SPACING)
	
NAME	dta c"NAME "
	dta c'"Im sure"'
	dta a(SPACING)
	
DATE	dta c"DATE "
	dta c'"Converted 05/10/2021"'
	dta a(SPACING)
	
SONGS	;dta c"SONGS "		; optional argument
	;dta c"3"
	;dta a(SPACING)

DEFSONG	;dta c"DEFSONG "	; optional argument
	;dta c"0"
	;dta a(SPACING)

	IFT STEREOMODE==1
STEREO	dta c"STEREO"
	dta a(SPACING)
	EIF

	;IFT REGIONPLAYBACK==1	; optional argument, and useless in Altirra with my own hijacked stuff
NTSC	;dta c"NTSC"
	;dta a(SPACING)
	;EIF
	
TYPE	dta c"TYPE B"
	dta a(SPACING)
	
FASTPLAY
	;dta c"FASTPLAY "	; optional argument, and useless in Altirra with my own hijacked stuff
	;dta c"312"		; must be manually edited for now. 312 for PAL 50hz and 262 for NTSC 60hz are most common
	;dta a(SPACING)
	
INIT	dta c"INIT "		; SAP init address, very important to have set up correctly or else bad things happen
	dta c"3E00"		; the same address used for Simple RMT Player because why not
	dta a(SPACING)
	
PLAYER_	dta c"PLAYER "		; rmtplay jump address
	dta c"3403"
	dta a(SPACING)
	
TIME	;dta c"TIME "		; optional argument
	;dta c"69:42.00 "	; Minutes:Seconds.Miliseconds format
	;dta c"LOOP"		; optional
	;dta a(SPACING)
	
ENDHEADER
	dta $FF,$FF		; Atari header before assembly, very important to keep!
	
; and that's all :D

