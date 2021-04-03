#ifndef __M_OPUS_H__
#define __M_OPUS_H__


#include <stdlib.h>
#include <stdint.h>

#include "opus/opus.h"

struct mopus_codec {
    OpusDecoder *decoder;
    OpusEncoder *encoder;
    int samplerate;
    int channels;
};


struct mopus_codec * mopus_init();



void mopus_destroy(struct mopus_codec *codec);


int mopus_encode(struct mopus_codec *codec,unsigned char *out_buf, unsigned char *in_buf, unsigned int in_size);



int mopus_decode(struct mopus_codec *codec, unsigned char *out_buf, unsigned char *in_buf, unsigned int in_size);


#endif // __M_OPUS_H__
