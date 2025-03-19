#pragma once
#include"stdint.h"
#include <emmintrin.h>
#include"combaseapi.h"
#include"mmdeviceapi.h"
#include"audioclient.h"
#include"winnt.h"
extern "C"
{

#include<libavutil/hmac.h>
#include<libavutil/error.h>
}
#include <iostream>
void bytestream_put_byte(char** buf, char val);
void bytestream_put_le32(uint32_t** buf, uint32_t val);
void bytestream_put_le16(uint16_t** buf, uint16_t val);
HRESULT PlayAudioStream();
void playSound(char* pcmdata);
