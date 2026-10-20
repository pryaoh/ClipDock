#include "StdAfx.h"
#include "MP3Play.h"

#include "../mp3lib/mad.h"

/*
#if _DEBUG
	#pragma comment(lib,"mp3lib/libmadd.lib")
#else 
	#pragma comment(lib,"mp3lib/libmad.lib")
#endif
*/


typedef unsigned (__stdcall *PTHREAD_START) (void *);

#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, \
   pvParam, fdwCreate, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                      \
         (void *)        (psa),                     \
         (unsigned)      (cbStack),                 \
         (PTHREAD_START) (pfnStartAddr),            \
         (void *)        (pvParam),                 \
         (unsigned)      (fdwCreate),               \
         (unsigned *)    (pdwThreadId)))

static enum mad_flow play_input(void *arg, struct mad_stream *stream)
{
	
	CMP3Play * pMp3Play;
	pMp3Play =(CMP3Play *) arg;

	if(pMp3Play->bDone) 
	{
		return MAD_FLOW_STOP;
	}

	int len;

	if (stream->next_frame) {

		pMp3Play->length = &pMp3Play->data[pMp3Play->length] - stream->next_frame;
		memmove(pMp3Play->data, stream->next_frame,pMp3Play->length);
	}
	
	pMp3Play->iCount = 0;

	if(1)
	{
		while (pMp3Play->iCount < 1 )
		{
			if(!pMp3Play) return MAD_FLOW_STOP;
			try
			{
				//EnterCriticalSection ( &pMp3Play->cs );
				//if(pMp3Play->ScheduledBufferCount< 1)
				{
				//	Sleep(300);				
				}
				pMp3Play->iCount = pMp3Play->ScheduledBufferCount;
				//LeaveCriticalSection ( &pMp3Play->cs );
			} catch(...)
			{
				return MAD_FLOW_STOP;			
			}

			Sleep(1);
		}
	} else {

		while (pMp3Play->iCount < 40)
		{
			EnterCriticalSection ( &pMp3Play->cs );
				pMp3Play->iCount = pMp3Play->ScheduledBufferCount;
			LeaveCriticalSection ( &pMp3Play->cs );

			Sleep(10);
		}

		pMp3Play->bTest =true;

	}

	char * mp3buffer = NULL;
	short sSize = 0;
	EnterCriticalSection ( &pMp3Play->cs );
	
		list<char *>::iterator pos =  pMp3Play->m_buffers.begin();
		mp3buffer = *pos;
		pMp3Play->m_buffers.erase(pos);
		pMp3Play->ScheduledBufferCount--;

	LeaveCriticalSection ( &pMp3Play->cs );
	memcpy(&sSize,mp3buffer,2); 

//		char tmpMsg[255];
//		sprintf(tmpMsg	,"ScheduledBufferCount ; %d \r\n",pMp3Play->ScheduledBufferCount);
//		OutputDebugStringA(tmpMsg);

	memcpy(pMp3Play->data+pMp3Play->length,mp3buffer+2,sSize);
	len = sSize;
	delete [] mp3buffer;
 

  if (len == -1) {
    return MAD_FLOW_BREAK;
  }
  else if (len == 0) {
    while (len < MAD_BUFFER_GUARD)
      pMp3Play->data[pMp3Play->length + len++] = 0;
  }

  mad_stream_buffer(stream, (unsigned char *)pMp3Play->data, pMp3Play->length += len);

	return (MAD_FLOW_CONTINUE);
}

//madplayer 에서 뽑아온 소스
static inline
unsigned long prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}


//madplayer 에서 뽑아온 소스
inline
signed long audio_linear_dither(unsigned int bits, mad_fixed_t sample,
				struct audio_dither *dither,
				struct audio_stats *stats)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX =  MAD_F_ONE - 1
  };

  /* noise shape */
  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  /* bias */
  output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

  scalebits = MAD_F_FRACBITS + 1 - bits;
  mask = (1L << scalebits) - 1;

  /* dither */
  random  = prng(dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  /* clip */
  if (output >= stats->peak_sample) {
    if (output > MAX) {
      ++stats->clipped_samples;
      if (output - MAX > stats->peak_clipping)
	stats->peak_clipping = output - MAX;

      output = MAX;

      if (sample > MAX)
	sample = MAX;
    }
    stats->peak_sample = output;
  }
  else if (output < -stats->peak_sample) {
    if (output < MIN) {
      ++stats->clipped_samples;
      if (MIN - output > stats->peak_clipping)
	stats->peak_clipping = MIN - output;

      output = MIN;

      if (sample < MIN)
	sample = MIN;
    }
    stats->peak_sample = -output;
  }

  /* quantize */
  output &= ~mask;

  /* error feedback */
  dither->error[0] = sample - output;

  /* scale */
  return output >> scalebits;
}


//madplayer 에서 뽑아온 소스
unsigned int audio_pcm_s16le(void * arg,unsigned char *data, unsigned int nsamples,
			     mad_fixed_t const *left, mad_fixed_t const *right, struct audio_stats *stats)
{
	CMP3Play * pMp3Play;
	pMp3Play =(CMP3Play *) arg;

	unsigned int len;
	register signed int sample0, sample1;

	len = nsamples;

      while (len--) {
		sample0 = audio_linear_dither(16, *left++,  &pMp3Play->left_dither,  stats);
		sample1 = audio_linear_dither(16, *right++, &pMp3Play->right_dither, stats);

		data[0] = sample0 >> 0;
		data[1] = sample0 >> 8;
		data[2] = sample1 >> 0;
		data[3] = sample1 >> 8;

		data += 4;
      }
   
      return nsamples * 2 * 2;
}

//madplayer 에서 뽑아온 소스
static enum mad_flow
play_output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
	bool bReset = false;
	CMP3Play * pMp3Play;
	pMp3Play =(CMP3Play *) data;
	mad_fixed_t const *ch1, *ch2;
	unsigned int nchannels,nsamples;
	unsigned long len;
	unsigned int nSampleRate;

	ch1 = pcm->samples[0];
	ch2 = pcm->samples[1];
	nchannels = pcm->channels;
	nsamples   = pcm->length;

	len = audio_pcm_s16le( data,pMp3Play->pcmdata, nsamples,
		ch1, ch2, &pMp3Play->stats);

	nSampleRate = header->samplerate;
/*
	if((pMp3Play->ScheduledBufferCount>4) && ( pMp3Play->m_WaveOut.outFormat.nSamplesPerSec != (DWORD)((double)nSampleRate)*1.1) &&(!pMp3Play->bDelayed))
	{
		pMp3Play->m_WaveOut.WIN_Audio_Reset(((double)nSampleRate)*1.05);
		pMp3Play->bDelayed = true;
	}

	if((pMp3Play->ScheduledBufferCount<1) && (pMp3Play->bDelayed))
	{
		pMp3Play->m_WaveOut.WIN_Audio_Reset(((double)nSampleRate*1.014));
		pMp3Play->bDelayed = false;
	}
	*/
	pMp3Play->m_WaveOut.WIN_Play_Samples(pMp3Play->pcmdata,len);

	return (MAD_FLOW_CONTINUE);
}
void MP3_DecThread(LPVOID lParam)
{
	CMP3Play * pThis;
	pThis = (CMP3Play *)lParam;
	pThis->Run();
}

CMP3Play::CMP3Play(void)
{
	
	bTest = false;
	bDelayed = false;
	lLastTimeStemp = 0;
	lLastPlayTime = timeGetTime() - 1000;
	memset(UserName,0x00,sizeof(UserName));

	InitializeCriticalSection ( &cs );

}

CMP3Play::~CMP3Play(void)
{	

	std::list<char*>::iterator j;
	j =  m_buffers.begin();
	while(j != m_buffers.end())
	{
	     delete [] *j;
		 j++;
	}
	m_buffers.clear();
	DeleteCriticalSection ( &cs );
	
}


void CMP3Play::Start(unsigned int SampleRate,unsigned int Bit,unsigned int nChanel)
{
	bDone = false;
	ScheduledBufferCount = 0;
	this->SampleRate = SampleRate;
	
	if (m_WaveOut.Set_WIN_Params ( ((double)SampleRate)*1.0115, Bit, nChanel))	{
	}

	//data = new unsigned char [MPEG_BUFSZ];

	mad_decoder_init(&mdec, this, play_input,
	    NULL, NULL, play_output, NULL, NULL);
	

	DWORD dwThreadId;
	CloseHandle(BEGINTHREADEX(NULL,0,MP3_DecThread,this,0,&dwThreadId));

}
void CMP3Play::AddBuffer(char * pData,int len)
{

	if(bDone) return;

	EnterCriticalSection ( &cs );

		short sSize;
		char * bufData = new char[len+2];

		if (bufData != 0)
		{
			m_buffers.push_back(bufData);
		}

		sSize = len;
		memcpy(bufData,&sSize,2);
		memcpy(bufData+2,pData,len);
		ScheduledBufferCount++;

	LeaveCriticalSection ( &cs );
}

void CMP3Play::Run ()
{
	short sSize;
	int size;
	int ret;
	bool	bReady;

	bReady = false;
	while(!bDone)
	{

		if(!bReady)
		{
			if(ScheduledBufferCount >1)
			{
				bReady = true;			
			}
		
		} else {
			if(ScheduledBufferCount >0 )
			{
				mad_decoder_run(&mdec, MAD_DECODER_MODE_SYNC);
			} else {
				bReady = false;
			}
		}
		Sleep(1);
	}
}

void CMP3Play::Stop()
{
	if(!bDone)
	{
		bDone = true;
		
		m_WaveOut.WIN_Audio_close();
	}

	
}
