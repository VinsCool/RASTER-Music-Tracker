************************************************************************
RASTER MUSIC TRACKER - RMT\
by Radek Sterba, Raster/C.P.U., 2002-2009\
currently maintained by Vin Samuel, VinsCool, 2021-2022\
http://raster.atari.org\
************************************************************************

### About:

RASTER Music Tracker (RMT) is a cross-platform tool for making Atari XE/XL
music on PC (OS Win9x). RMT uses  new Atari XE/XL music routines of my own
- I developed them for a very long time and I hope it will be small
revolution for all Atari musicians and fans. 

### Disclaimer:

THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.
AUTHOR DOES NOT WARRANT, GUARANTEE, OR MAKE ANY REPRESENTATIONS REGARDING
THE USE, OR THE RESULTS OF USE, OF THE SOFTWARE OR WRITTEN MATERIALS IN
TERMS OF CORRECTNESS, ACCURACY, RELIABILITY, CURRENTNESS, OR OTHERWISE.
THE ENTIRE RISK AS TO THE RESULTS AND PERFORMANCE OF THE SOFTWARE
IS ASSUMED BY YOU.

### Credits:

Bob!k/C.P.U.\
JirkaS/C.P.U.

### Main features:

* Mono 4 tracks / stereo 8 tracks.
* 254 tracks, each with its own length (256 beats max.) and with support for track loop.
* 64 instruments (stereo, instrument table up to 32 steps - 2 types and 2 modes with loop,
  instrument envelope up to 32 steps with loop, portamento, filter, 16bit bass, volume slide,
  volume minimum, vibrato, frequency shifting, etc.).Fully automatic management of AUDCTL
  register (filters, 16bit basses) and/or manual AUDCTL settings.
* Support for "volume only" forced output.
* Note portamento up/down effect.
* Instrument envelope commands for note/frequency shifting and support for special 
  "like a C64 SID chip" filtering.
* Up to 256 lines for song (with "goto line" support).
* Beat speed 1 to 255 (1/50 to 255/50 sec).
* Instrument speed from 1 to 4 per screen (up to 1/200 sec).
* Main input/output song file format: RMT song files (*.rmt).
* Input/output instrument file format: RMT instrument files (*.rti).
* Export formats: RMT stripped song file (*.rmt), SAP file (*.sap),
  XEX Atari executable MSX file (*.xex), ASM simple notation source (*.asm).
* Import formats: ProTracker modules (*.mod), Atari XE/XL Theta Music Composer songs (*.tmc)
* Support for speed/size optimizations of RMT assembler player routine 
  for concrete RMT module (very useful for background music in demos, games, etc.).
* MIDI IN support!
* MIDI multitimbral playing possibilities.
  You can use the RMT like a Atari multitimbral MIDI instrument. 
  You have to send MIDI output from your MIDI sequencer or player 
  to RMT MIDI input by means of some virtual MIDI cable (for example 
  "MIDI Yoke" etc.). MIDI implementation chart is in midi.txt file.

### Technical info:

Pokey sound emulation and Atari 6502 processor emulation aren't built-in
components of RMT. If the sound output is needed, the external dynamic DLL
libraries with the following functions are required. 
If you run RMT without this way described DLLs (apokeysnd.dll or sa_pokey.dll,
sa_c6502.dll), RMT will work, but there won't be any Pokey sound output
and Atari sound routines won't be executed.

#### Pokey sound emulation:

`apokeysnd.dll`\
`void APokeySound_Initialize(abool stereo);`\
`void APokeySound_PutByte(int addr, int data);`\
`int APokeySound_GetRandom(int addr, int cycle);`\
`int APokeySound_Generate(int cycles, byte buffer[], ASAP_SampleFormat format);`\
`void APokeySound_About(const char **name, const char **author, const char **description);`

or

`sa_pokey.dll`\
`void Pokey_Initialise(int *argc, char *argv[]);`\
`void Pokey_SoundInit(uint32 freq17, uint16 playback_freq, uint8 num_pokeys);`\
`void Pokey_Process(uint8 * sndbuffer, const uint16 sndn);`\
`UBYTE Pokey_GetByte(UWORD addr);`\
`void Pokey_PutByte(UWORD addr, UBYTE byte);`\
`void Pokey_About(char** name, char** author, char** description);`

#### 6502 processor emulation:

`sa_c6502.dll`\
`void C6502_Initialise(BYTE* memory);`\
`int C6502_JSR(WORD* addr, BYTE* areg, BYTE* xreg, BYTE* yreg, int* maxcycles);`\
`void C6502_About(char** name, char** author, char** description)`;

### Greetings:

Fox/Taquart (Thanks for XASM and ASAP - http://asap.sourceforge.net)<br>
Jaskier/Taquart (Thanks for TMC and a lot of RMT routine speed/size optimizations)\
Tatqoo/Taquart\
Sack/Cosine\
X-ray/Grayscale\
Greg/Grayscale\
Bewu/Grayscale\
PG (Thanks for ASMA - Atari SAP Music Archive - http://asma.atari.org)<br>
Fandal\
ZdenekB\
KrupkaJ\
Pepax\
LiSU\
Miker\
Dely\
Nils Feske\
Elan\
Wrathchild\
Kozyca\
Born/LaResistance\
Sal Esquivel\
Nooly\
All the active "Atariarea" Polish Atarians (http://atariarea.krap.pl)<br>
...and other 8bit Atarians all over the world! :-)

TODO: Update README.md to reflect all the changes done since version 1.28

