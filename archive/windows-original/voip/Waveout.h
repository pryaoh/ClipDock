#pragma once
#include "Mmsystem.h"

#define MAX_WAVEBLOCKS    64

class CWaveout
{
public:
	CWaveout()
	{
		dev                    = NULL;
		PlayedWaveHeadersCount = 0;
		InitializeCriticalSection ( &cs );
	};
	~CWaveout()
	{
		DeleteCriticalSection ( &cs );
	}

	CRITICAL_SECTION	cs;
	HWAVEOUT			dev;
	DWORD				SampleRate;
	bool				bFast;

	int               PlayedWaveHeadersCount;          // free index
	WAVEHDR*          PlayedWaveHeaders [MAX_WAVEBLOCKS];


	WAVEFORMATEX  outFormat;
	int               ScheduledBlocks     ;
	void free_memory();
	int Set_WIN_Params (long double SampleFreq,unsigned int BitsPerSample,unsigned int Channels );
	int WIN_Play_Samples ( const void* data, size_t len );
	int WIN_Audio_Reset (long double SampleFreq);
	int WIN_Audio_close ( void );

	int GetVolume();
public:

};
