// copyright (c) 2007 magnus auvinen, see licence.txt for more info
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "SDL.h"

#include "sound.h"

extern "C" { // wavpack
	#include <engine/external/wavpack/wavpack.h>
}
#include <math.h>

enum
{
	NUM_SAMPLES = 1024, //some more ;)
	NUM_VOICES = 64,
	NUM_CHANNELS = 16,

	MAX_FRAMES = 1024
};

struct CSample
{
	short *m_pData;
	int m_NumFrames;
	int m_Rate;
	int m_Channels;
	int m_LoopStart;
	int m_LoopEnd;
	int m_PausedAt;
};

struct CChannel
{
	int m_Vol;
	int m_Pan;
} ;

struct CVoice
{
	CSample *m_pSample;
	CChannel *m_pChannel;
	int m_Tick;
	int m_Vol; // 0 - 255
	int m_Flags;
	int m_X, m_Y;
} ;

static CSample m_aSamples[NUM_SAMPLES] = { {0} };
static CVoice m_aVoices[NUM_VOICES] = { {0} };
static CChannel m_aChannels[NUM_CHANNELS] = { {255, 0} };

static CSound *m_pSelf = 0;

static LOCK m_MusicLock = 0;
static LOCK m_SoundLock = 0;
static int m_SoundEnabled = 0;

static int m_CenterX = 0;
static int m_CenterY = 0;

static int m_MixingRate = 48000;
static volatile int m_SoundVolume = 100;

static int m_NextVoice = 0;

// TODO: there should be a faster way todo this
static short Int2Short(int i)
{
	if(i > 0x7fff)
		return 0x7fff;
	else if(i < -0x7fff)
		return -0x7fff;
	return i;
}

static int IntAbs(int i)
{
	if(i<0)
		return -i;
	return i;
}

static void LoadMusicWavThread(void *pUser)
{
    CSound *pSelf = (CSound *)pUser;
    while (1)
    {
        if (1) // wave
        {
            while(pSelf->m_MusicTmpBufferIn >= MUSICTMPBUFFERSIZE - 1 || !pSelf->m_MusicFileHandle)
            {
                thread_sleep(1);

            }
            lock_wait(m_MusicLock);
            if (pSelf->m_MusicFinished || !pSelf->m_MusicPlaying || !pSelf->m_MusicFileHandle)
            {
                mem_zero((void *)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferRead], pSelf->m_MusicFrameCount * 4);
            }
            else if (pSelf->m_MusicFileHandle)
            {
                if (pSelf->m_MusicSeekStart)
                {

                    io_read(pSelf->m_MusicFileHandle, &pSelf->m_MusicWaveFileHeader, sizeof(pSelf->m_MusicWaveFileHeader));
                    #if defined(CONF_ARCH_ENDIAN_BIG)
                        swap_endian(&pSelf->m_MusicWaveFileHeader.riff_name, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.riff_laenge, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.riff_type, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.fmt_name, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.fmt_laenge, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.formattyp, 2, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.kanalzahl, 2, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.samplerate, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.b_pro_sec, 4, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.b_pro_sample, 2, 1);
                        swap_endian(&pSelf->m_MusicWaveFileHeader.Bits_per_sample, 2, 1);
                    #endif
                    pSelf->m_MusicPlayedBytes = pSelf->m_MusicWaveFileHeader.fmt_laenge + 21;
                    io_seek(pSelf->m_MusicFileHandle, pSelf->m_MusicWaveFileHeader.fmt_laenge + 21, IOSEEK_START);
                    pSelf->m_MusicSeekStart = false;
                }
                mem_zero((void *)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferRead], pSelf->m_MusicFrameCount * 4);
                pSelf->m_MusicPlayedBytes = pSelf->m_MusicPlayedBytes + pSelf->m_MusicFrameCount * 4;
                if (io_read(pSelf->m_MusicFileHandle, (void *)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferRead], pSelf->m_MusicFrameCount * 4) == 0)
                {
                    pSelf->m_MusicFinished = true;
                    pSelf->m_MusicPlayedBytes = pSelf->m_MusicWaveFileHeader.fmt_laenge + 21;
                    pSelf->m_MusicPlayIndex++;
                }
                pSelf->m_MusicTmpBufferRead++;
                pSelf->m_MusicTmpBufferIn++;
                if (pSelf->m_MusicTmpBufferRead >= MUSICTMPBUFFERSIZE)
                    pSelf->m_MusicTmpBufferRead = 0;
            }
            lock_release(m_MusicLock);
        }
        else
        {

        }
    }
}

static void Mix(short *pFinalOut, unsigned Frames)
{
    CSound *pSelf = m_pSelf;
	int aMixBuffer[MAX_FRAMES*2] = {0};
	int aMusicBuffer[MAX_FRAMES*2] = {0};
	int MasterVol;
	int MasterMusicVol;

	// aquire lock while we are mixing
	lock_wait(m_SoundLock);

	MasterVol = m_SoundVolume;
	MasterMusicVol = pSelf->m_MusicVolume;

	for(unsigned i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample)
		{
			// mix voice
			CVoice *v = &m_aVoices[i];
			int *pOut = aMixBuffer;

			int Step = v->m_pSample->m_Channels; // setup input sources
			short *pInL = &v->m_pSample->m_pData[v->m_Tick*Step];
			short *pInR = &v->m_pSample->m_pData[v->m_Tick*Step+1];

			unsigned End = v->m_pSample->m_NumFrames-v->m_Tick;

			int Rvol = v->m_pChannel->m_Vol;
			int Lvol = v->m_pChannel->m_Vol;

			// make sure that we don't go outside the sound data
			if(Frames < End)
				End = Frames;

			// check if we have a mono sound
			if(v->m_pSample->m_Channels == 1)
				pInR = pInL;

			// volume calculation
			if(v->m_Flags&ISound::FLAG_POS && v->m_pChannel->m_Pan)
			{
				// TODO: we should respect the channel panning value
				const int Range = 1500; // magic value, remove
				int dx = v->m_X - m_CenterX;
				int dy = v->m_Y - m_CenterY;
				int Dist = (int)sqrtf((float)dx*dx+dy*dy); // float here. nasty
				int p = IntAbs(dx);
				if(Dist >= 0 && Dist < Range)
				{
					// panning
					if(dx > 0)
						Lvol = ((Range-p)*Lvol)/Range;
					else
						Rvol = ((Range-p)*Rvol)/Range;

					// falloff
					Lvol = (Lvol*(Range-Dist))/Range;
					Rvol = (Rvol*(Range-Dist))/Range;
				}
				else
				{
					Lvol = 0;
					Rvol = 0;
				}
			}

			// process all frames
			for(unsigned s = 0; s < End; s++)
			{
				*pOut++ += (*pInL)*Lvol;
				*pOut++ += (*pInR)*Rvol;
				pInL += Step;
				pInR += Step;
				v->m_Tick++;
			}

			// free voice if not used any more
			if(v->m_Tick == v->m_pSample->m_NumFrames)
            {
                if(v->m_Flags&ISound::FLAG_LOOP)
                    v->m_Tick = 0;
                else
                    v->m_pSample = 0;
            }

		}
	}
    if (!pSelf->m_MusicTmpBuffer[0])
    {
        for (int i = 0; i < MUSICTMPBUFFERSIZE; i++)
            pSelf->m_MusicTmpBuffer[i] = new char[Frames * 4];
        thread_create(LoadMusicWavThread, (void *)pSelf);
    }
    else if(!pSelf->m_MusicFinished && pSelf->m_MusicPlaying && pSelf->m_MusicFileHandle)
    {
        pSelf->m_MusicPeakL = 0;
        pSelf->m_MusicPeakR = 0;
        if (pSelf->m_MusicTmpBufferIn > 1)
        {
            for(unsigned i = 0; i < Frames; i++)
            {
                aMusicBuffer[i * 2] = (int)(((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 0]) * 256 + ((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 1])) * 100;
                aMusicBuffer[i * 2 + 1] = (int)(((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 2]) * 256 + ((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 3])) * 100;
                pSelf->m_MusicPeakL += (int)(((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 0]) * 256 + ((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 1]));
                pSelf->m_MusicPeakR += (int)(((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 2]) * 256 + ((unsigned int)pSelf->m_MusicTmpBuffer[pSelf->m_MusicTmpBufferWrite][i * 4 + 3]));
            }
            pSelf->m_MusicTmpBufferWrite++;
            pSelf->m_MusicTmpBufferIn--;
            if (pSelf->m_MusicTmpBufferWrite >= MUSICTMPBUFFERSIZE)
                pSelf->m_MusicTmpBufferWrite = 0;
        }
        else
        {
            mem_zero(aMusicBuffer, Frames * 2);
        }
    }
    if (pSelf->m_MusicLoadNew == true && pSelf->m_MusicFileName[0])
    {
        lock_wait(m_MusicLock);
        if (pSelf->m_MusicFileHandle)
        {
            io_close(pSelf->m_MusicFileHandle);
            pSelf->m_MusicFileHandle = 0;
        }
        pSelf->m_MusicFileHandle = io_open(pSelf->m_MusicFileName, IOFLAG_READ);
        if (pSelf->m_MusicFileHandle)
        {
            pSelf->m_MusicLoadNew = false;
            pSelf->m_MusicFinished = false;
            pSelf->m_MusicSeekStart = true;
            pSelf->m_MusicPlaying = true;
            pSelf->m_MusicTmpBufferIn = 0;
            pSelf->m_MusicTmpBufferWrite = 0;
            pSelf->m_MusicTmpBufferRead = 0;
        }
        else
        {
            pSelf->m_MusicPlayIndex++;
            pSelf->m_MusicLoadNew = false;
        }
        lock_release(m_MusicLock);
    }
    if (pSelf->m_MusicTmpBufferIn < -10)
        pSelf->m_MusicPlaying = false;
    pSelf->m_MusicFrameCount = Frames;

	// release the lock
	lock_release(m_SoundLock);

	{
		// clamp accumulated values
		// TODO: this seams slow
		for(unsigned i = 0; i < Frames; i++)
		{
			int j = i<<1;
			int vl = ((aMixBuffer[j]*MasterVol)/101)>>8;
			vl += ((aMusicBuffer[j]*MasterMusicVol)/101)>>8;
			int vr = ((aMixBuffer[j+1]*MasterVol)/101)>>8;
			vr += ((aMusicBuffer[j+1]*MasterMusicVol)/101)>>8;

			pFinalOut[j] = Int2Short(vl);
			pFinalOut[j+1] = Int2Short(vr);
		}
	}

    static int sRecordAudio = 0;
    if (pSelf->m_RecordAudio == 1) //record audio
    {
        static IOHANDLE AudioOut;
        if (sRecordAudio == 0)
        {
            AudioOut = pSelf->m_pStorage->OpenFile("tmp/pixelstream/sound.wav", IOFLAG_WRITE, IStorage::TYPE_SAVE);
        }
        if (1)
        {
            const char x = 0;
            str_copy(pSelf->m_RecordWaveFileHeader.riff_name, "RIFF", 5);
            str_copy(pSelf->m_RecordWaveFileHeader.riff_type, "WAVE", 5);
            str_copy(pSelf->m_RecordWaveFileHeader.fmt_name, "fmt ", 5);
            pSelf->m_RecordWaveFileHeader.riff_laenge = sRecordAudio * Frames * 2 * 2 + sizeof(pSelf->m_RecordWaveFileHeader);
            pSelf->m_RecordWaveFileHeader.fmt_laenge = 16;
            pSelf->m_RecordWaveFileHeader.formattyp = 1;
            pSelf->m_RecordWaveFileHeader.kanalzahl = 2;
            pSelf->m_RecordWaveFileHeader.samplerate = 44100;
            pSelf->m_RecordWaveFileHeader.b_pro_sec = 44100 * 2 * 2;
            pSelf->m_RecordWaveFileHeader.b_pro_sample = 4;
            pSelf->m_RecordWaveFileHeader.Bits_per_sample = 16;

            io_seek(AudioOut, 0, IOSEEK_START);
            io_write(AudioOut, &pSelf->m_RecordWaveFileHeader, sizeof(pSelf->m_RecordWaveFileHeader));
            io_write(AudioOut, "data", sizeof("data"));
            io_write(AudioOut, &x, 1);
            io_write(AudioOut, &x, 1);
            io_write(AudioOut, &x, 1);
            io_seek(AudioOut, 0, IOSEEK_END);

            io_write(AudioOut, pFinalOut, Frames * 2 * 2);
        }
        sRecordAudio++;
    }
    else
    {
        sRecordAudio = 0;
    }

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(pFinalOut, sizeof(short), Frames * 2);
#endif
}

static void SdlCallback(void *pUnused, Uint8 *pStream, int Len)
{
	(void)pUnused;
	Mix((short *)pStream, Len/2/2);
}

int CSound::Init()
{
	m_SoundEnabled = 0;
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();

	SDL_AudioSpec Format;

    //init music
    m_MusicTmpBuffer[0] = 0;
    m_MusicTmpBufferIn = 0;
    m_MusicTmpBufferRead = 0;
    m_MusicTmpBufferWrite = 0;
    m_MusicFrameCount = 0;
    m_MusicFileHandle = 0;
    m_MusicSeekStart = true;
    m_MusicFinished = true;
    m_MusicPlaying = false;
    m_MusicVolume = 100;
    m_MusicPeakL = 0;
    m_MusicPeakR = 0;
    m_pSelf = this; // needed for the mixing process

    m_RecordAudio = 1;

	m_MusicLock = lock_create();
	m_SoundLock = lock_create();

	if(!g_Config.m_SndEnable)
		return 0;

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		dbg_msg("gfx", "unable to init SDL audio: %s", SDL_GetError());
		return -1;
	}

	m_MixingRate = g_Config.m_SndRate;

	// Set 16-bit stereo audio at 22Khz
	Format.freq = g_Config.m_SndRate; // ignore_convention
	Format.format = AUDIO_S16; // ignore_convention
	Format.channels = 2; // ignore_convention
	Format.samples = g_Config.m_SndBufferSize; // ignore_convention
	Format.callback = SdlCallback; // ignore_convention
	Format.userdata = NULL; // ignore_convention

	// Open the audio device and start playing sound!
	if(SDL_OpenAudio(&Format, NULL) < 0)
	{
		dbg_msg("client/sound", "unable to open audio: %s", SDL_GetError());
		return -1;
	}
	else
		dbg_msg("client/sound", "sound init successful");


	SDL_PauseAudio(0);

	m_SoundEnabled = 1;
	Update(); // update the volume
	return 0;
}

int CSound::Update()
{
	// update volume
	int WantedVolume = g_Config.m_SndVolume;
    int WantedMusicVolume = g_Config.m_SndMusicVolume;

	if(!m_pGraphics->WindowActive() && g_Config.m_SndNonactiveMute)
	{
		WantedVolume = 0;
	    WantedMusicVolume = 0;
	}

	if(WantedVolume != m_SoundVolume)
	{
		lock_wait(m_SoundLock);
		m_SoundVolume = WantedVolume;
		lock_release(m_SoundLock);
	}
	if(WantedMusicVolume != m_MusicVolume)
	{
		lock_wait(m_SoundLock);
		m_MusicVolume = WantedMusicVolume;
		lock_release(m_SoundLock);
	}

	return 0;
}

int CSound::Shutdown()
{
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	lock_destroy(m_SoundLock);
	lock_destroy(m_MusicLock);



	return 0;
}

int CSound::AllocID()
{
	// TODO: linear search, get rid of it
	for(unsigned SampleID = 0; SampleID < NUM_SAMPLES; SampleID++)
	{
		if(m_aSamples[SampleID].m_pData == 0x0)
			return SampleID;
	}

	return -1;
}

void CSound::RateConvert(int SampleID)
{
	CSample *pSample = &m_aSamples[SampleID];
	int NumFrames = 0;
	short *pNewData = 0;

	// make sure that we need to convert this sound
	if(!pSample->m_pData || pSample->m_Rate == m_MixingRate)
		return;

	// allocate new data
	NumFrames = (int)((pSample->m_NumFrames/(float)pSample->m_Rate)*m_MixingRate);
	pNewData = (short *)mem_alloc(NumFrames*pSample->m_Channels*sizeof(short), 1);

	for(int i = 0; i < NumFrames; i++)
	{
		// resample TODO: this should be done better, like linear atleast
		float a = i/(float)NumFrames;
		int f = (int)(a*pSample->m_NumFrames);
		if(f >= pSample->m_NumFrames)
			f = pSample->m_NumFrames-1;

		// set new data
		if(pSample->m_Channels == 1)
			pNewData[i] = pSample->m_pData[f];
		else if(pSample->m_Channels == 2)
		{
			pNewData[i*2] = pSample->m_pData[f*2];
			pNewData[i*2+1] = pSample->m_pData[f*2+1];
		}
	}

	// free old data and apply new
	mem_free(pSample->m_pData);
	pSample->m_pData = pNewData;
	pSample->m_NumFrames = NumFrames;
}

int CSound::ReadData(void *pBuffer, int Size)
{
	return io_read(ms_File, pBuffer, Size);
}

int CSound::LoadWV(const char *pFilename)
{
	CSample *pSample;
	int SampleID = -1;
	char aError[100];
	WavpackContext *pContext;

	// don't waste memory on sound when we are stress testing
	if(g_Config.m_DbgStress)
		return -1;

	// no need to load sound when we are running with no sound
	if(!m_SoundEnabled)
		return 1;

	if(!m_pStorage)
		return -1;
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "downloadfiles/%s", pFilename);
	ms_File = m_pStorage->OpenFile((char *)aBuf, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!ms_File)
	{
		ms_File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
		if(!ms_File)
		{
			dbg_msg("sound/wv", "failed to open file. filename='%s'", pFilename);
			return -1;
		}
	}
	SampleID = AllocID();
	if(SampleID < 0)
		return -1;
	pSample = &m_aSamples[SampleID];

	pContext = WavpackOpenFileInput(ReadData, aError);
	if (pContext)
	{
		int m_aSamples = WavpackGetNumSamples(pContext);
		int BitsPerSample = WavpackGetBitsPerSample(pContext);
		unsigned int SampleRate = WavpackGetSampleRate(pContext);
		int m_aChannels = WavpackGetNumChannels(pContext);
		int *pData;
		int *pSrc;
		short *pDst;
		int i;

		pSample->m_Channels = m_aChannels;
		pSample->m_Rate = SampleRate;

		if(pSample->m_Channels > 2)
		{
			dbg_msg("sound/wv", "file is not mono or stereo. filename='%s'", pFilename);
			return -1;
		}

		/*
		if(snd->rate != 44100)
		{
			dbg_msg("sound/wv", "file is %d Hz, not 44100 Hz. filename='%s'", snd->rate, filename);
			return -1;
		}*/

		if(BitsPerSample != 16)
		{
			dbg_msg("sound/wv", "bps is %d, not 16, filname='%s'", BitsPerSample, pFilename);
			return -1;
		}

		pData = (int *)mem_alloc(4*m_aSamples*m_aChannels, 1);
		WavpackUnpackSamples(pContext, pData, m_aSamples); // TODO: check return value
		pSrc = pData;

		pSample->m_pData = (short *)mem_alloc(2*m_aSamples*m_aChannels, 1);
		pDst = pSample->m_pData;

		for (i = 0; i < m_aSamples*m_aChannels; i++)
			*pDst++ = (short)*pSrc++;

		mem_free(pData);

		pSample->m_NumFrames = m_aSamples;
		pSample->m_LoopStart = -1;
		pSample->m_LoopEnd = -1;
		pSample->m_PausedAt = 0;
	}
	else
	{
		dbg_msg("sound/wv", "failed to open %s: %s", pFilename, aError);
	}

	io_close(ms_File);
	ms_File = NULL;

	if(g_Config.m_Debug)
		dbg_msg("sound/wv", "loaded %s", pFilename);

	RateConvert(SampleID);
	return SampleID;
}

void CSound::SetListenerPos(float x, float y)
{
	m_CenterX = (int)x;
	m_CenterY = (int)y;
}


void CSound::SetChannel(int ChannelID, float Vol, float Pan)
{
	m_aChannels[ChannelID].m_Vol = (int)(Vol*255.0f);
	m_aChannels[ChannelID].m_Pan = (int)(Pan*255.0f); // TODO: this is only on and off right now
}

int CSound::Play(int ChannelID, int SampleID, int Flags, float x, float y)
{
	int VoiceID = -1;
	int i;

	lock_wait(m_SoundLock);

	// search for voice
	for(i = 0; i < NUM_VOICES; i++)
	{
		int id = (m_NextVoice + i) % NUM_VOICES;
		if(!m_aVoices[id].m_pSample)
		{
			VoiceID = id;
			m_NextVoice = id+1;
			break;
		}
	}

	// voice found, use it
	if(VoiceID != -1)
	{
		m_aVoices[VoiceID].m_pSample = &m_aSamples[SampleID];
		m_aVoices[VoiceID].m_pChannel = &m_aChannels[ChannelID];
		if(Flags & FLAG_LOOP)
			m_aVoices[VoiceID].m_Tick = m_aSamples[SampleID].m_PausedAt;
		else
			m_aVoices[VoiceID].m_Tick = 0;
		m_aVoices[VoiceID].m_Vol = 255;
		m_aVoices[VoiceID].m_Flags = Flags;
		m_aVoices[VoiceID].m_X = (int)x;
		m_aVoices[VoiceID].m_Y = (int)y;
	}

	lock_release(m_SoundLock);
	return VoiceID;
}

int CSound::PlayAt(int ChannelID, int SampleID, int Flags, float x, float y)
{
	return Play(ChannelID, SampleID, Flags|ISound::FLAG_POS, x, y);
}

int CSound::Play(int ChannelID, int SampleID, int Flags)
{
	return Play(ChannelID, SampleID, Flags, 0, 0);
}

void CSound::Stop(int SampleID)
{
	// TODO: a nice fade out
	lock_wait(m_SoundLock);
	CSample *pSample = &m_aSamples[SampleID];
	for(int i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample == pSample)
		{
			if(m_aVoices[i].m_Flags & FLAG_LOOP)
				m_aVoices[i].m_pSample->m_PausedAt = m_aVoices[i].m_Tick;
			else
				m_aVoices[i].m_pSample->m_PausedAt = 0;
			m_aVoices[i].m_pSample = 0;
		}
	}
	lock_release(m_SoundLock);
}

void CSound::StopAll()
{
	// TODO: a nice fade out
	lock_wait(m_SoundLock);
	for(int i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample)
		{
			if(m_aVoices[i].m_Flags & FLAG_LOOP)
				m_aVoices[i].m_pSample->m_PausedAt = m_aVoices[i].m_Tick;
			else
				m_aVoices[i].m_pSample->m_PausedAt = 0;
		}
		m_aVoices[i].m_pSample = 0;
	}
	lock_release(m_SoundLock);
}

IOHANDLE CSound::ms_File = 0;

IEngineSound *CreateEngineSound() { return new CSound; }

