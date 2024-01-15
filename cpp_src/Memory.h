// Memory Handler functions for Raster Music Tracker
// By VinsCool, 2024

#pragma once

//#include "General.h"
//#include "global.h"

#define MIN_SIZE	0xFFF
#define MAX_SIZE	0xFFFFFF

// ----------------------------------------------------------------------------
// Memory Allocation and Data Buffer I/O Class
// For dynamic memory allocation and data buffer management 
//
class CMemory
{
public:
	CMemory();
	~CMemory();

public:
	BYTE* GetBuffer() { return m_buffer; };
	UINT64 GetOffset() { return m_offset; };
	UINT64 GetSize() { return m_size; };

	void SeekOffset(UINT64 offset);
	void ResizeBuffer();
	void TruncateBuffer();
	void PutByte(BYTE data);
	void PushBytes(BYTE* data, UINT64 size);
	void PullBytes(BYTE* data, UINT64 size);

private:
	BYTE* m_buffer;
	UINT64 m_offset;
	UINT64 m_size;
};
