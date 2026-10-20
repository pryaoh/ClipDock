#pragma once
#include "Waveout.h"
#include <list>
#include "../mp3lib/mad.h"
# include "audio.h"

#define MPEG_BUFSZ 10000

class CMP3Play
{
public:
	CRITICAL_SECTION  cs;
	struct mad_decoder mdec;
	unsigned char data[MPEG_BUFSZ];
	unsigned long length;
	struct audio_stats stats;
	struct audio_dither left_dither, right_dither;
	unsigned char pcmdata[10000];
	unsigned int SampleRate;
	int iCount;
	bool bTest;
	bool bDelayed;
	double nOldSample;

	unsigned long	lLastPlayTime;
	unsigned long	lLastTimeStemp;
	char *			UserName[64];

	HINSTANCE m_hMp3Dec;

	std::list<char*> m_buffers;

	int ScheduledBufferCount;
	bool bDone;

	CWaveout m_WaveOut;
	CMP3Play(void);
	void Start(unsigned int SampleRate,unsigned int Bit,unsigned int nChanel);
	void AddBuffer(char * pData,int len);
	void Run ();
	void Stop();
public:
	~CMP3Play(void);
};
