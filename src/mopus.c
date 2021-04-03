#include "mopus.h"
#include "opus/opus.h"
#include "opus/opus_defines.h"
#include "opus/opus_types.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/* 说明 */
/* 1. 目前只适配了16K和8K采样，要支持其他需要更改海思芯片`stAioAttr.u32PtNumPerFrm`这个参数，因为默认只支持这个参数是320. */
/* 2. `opus_encoder_ctl(codec->encoder, OPUS_SET_BITRATE(OPUS_AUTO));`设置的是自动，可以考虑将该值写死，比如`opus_encoder_ctl(codec->encoder, OPUS_SET_BITRATE(16000));` */


#define MAX_MOPUS_PTIME (120) //OPUS支持的包间隔从20ms到120ms, 视频会议对实时性要求比较高, 所以我们采用的20ms,
#define MOPUS_PTIME (20)
#define SIGNAL_SAMPLE_SIZE  2 // 2 bytes per sample

/* Define codec specific settings */
#define MAX_BYTES_PER_MS	30 //25  // Equals peak bitrate of 200 kbps
#define MAX_INPUT_FRAMES	5   // The maximum amount of Opus frames in a packet we are using

/*inspired by the websits: */
/* 1. https://opus-codec.org/examples */
/* 2. https://blog.csdn.net/wirelessdisplay/article/details/77801825 */




int  ptime = MOPUS_PTIME;

struct mopus_codec * mopus_init(int rate,int chns)
{
    struct mopus_codec *codec = NULL;
    int err;
    if(16000 != rate && 8000 != rate){
        fprintf(stderr, "mopus not support this[%d] rate, only support 16k and 8k !\n",rate);
        return NULL;
    }
    codec = calloc(sizeof(struct mopus_codec), 1);
    if(NULL == codec){
        printf("[ERR]%s:L%d@%s,calloc failed !\n",__FUNCTION__,__LINE__,__FILE__);
        return NULL;
    }

    codec->encoder = opus_encoder_create(rate,chns,OPUS_APPLICATION_VOIP,&err);
    if( OPUS_OK != err || NULL == codec->encoder){
        printf("[ERR]%s:L%d@%s,opus_encoder_create failed !\n",__FUNCTION__,__LINE__,__FILE__);
        goto err_exit;
    }


    codec->samplerate = rate;
    codec->channels = chns;



    opus_encoder_ctl(codec->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

    opus_encoder_ctl(codec->encoder, OPUS_SET_BITRATE(OPUS_AUTO));     //使用AUTO主要是应为视频会议过程中,大部分场景是不说话的,减少带宽占用

    opus_encoder_ctl(codec->encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));

    opus_encoder_ctl(codec->encoder, OPUS_SET_VBR(0));//0:CBR, 1:VBR

    //opus_encoder_ctl(codec->encoder, OPUS_SET_VBR_CONSTRAINT(0));//0:Unconstrained VBR., 1:Constrained VBR.

    opus_encoder_ctl(codec->encoder, OPUS_SET_COMPLEXITY(1));//range:0~10 /* set complexity to 0 for single processor arm devices */

    opus_encoder_ctl(codec->encoder, OPUS_SET_FORCE_CHANNELS(chns)); //1:Forced mono, 2:Forced stereo

    opus_encoder_ctl(codec->encoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));

    opus_encoder_ctl(codec->encoder, OPUS_SET_INBAND_FEC(1));//0:Disable, 1:Enable
    opus_encoder_ctl(codec->encoder, OPUS_SET_DTX(0));



    if(codec->samplerate == 8000){
        ptime = 40;
        opus_encoder_ctl(codec->encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_40_MS));
    }else{
        opus_encoder_ctl(codec->encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS));
    }
    err = opus_encoder_ctl(codec->encoder, OPUS_SET_PACKET_LOSS_PERC(10));
    if (err != OPUS_OK) {
        printf("[ERR]%s:L%d@%s,Could not set default loss percentage to opus encoder: %s",__FUNCTION__,__LINE__,__FILE__, opus_strerror(err));
    }


    codec->decoder = opus_decoder_create(rate, chns, &err);
    if( OPUS_OK != err || NULL == codec->decoder){
        printf("[ERR]%s:L%d@%s,opus_encoder_create failed !\n",__FUNCTION__,__LINE__,__FILE__);
        goto err_exit;
    }


    return codec;

err_exit:

    free(codec);
    return NULL;

}





void mopus_destroy(struct mopus_codec *codec)
{
    if(!codec)
        return;

    opus_decoder_destroy(codec->decoder);
    opus_encoder_destroy(codec->encoder);
    free(codec);

}

int mopus_encode(struct mopus_codec *codec,unsigned char *out_buf, unsigned char *in_buf, unsigned int in_size)
{
    int out_size = 0;
    int packet_size,pcm_buffer_size;
    int max_frame_byte_size;
    int frame_count = 0, frame_size = 0;
    opus_int32 total_length = 0;

    if(!codec || !out_buf || !in_buf ){
        printf("[ERR]%s:L%d@%s, Params error!\n",__FUNCTION__,__LINE__,__FILE__);
        return -1;
    }

    //ptime = MOPUS_PTIME;
    packet_size = codec->samplerate * ptime / 1000; /*in samples*/

    switch (ptime) {
    case 10:
        frame_size = codec->samplerate * 10 / 1000;
        frame_count = 1;
        break;
    case 20:
        frame_size = codec->samplerate * 20 / 1000;
        frame_count = 1;
        break;
    case 40:
        frame_size = codec->samplerate * 40 / 1000;
        frame_count = 1;
        break;
    case 60:
        frame_size = codec->samplerate * 60 / 1000;
        frame_count = 1;
        break;
    case 80:
        frame_size = codec->samplerate * 40 / 1000;
        frame_count = 2;
        break;
    case 100:
        frame_size = codec->samplerate * 20 / 1000;
        frame_count = 5;
        break;
    case 120:
        frame_size = codec->samplerate * 60 / 1000;
        frame_count = 2;
        break;
    default:
        frame_size = codec->samplerate * 20 / 1000;
        frame_count = 1;
	}

	max_frame_byte_size = 48000 * 2 ;//MOPUS_PTIME * codec->samplerate / 1000;//MAX_BYTES_PER_MS * ptime/frame_count;


	pcm_buffer_size = codec->channels * frame_size * SIGNAL_SAMPLE_SIZE;
    //opus_encode(OpusEncoder *st, const opus_int16 *pcm, int frame_size, unsigned char *data, opus_int32 max_data_bytes)
    out_size = opus_encode(codec->encoder, (const opus_int16*)in_buf, frame_size, out_buf, max_frame_byte_size);

    if(out_size < 0)
        printf("[ERR]%s:L%d@%s, opus encoder error: %s\n",__FUNCTION__,__LINE__,__FILE__, opus_strerror(out_size));

    return out_size;
}



int mopus_decode(struct mopus_codec *codec, unsigned char *out_buf, unsigned char *in_buf, unsigned int in_size)
{
    int out_size = 0;
    int max_frame_byte_size = 0;
    if(!codec || !out_buf){
        printf("[ERR]%s:L%d@%s,[%p,%p] Params error!\n",__FUNCTION__,__LINE__,__FILE__,codec,out_buf);
        return -1;
    }

    max_frame_byte_size = 5760;///* 5760 is the maximum number of sample in a packet (120ms at 48KHz) */
    if (NULL == in_buf || 0 == in_size){
        //opus_decode(OpusDecoder *st, const unsigned char *data, opus_int32 len, opus_int16 *pcm, int frame_size, int decode_fec)
        out_size = opus_decode(codec->decoder,NULL,0,(opus_int16 *)out_buf,max_frame_byte_size,1);
        if(out_size < 0)
            printf("[ERR]%s:L%d@%s, opus decoder error: %s\n",__FUNCTION__,__LINE__,__FILE__, opus_strerror(out_size));
    } else {
        out_size = opus_decode(codec->decoder,(const unsigned char*)in_buf,(opus_int32)in_size,(opus_int16 *)out_buf,max_frame_byte_size,0);
        if(out_size < 0)
            printf("[ERR]%s:L%d@%s, opus decoder error: %s\n",__FUNCTION__,__LINE__,__FILE__, opus_strerror(out_size));
    }

    return out_size > 0 ? 2 * out_size : out_size;
}
