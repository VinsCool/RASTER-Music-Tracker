#include "stdafx.h"
#include <fstream>
#include "PokeyStream.h"

#include "Atari6502.h"
#include "ChannelControl.h"

#include "General.h"
#include "global.h"


CPokeyStream::CPokeyStream()
{
	m_recordState = STREAM_STATE::STOP;
	m_StreamBuffer = NULL;
	m_BufferSize = 0;
	m_FrameCounter = 0;
	m_SonglineCounter = 0;
	memset(m_PlayCount, 0, sizeof(m_PlayCount));
	memset(m_FramesPerSongline, 0, sizeof(m_FramesPerSongline));
	memset(m_OffsetPerSongline, 0, sizeof(m_OffsetPerSongline));
	m_SongLoopedCounter = 0;
}

CPokeyStream::~CPokeyStream()
{
	if (m_StreamBuffer)
	{
		free(m_StreamBuffer);
		m_StreamBuffer = NULL;
	}
}

void CPokeyStream::Clear()
{
	if (m_StreamBuffer)
	{
		free(m_StreamBuffer);
		m_StreamBuffer = NULL;
	}
	m_BufferSize = 0;
	m_FrameCounter = 0;
	m_SonglineCounter = 0;
	memset(m_PlayCount, 0, sizeof(m_PlayCount));
	memset(m_FramesPerSongline, 0, sizeof(m_FramesPerSongline));
	memset(m_OffsetPerSongline, 0, sizeof(m_OffsetPerSongline));
	m_SongLoopedCounter = 0;
}

void CPokeyStream::StartRecording()
{
	if (m_StreamBuffer)
	{
		free(m_StreamBuffer);
		m_StreamBuffer = NULL;
	}

	m_recordState = STREAM_STATE::START;
	m_BufferSize = 0xFFFFF;
	m_StreamBuffer = (unsigned char*)calloc(m_BufferSize, 1);
	m_FrameCounter = 0;
	m_SonglineCounter = 0;
	memset(m_PlayCount, 0, sizeof(m_PlayCount));
	memset(m_FramesPerSongline, 0, sizeof(m_FramesPerSongline));
	memset(m_OffsetPerSongline, 0, sizeof(m_OffsetPerSongline));
	m_SongLoopedCounter = 0;

	m_FirstCountPoint = 0;
	m_SecondCountPoint = 0;
	m_ThirdCountPoint = 0;
}

int CPokeyStream::SwitchIntoRecording()
{
	m_recordState = STREAM_STATE::RECORD;
	m_FirstCountPoint = m_FrameCounter; 
	return m_FirstCountPoint;
}

int CPokeyStream::SwitchIntoStop()
{
	m_recordState = STREAM_STATE::STOP;
	m_SecondCountPoint = m_FrameCounter - m_FirstCountPoint;
	if (m_SecondCountPoint < 0) m_SecondCountPoint = 0;
	m_ThirdCountPoint = m_FirstCountPoint - m_SecondCountPoint;
	if (m_ThirdCountPoint < 0) m_ThirdCountPoint = 0;
	return m_SecondCountPoint;
}

void CPokeyStream::CallFromPlay(int playerState, int trackLine, int songLine)
{
	// The SAP-R dumper initialisation flag was set
	if (m_recordState == STREAM_STATE::START)	
	{
		memset(m_PlayCount, 0, sizeof(m_PlayCount));	// Reset lines play counter first
		if (playerState == MPLAY_BLOCK)
			m_PlayCount[trackLine] += 1;				// Increment the track line play count early, so it will be detected as the selection block loop
		else
			m_PlayCount[songLine] += 1;					// Increment the line play count early, to ensure that same line will be detected again as the loop point
		m_recordState = STREAM_STATE::RECORD;			// Set the SAPR dumper with the "is currently recording data" flag 
	}
}

bool CPokeyStream::TrackSongLine(int trackedInstance)
{
	if (m_recordState == STREAM_STATE::RECORD)			// The SAPR dumper is running with the "is currently recording data" flag 
	{
		m_PlayCount[trackedInstance] += 1;				// Increment the position counter by 1

		// If the Songline is played for the first time, the current frames count will be used for its index
		if (m_SongLoopedCounter < 1)
		{
			// At least 1 Songline will be played, increment the count early
			m_SonglineCounter++;
			m_OffsetPerSongline[m_SonglineCounter] = m_FrameCounter;
		}

		int count = m_PlayCount[trackedInstance];		// Fetch that line play count for the next step
		if (count > 1)
		{
			// A value above 1 means a full playback loop has been completed, the line play count incremented twice
			m_SongLoopedCounter++;						// Increment the dumper iteration count by 1
			m_recordState = STREAM_STATE::WRITE;		// Set the "write SAP-R data to file" flag
			if (m_SongLoopedCounter == 1)
			{
				memset(m_PlayCount, 0, sizeof(m_PlayCount));	// Reset the lines play count before the next step 
				m_PlayCount[trackedInstance] += 1;				// Increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 

				SwitchIntoRecording();
			}
			if (m_SongLoopedCounter == 2)
			{
				SwitchIntoStop();
				return true;
			}
		}
	}
	return false;
}

bool CPokeyStream::CallFromPlayBeat(int trackedInstance)
{
	if (m_recordState == STREAM_STATE::RECORD)	
	{
		// The SAPR dumper is running with the "is currently recording data" flag 
		m_PlayCount[trackedInstance] += 1;			// Increment the position counter by 1

		int count = m_PlayCount[trackedInstance];		// Fetch that line play count for the next step
		if (count > 1)
		{
			// A value above 1 means a full playback loop has been completed, the line play count incremented twice
			m_SongLoopedCounter++;						// Increment the dumper iteration count by 1
			m_recordState = STREAM_STATE::WRITE;		// Set the "write SAP-R data to file" flag
			if (m_SongLoopedCounter == 1)
			{
				memset(m_PlayCount, 0, sizeof(m_PlayCount));	// Reset the lines play count before the next step 
				m_PlayCount[trackedInstance] += 1;					// Increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 
			}
			if (m_SongLoopedCounter == 2)
			{
				return true;
			}
		}
	}
	return false;
}

void CPokeyStream::Record()
{
	if (m_recordState == STREAM_STATE::STOP) return;
	if (m_recordState == STREAM_STATE::START) return;		// Too soon, must first be initialised to get a constant rate every time, this prevents writing garbage in memory for the first few frames

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;			// Stereo support doubles the dumped data
	int offsetIntoSAPRBuffer = m_FrameCounter * frameSize;	// 4 AUDC, 4 AUDF, 1 AUDCTL + Second POKEY if used

	if (offsetIntoSAPRBuffer > m_BufferSize - frameSize + 1)
	{
		// Buffer is too small, grow it
		m_BufferSize *= 2;
		m_StreamBuffer = (unsigned char*)realloc(m_StreamBuffer, m_BufferSize);
	}

	// Dump Pokey sound registers to position defined by the frames counter
	// AUDF1, AUDC1, AUDF2, AUDC2, AUDF3, AUDC3, AUDF4, AUDC4, AUDCTL
	for (int i = 0; i < 9; i++)
	{
		int j = (frameSize == 18) ? 9 : 0;		// Slight offset for i count, memory can then be aligned as it is expected

		// Copy data from the 1st Pokey
		// 0 offset in mono
		// 9 offset in stereo
		m_StreamBuffer[offsetIntoSAPRBuffer + i + j] = g_atarimem[0xd200 + i];
		if (i == 1)	// AUDC1
		{	// Test SKCTL ($D20F), if Two-Tone is expected, set the Volume Only bit in the current AUDC1 offset
			m_StreamBuffer[offsetIntoSAPRBuffer + i + j] |= (g_atarimem[0xd20F] == 0x8B) ? 0x10 : 0x00;
		}

		if (frameSize == 9)
			continue;	// No second POKEY 

		// Copy data from the 2nd Pokey
		m_StreamBuffer[offsetIntoSAPRBuffer + i] = g_atarimem[0xd210 + i];
		if (i == 1)	//AUDC1
		{	//test SKCTL, if Two-Tone is expected, set the Volume Only bit in the current AUDC1 offset
			m_StreamBuffer[offsetIntoSAPRBuffer + i] |= (g_atarimem[0xd21F] == 0x8B) ? 0x10 : 0x00;
		}
	}

	// If the end was reached, do nothing, simply ignore the last frame
	if (m_SongLoopedCounter == 2) return;

	// Count the frames played in each Songline, until the first loop point is found
	if (m_SongLoopedCounter < 1)
	{
		// Increment the frames counter for the current songline
		m_FramesPerSongline[m_SonglineCounter]++; 
	}

	// Increment the frames counter for the next iteration
	m_FrameCounter++;
}

void CPokeyStream::WriteToFile(std::ofstream& ou, int frames, int offset)
{
	if (m_StreamBuffer == NULL) return;

	int bytenum = (g_tracks4_8 == 8) ? 18 : 9;
	int len = frames * bytenum;
	int off = offset * bytenum;
	ou.write((char*)m_StreamBuffer + off, len);
}

void CPokeyStream::FinishedRecording()
{
	m_recordState = STREAM_STATE::STOP;		// Reset the SAPR dump flag now it is done
	m_FrameCounter = 0;						// Also reset the framecount once finished
	m_SongLoopedCounter = 0;				// Reset the playback counter

	// Clear the allocated memory for the SAP-R dumper, TODO: manage memory dynamically instead
	if (m_StreamBuffer)
	{
		free(m_StreamBuffer);
		m_StreamBuffer = NULL;
	}

	Atari_InitRMTRoutine();	//reset the Atari memory 
	SetChannelOnOff(-1, 1);	//switch all channels back on, since they were purposefully turned off during the recording
}