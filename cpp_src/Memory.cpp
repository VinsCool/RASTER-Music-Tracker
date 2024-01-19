// Memory allocation and buffer I/O functions for Raster Music Tracker
// By VinsCool, 2024

#include "stdafx.h"

#include "Memory.h"


CMemory::CMemory()
{
	m_size = MIN_SIZE;
	m_offset = 0;
	m_buffer = new BYTE[m_size];
	memset(m_buffer, 0, m_size);
}

CMemory::~CMemory()
{
	if (m_buffer)
		delete m_buffer;

	m_size = 0;
	m_offset = 0;
	m_buffer = NULL;
}

void CMemory::SeekOffset(UINT64 offset)
{
	if (offset < m_size)
		m_offset = offset;
	else
		m_offset = m_size;
}

// Create a new buffer and copy data from the old buffer, matching the new allocated size
// If the new size is smaller than the old size, the data will be truncated
void CMemory::ResizeBuffer(UINT64 size)
{
	BYTE* oldBuffer = m_buffer;
	m_buffer = new BYTE[size];
	memset(m_buffer, 0, size);
	memcpy(m_buffer, oldBuffer, size >= m_size ? m_size : size);
	m_size = size;
	delete oldBuffer;
}

// Resize buffer to match the offset position
void CMemory::TruncateBuffer()
{
	ResizeBuffer(m_offset);
}

void CMemory::PutByte(BYTE data)
{
	PushBytes(&data, sizeof(BYTE));
}

BYTE CMemory::GetByte()
{
	BYTE data = 0;
	PullBytes(&data, sizeof(BYTE));
	return data;
}

void CMemory::PushBytes(BYTE* data, UINT64 size)
{	
	while (size--)
	{
		// Buffer too small, resize before copying more data
		if (m_offset >= m_size)
			ResizeBuffer(m_size * 2);

		// Copy data into the buffer and update the offset position
		m_buffer[m_offset++] = *data++;
	}
}

void CMemory::PullBytes(BYTE* data, UINT64 size)
{
	while (size--)
	{
		// Offset reached the end of Buffer, no more data to be copied
		if (m_offset >= m_size)
			return;

		// Copy data from the buffer and update the offset position
		*data++ = m_buffer[m_offset++];
	}
}

void CMemory::ReadFile(std::ifstream& in)
{
	// Get the file size to load in memory
	in.seekg(0, std::ios_base::end);
	UINT64 size = in.tellg();

	// Buffer too small, resize before copying more data
	if (m_size < m_offset + size)
		ResizeBuffer(m_offset + size);

	// Read the entire file into the buffer from the beginning
	in.seekg(0, std::ios_base::beg);
	in.read((char*)m_buffer + m_offset, size);

	// Update offset and adjust buffer size if needed
	m_offset += size;

	if (m_offset < m_size)
		TruncateBuffer();

	// Close the file once it is loaded in memory
	in.close();
}

void CMemory::WriteFile(std::ofstream& ou)
{
	// Write the entire buffer to the file from the beginning
	ou.seekp(0, std::ios_base::beg);
	ou.write((char*)m_buffer + m_offset, m_size - m_offset);

	// Update offset
	m_offset = m_size;

	// Close the file once it is written
	ou.close();
}
