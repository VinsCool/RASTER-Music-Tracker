The RMT driver's 6502 code is located in this folder.
Current asm sources are split between 2 versions: Legacy and Patch-16

---------------------------------------------------------------------

Legacy (For preservation and reference):

This folder contains all of the source files that were used for RMT versions 1.29 and lower.
All of this folder was copied over for preservation purpose, and may not reflect what was actually used in older RMT releases!
If a project require this version specifically, it will still be useful, in any case.

Patch-16 (Current version used by RMT):

This folder contains all of the RMT driver assembly code used since version 1.31 by VinsCool.
This fork began as a very experimental set of changes, intended to be used for a Rmt.exe hack.
Ultimately, this version became its own thing, and has since expanded the capabilities of the RMT driver.
With all of the things done into the code, there are chances things won't work as expected in some situation.
Use with this consideration in mind!

For a complete commit history of Patch-16, please refer to this (now read-only) repository: https://github.com/VinsCool/RMT-Patch16
Current VUPlayer (and associated LZSS driver) export code is available in this repository: https://github.com/VinsCool/VUPlayer-LZSS

---------------------------------------------------------------------

All of the following will be written with the assumption Patch-16 is the RMT driver version used.

Sources can be assembled using XASM or MADS, and can output the following:

- tracker.obx driver binary (for use with Rmt.exe) using FEAT_IS_TRACKER definition.
- XEX/SAP binaries as standalone player exports* using the FEAT_IS_SIMPLEP, EXPORTXEX or EXPORTSAP definitions.
- Any project that makes use of RMT may also use this version, assuming the memory/CPU usage may handle it with no issues.

*Note that this functionality is of being replaced with the much more robust LZSS driver, and might not work correctly anymore.

