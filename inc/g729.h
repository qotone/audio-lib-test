#ifndef __G729_H__
#define __G729_H__

#include <stdlib.h>
#include <stdint.h>

#include "bcg729/encoder.h"
#include "bcg729/decoder.h"


struct G729_codec {
    bcg729DecoderChannelContextStruct *decoder;
    bcg729EncoderChannelContextStruct *encoder;
};


/*
 * codec init
 * @return G729 codec point.
 */
struct G729_codec * g729_init();


/*
 * codec destroy
 * @param codec: G729 codec context.
 */
void g729_destroy(struct G729_codec *codec);

/*
 * pcm16 to g729
 * @param codec: G729 codec context.
 * @param out_buf: the g729 data .
 * @param in_buf: the pcm16 data .
 * @param in_size: size of `in_buf`.
 * @return size of `out_buf` data.
 */
int g729_encode(struct G729_codec *codec,unsigned char* out_buf, unsigned char* in_buf, unsigned int in_size);


/*
 * g729 to pcm16
 * @param codec: G729 codec context.
 * @param out_buf: the pcm16 data .
 * @param in_buf: the g729 data .
 * @param in_size: size of `in_buf`.
 * @return size of `out_buf` data.
 */
int g729_decode(struct G729_codec *codec,unsigned char* out_buf, unsigned char* in_buf, unsigned int in_size);


#endif /* __G729_H__ */
