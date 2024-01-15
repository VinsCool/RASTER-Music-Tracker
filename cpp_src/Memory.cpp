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
	if (!m_buffer)
		return;

	if (offset < m_size)
		m_offset = offset;
	else
		m_offset = m_size - 1;
}

void CMemory::ResizeBuffer()
{
	if (!m_buffer)
		return;

	// Create a new buffer and copy data from the old buffer, double the allocated size
	m_size *= 2;
	BYTE* newBuffer = new BYTE[m_size];
	memset(newBuffer, 0, m_size);
	memcpy(newBuffer, m_buffer, m_size / 2);
	delete m_buffer;
	m_buffer = newBuffer;
}

void CMemory::TruncateBuffer()
{
	if (!m_buffer)
		return;

	// Create a new buffer and copy data from the old buffer, truncate size to the offset
	m_size = m_offset;
	BYTE* newBuffer = new BYTE[m_size];
	memset(newBuffer, 0, m_size);
	memcpy(newBuffer, m_buffer, m_size);
	delete m_buffer;
	m_buffer = newBuffer;
}

void CMemory::PutByte(BYTE data)
{
	PushBytes(&data, sizeof(BYTE));
}

void CMemory::PushBytes(BYTE* data, UINT64 size)
{
	if (!m_buffer)
		return;

	while (size--)
	{
		// Buffer too small, resize before copying more data
		if (m_offset >= m_size - 1)
			ResizeBuffer();

		// Copy data into the buffer and update the offset position
		m_buffer[m_offset++] = *data++;
	}
}

void CMemory::PullBytes(BYTE* data, UINT64 size)
{
	if (!m_buffer)
		return;

	while (size--)
	{
		// Offset reached the end of Buffer, no more data to be copied
		if (m_offset >= m_size - 1)
			return;

		// Copy data from the buffer and update the offset position
		*data++ = m_buffer[m_offset++];
	}
}
