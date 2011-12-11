/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SOUND_H
#define ENGINE_SOUND_H

#include "kernel.h"

class ISound : public IInterface
{
	MACRO_INTERFACE("sound", 0)
public:
	enum
	{
		FLAG_LOOP=1,
		FLAG_POS=2,
		FLAG_ALL=3
	};

	virtual bool IsSoundEnabled() = 0;

	virtual int LoadWV(const char *pFilename) = 0;

	virtual void SetChannel(int ChannelID, float Volume, float Panning) = 0;
	virtual void SetListenerPos(float x, float y) = 0;

	virtual int PlayAt(int ChannelID, int SampleID, int Flags, float x, float y) = 0;
	virtual int Play(int ChannelID, int SampleID, int Flags) = 0;
	virtual void Stop(int SampleID) = 0;
	virtual void StopAll() = 0;

//music
#define MUSICTMPBUFFERSIZE 20
struct CWaveHeader
{
	// RIFF - CHUNK
	char riff_name[4]; // FOURCC "RIFF"
	long riff_laenge;  // Length of the Riffchunks
	char riff_type[4];  // FOURCC "WAVE"
	// FMT - CHUNK
	char fmt_name[4];   // FOURCC "FMT "
	long fmt_laenge;    // Length of the fmt-chunks
	short formattyp;    // 0=Mono, 1=Stereo
	short kanalzahl;	// Number of chanels
	long samplerate;	// Sample-Rate in Hz
	long b_pro_sec;		// Bytes per Sekunde/*static*//*static*/
	short b_pro_sample;	// Bytes per Sample 1=8bitMono 2=8bitStereo 4=16bitStereo
	short Bits_per_sample;
};
volatile char *m_MusicTmpBuffer[MUSICTMPBUFFERSIZE];
char m_MusicFileName[1024];
volatile int m_MusicTmpBufferIn;
volatile int m_MusicTmpBufferRead;
volatile int m_MusicTmpBufferWrite;
volatile int m_MusicFrameCount;
volatile IOHANDLE m_MusicFileHandle;
CWaveHeader m_MusicWaveFileHeader;
volatile bool m_MusicSeekStart;
volatile bool m_MusicLoadNew;
volatile bool m_MusicFinished;
volatile bool m_MusicPlaying;
volatile int m_MusicVolume;
volatile int m_MusicPeakL;
volatile int m_MusicPeakR;
volatile int m_MusicPlayIndex;
volatile int m_MusicPlayedBytes;

bool m_RecordAudio;
CWaveHeader m_RecordWaveFileHeader;

};


class IEngineSound : public ISound
{
	MACRO_INTERFACE("enginesound", 0)
public:
	virtual int Init() = 0;
	virtual int Update() = 0;
	virtual int Shutdown() = 0;
};

extern IEngineSound *CreateEngineSound();

#endif
