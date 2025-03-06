#include "Common.h"

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
#define DIVBY32768 0.000030517578125f
typedef int16_t Sint16;
void bytestream_put_byte(char** buf, char val) {
    char* ptr = *buf;
    *ptr = val;
    (*buf)++;
}
void bytestream_put_le32(uint32_t** buf, uint32_t val) {
    uint32_t* ptr = *buf;
    *ptr = val;
    (*buf)++;
}
void bytestream_put_le16(uint16_t** buf, uint16_t val) {
    uint16_t* ptr = *buf;
    *ptr = val;
    (*buf)++;
}
IAudioClient* pAudioClient = NULL;
IAudioRenderClient* pRenderClient = NULL;
HRESULT PlayAudioStream()
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	

	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	UINT32 numFramesPadding;
	BYTE* pData;
	HANDLE hRenderEvent;
	DWORD flags = 0;
	UINT32 iPaddingFrames = 0;
	UINT32 i = 0;
	UINT32 dwBytesFrames;
	CoInitializeEx(0, 0);
	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	hr = pEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &pDevice);
	hr = pDevice->Activate(
			IID_IAudioClient, CLSCTX_ALL,
			NULL, (void**)&pAudioClient);
	

		WAVEFORMATEX* pwfx;
	/*ZeroMemory(&pwfx, sizeof(WAVEFORMATEX));
	pwfx.wFormatTag = 1;
	pwfx.nChannels = 2;
	pwfx.nSamplesPerSec = 8000;
	pwfx.nAvgBytesPerSec = 32000;
	pwfx.nBlockAlign = 4;
	pwfx.wBitsPerSample = 16;
	pwfx.cbSize = 0;*/
	hr = pAudioClient->GetMixFormat(&pwfx);
		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			0, 0, pwfx, NULL);




		// Get the actual size of the allocated buffer.
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);


		hRenderEvent = CreateEvent(NULL, false, false, NULL);
	hr = pAudioClient->SetEventHandle(hRenderEvent);

	hr = pAudioClient->GetService(
		IID_IAudioRenderClient,
		(void**)&pRenderClient);



		numFramesAvailable = bufferFrameCount;

	hr = pAudioClient->Start();  // Start playing.


		// Each loop fills about half of the shared buffer.

		

	// Wait for last data in buffer to play before stopping.


	//hr = pAudioClient->Stop();  // Stop playing.
	//EXIT_ON_ERROR(hr)

	//	Exit:
	//CoTaskMemFree(&pwfx);
	//SAFE_RELEASE(pEnumerator)
	//	SAFE_RELEASE(pDevice)
	//	SAFE_RELEASE(pAudioClient)
	//	SAFE_RELEASE(pRenderClient)

		return hr;
}


static void SDL_Convert_S16_to_F32_SSE2(Sint16* src, float* dst)
{

	int i;

	//LOG_DEBUG_CONVERT("AUDIO_S16", "AUDIO_F32 (using SSE2)");

	/* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
	for (i = 8192 / sizeof(Sint16); i && (((size_t)(dst - 7)) & 15); --i, --src, --dst) {
		*dst = ((float)*src) * DIVBY32768;
	}

	src -= 7; dst -= 7;  /* adjust to read SSE blocks from the start. */
	//SDL_assert(!i || ((((size_t)dst) & 15) == 0));

	/* Make sure src is aligned too. */
	if ((((size_t)src) & 15) == 0) {
		/* Aligned! Do SSE blocks as long as we have 16 bytes available. */
		const __m128 divby32768 = _mm_set1_ps(DIVBY32768);
		while (i >= 8) {   /* 8 * 16-bit */
			const __m128i ints = _mm_load_si128((__m128i const*) src);  /* get 8 sint16 into an XMM register. */
			/* treat as int32, shift left to clear every other sint16, then back right with sign-extend. Now sint32. */
			const __m128i a = _mm_srai_epi32(_mm_slli_epi32(ints, 16), 16);
			/* right-shift-sign-extend gets us sint32 with the other set of values. */
			const __m128i b = _mm_srai_epi32(ints, 16);
			/* Interleave these back into the right order, convert to float, multiply, store. */
			_mm_store_ps(dst, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi32(a, b)), divby32768));
			_mm_store_ps(dst + 4, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi32(a, b)), divby32768));
			i -= 8; src -= 8; dst -= 8;
		}
	}

	src += 7; dst += 7;  /* adjust for any scalar finishing. */

	/* Finish off any leftovers with scalar operations. */
	while (i) {
		*dst = ((float)*src) * DIVBY32768;
		i--; src--; dst--;
	}


}
void playSound(char* pcmdata) {
	Sint16* src = ((Sint16*)(pcmdata + 8192)) - 1;
	float* dst = ((float*)(pcmdata + 8192 * 2)) - 1;
	
	SDL_Convert_S16_to_F32_SSE2(src, dst);
	BYTE* pData;
	for (int i = 0; i < 2; i++)
	{
		Sleep(15);
		pRenderClient->GetBuffer(480, &pData);
		memcpy(pData, pcmdata+(i%2)* 3840, 3840);
		pRenderClient->ReleaseBuffer(480, 0);
	}
}

