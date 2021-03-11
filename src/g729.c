#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "g729.h"

#define G729_PAYLOAD_ID          18
#define G729_BYTES_PER_FRAME     10
#define G729_SAMPLES_PER_FRAME   10
#define PCM_BYTES_PER_FRAME      160

/*
 * codec init
 */
struct G729_codec * g729_init()
{
    struct G729_codec *codec = NULL;
    codec = calloc(sizeof(struct G729_codec), 1);
    if(NULL == codec){
        printf("[ERR]%s:L%d@%s,calloc failed !\n",__FUNCTION__,__LINE__,__FILE__);
        return NULL;
    }

    codec->encoder = initBcg729EncoderChannel(0);
    codec->decoder = initBcg729DecoderChannel();

    return codec;
}


/*
 * codec destroy
 */
void g729_destroy(struct G729_codec *codec)
{

    if(!codec)
        return;

    closeBcg729DecoderChannel(codec->decoder);
    closeBcg729EncoderChannel(codec->encoder);

    free(codec);
}

/*
 * pcm16 to g729
 */
int g729_encode(struct G729_codec *codec,unsigned char* out_buf, unsigned char* in_buf, unsigned int in_size)
{
    unsigned int out_size = 0;
    unsigned int size = in_size;
    if(!codec)
        return -1;

    if(size % PCM_BYTES_PER_FRAME != 0){
        printf("[ERR]%s:L%d@%s,number of blocks should be integral(block size = %u)\n",__FUNCTION__,__LINE__,__FILE__,PCM_BYTES_PER_FRAME );
        return -1;
    }

    while (size >= PCM_BYTES_PER_FRAME) {
        /* Encode a frame  */
        uint8_t olen;
        bcg729Encoder(codec->encoder, (int16_t *)in_buf, (uint8_t *)out_buf, &olen);


        size -= PCM_BYTES_PER_FRAME;
        in_buf += PCM_BYTES_PER_FRAME;

        out_buf += olen;
        out_size += olen;
    }

    return out_size;
}


/*
 * g729 to pcm16
 */
int g729_decode(struct G729_codec *codec,unsigned char* out_buf, unsigned char* in_buf, unsigned int in_size)
{
    unsigned int out_size = 0;
    unsigned int size = in_size;

    if(!codec)
        return -1;

    while(size > 0){
        if(size >= G729_BYTES_PER_FRAME){
            /* Decode a frame  */
            bcg729Decoder(codec->decoder, in_buf,
                          G729_BYTES_PER_FRAME,
                          0 /* no erasure */,
                          0 /* not SID */,
                          0 /* not RFC3389 */,
                          (int16_t *)out_buf);

            size -= G729_BYTES_PER_FRAME;
            in_buf += G729_BYTES_PER_FRAME;

            out_buf += PCM_BYTES_PER_FRAME;
            out_size += PCM_BYTES_PER_FRAME;
        }else if (size == 2 || size == 3 || size == 6) {
            /* Decode SID frame */
            bcg729Decoder(codec->decoder, in_buf,
                          size,
                          0 /* no erasure */,
                          1 /* SID frame */,
                          0 /* not RFC3389 */,
                          (int16_t *)out_buf);

            out_size += PCM_BYTES_PER_FRAME;
            break;
        }else {
            /* Unknown frame */
            printf("[ERR]%s:L%d@%s,Unknown frame (size=%d)\n", __FUNCTION__,__LINE__,__FILE__,size);
            break;
        }
    }

    return out_size;
}
