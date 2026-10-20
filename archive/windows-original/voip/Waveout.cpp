#include "StdAfx.h"

#include "Waveout.h"


static int
Box ( const char* msg )
{
	//MessageBox ( NULL, msg,_T( " : Error Message . . ."), MB_OK | MB_ICONEXCLAMATION );
	return -1;
}

static void CALLBACK
wave_callback ( HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	CWaveout * pWaveOut;
	pWaveOut = (CWaveout  *)dwInstance;
	if ( uMsg == WOM_DONE ) {
		EnterCriticalSection ( &pWaveOut->cs );
		pWaveOut->PlayedWaveHeaders [pWaveOut->PlayedWaveHeadersCount++] = (WAVEHDR*) dwParam1;
		LeaveCriticalSection ( &pWaveOut->cs );
	}
}

void
CWaveout::free_memory ( void )
{
	WAVEHDR*  wh;
	HGLOBAL   hg;

	EnterCriticalSection ( &cs );
	wh = PlayedWaveHeaders [--PlayedWaveHeadersCount];
	ScheduledBlocks--;                               // decrease the number of USED blocks
	LeaveCriticalSection ( &cs );

	waveOutUnprepareHeader ( dev, wh, sizeof (WAVEHDR) );

	hg = GlobalHandle ( wh -> lpData );       // Deallocate the buffer memory
	GlobalUnlock (hg);
	GlobalFree   (hg);

	hg = GlobalHandle ( wh );                 // Deallocate the header memory
	GlobalUnlock (hg);
	GlobalFree   (hg);
}

int CWaveout::Set_WIN_Params (long double SampleFreq, 
							  unsigned int BitsPerSample, 
							  unsigned int Channels )
{
	
	UINT          deviceID = WAVE_MAPPER;

	ScheduledBlocks        = 0;


	if ( waveOutGetNumDevs () == 0 )
		return Box ( "No audio device present." );

	outFormat.wFormatTag      = WAVE_FORMAT_PCM;
	outFormat.wBitsPerSample  = BitsPerSample;
	outFormat.nChannels       = Channels;
	outFormat.nSamplesPerSec  = (unsigned long)(SampleFreq );//+ 0.5);
	outFormat.nBlockAlign     = outFormat.nChannels * (outFormat.wBitsPerSample /4);//(outFormat.wBitsPerSample + 7) / 8 * outFormat.nChannels;
	outFormat.nAvgBytesPerSec = outFormat.nSamplesPerSec * outFormat.nBlockAlign;

	switch ( waveOutOpen ( &dev, deviceID, &outFormat, (DWORD)wave_callback,(DWORD)this, CALLBACK_FUNCTION ) )
	{
		case MMSYSERR_ALLOCATED:   return Box ( "Device is already open." );
		case MMSYSERR_BADDEVICEID: return Box ( "The specified device is out of range." );
		case MMSYSERR_NODRIVER:    return Box ( "There is no audio driver in this system." );
		case MMSYSERR_NOMEM:       return Box ( "Unable to allocate sound memory." );
		case WAVERR_BADFORMAT:     return Box ( "This audio format is not supported." );
		case WAVERR_SYNC:          return Box ( "The device is synchronous." );
		default:                   return Box ( "Unknown media error." );
		case MMSYSERR_NOERROR:     break;
	}

	waveOutReset ( dev );

	SetPriorityClass ( GetCurrentProcess (), HIGH_PRIORITY_CLASS );

	bFast = false;

	return 0;
}

int 
CWaveout::GetVolume()
{	
	DWORD dwVolume;
	int percentVol;

	if( waveOutGetVolume( dev ,&dwVolume) != MMSYSERR_NOERROR ) 
	{
		//int a=0;
	}

	float leftVol = LOWORD(dwVolume);
	float rightVol = HIWORD(dwVolume);
	//float left2 = leftVol;
	//float right2= rightVol;

	if(leftVol > rightVol)
		percentVol = (int)((leftVol/65536)*100);
	else
		percentVol = (int)((leftVol/65536)*100);

	return percentVol;
}



int
CWaveout::WIN_Play_Samples ( const void* data, size_t len )
{
	HGLOBAL    hg;
	HGLOBAL    hg2;
	LPWAVEHDR  wh;
	void*      allocptr;


	do {
		while ( PlayedWaveHeadersCount > 0 )                // free used blocks ...
			free_memory ();

		if ( ScheduledBlocks < sizeof(PlayedWaveHeaders)/sizeof(*PlayedWaveHeaders) ) // wait for a free block ...
			break;
		Sleep (26);
	} while (1);

	if(ScheduledBlocks >6 )
	{
		
		if(!bFast)
		{
			waveOutSetPlaybackRate(dev,(DWORD)((double)SampleRate*1.2));
			bFast = true;
		}

	} else if(ScheduledBlocks <3 ){

		//waveOutGetPlaybackRate(dev,&rate);
		if(bFast)
		{
			waveOutSetPlaybackRate(dev,SampleRate);
			bFast = false;
		}
	}
	if ( (hg2 = GlobalAlloc ( GMEM_MOVEABLE, len )) == NULL )   // allocate some memory for a copy of the buffer
		return Box ( "GlobalAlloc failed." );

	allocptr = GlobalLock (hg2);
	CopyMemory ( allocptr, data, len );                         // Here we can call any modification output functions we want....

	if ( (hg = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (WAVEHDR))) == NULL ) // now make a header and WRITE IT!
		return -1;

	wh                   = (LPWAVEHDR)GlobalLock (hg);
	wh -> dwBufferLength = len;
	wh -> lpData         = (char *)allocptr;

	if ( waveOutPrepareHeader ( dev, wh, sizeof (WAVEHDR)) != MMSYSERR_NOERROR ) {
		GlobalUnlock (hg);
		GlobalFree   (hg);
		return -1;
	}

	if ( waveOutWrite ( dev, wh, sizeof (WAVEHDR)) != MMSYSERR_NOERROR ) {
		GlobalUnlock (hg);
		GlobalFree   (hg);
		return -1;
	}
	EnterCriticalSection ( &cs );
	ScheduledBlocks++;
	LeaveCriticalSection ( &cs );
	return len;
}

int CWaveout::WIN_Audio_Reset ( long double SampleFreq)
{
	if ( dev != NULL ) {

		while ( ScheduledBlocks > 0 ) {
			Sleep (ScheduledBlocks);
			while ( PlayedWaveHeadersCount > 0 )         // free used blocks ...
				free_memory ();
		}

		waveOutReset (dev);      // reset the device
		waveOutClose (dev);      // close the device
		dev = NULL;
	}

	UINT          deviceID = WAVE_MAPPER;

	if ( waveOutGetNumDevs () == 0 )
		return Box ( "No audio device present." );

	outFormat.nSamplesPerSec  = (unsigned long)(SampleFreq );//+ 0.5);

	switch ( waveOutOpen ( &dev, deviceID, &outFormat, (DWORD)wave_callback,(DWORD)this, CALLBACK_FUNCTION ) )
	{
		case MMSYSERR_ALLOCATED:   return Box ( "Device is already open." );
		case MMSYSERR_BADDEVICEID: return Box ( "The specified device is out of range." );
		case MMSYSERR_NODRIVER:    return Box ( "There is no audio driver in this system." );
		case MMSYSERR_NOMEM:       return Box ( "Unable to allocate sound memory." );
		case WAVERR_BADFORMAT:     return Box ( "This audio format is not supported." );
		case WAVERR_SYNC:          return Box ( "The device is synchronous." );
		default:                   return Box ( "Unknown media error." );
		case MMSYSERR_NOERROR:     break;
	}

	waveOutReset ( dev );

	SetPriorityClass ( GetCurrentProcess (), HIGH_PRIORITY_CLASS );

	bFast = false;
}


int
CWaveout::WIN_Audio_close ( void )
{
	if ( dev != NULL ) {

		while ( ScheduledBlocks > 0 ) {
			Sleep (ScheduledBlocks);
			while ( PlayedWaveHeadersCount > 0 )         // free used blocks ...
				free_memory ();
		}

		waveOutReset (dev);      // reset the device
		waveOutClose (dev);      // close the device
		dev = NULL;
	}


	ScheduledBlocks = 0;
	return 0;
}

