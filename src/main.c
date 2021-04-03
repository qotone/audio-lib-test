#include "hi_comm_aio.h"
#include "hi_comm_video.h"
#include "hi_common.h"
#include "hi_type.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#if 0

#include "ht_audio.h"
#include "ht_hicomm.h"
#include "ht_sys.h"

#else

#include "hink_comm.h"
#include "hink_sys.h"
#include "hink_adec.h"
#include "hink_aenc.h"
#include "hink_aio.h"

#endif

#include "nanomsg/pipeline.h"
#include "nanomsg/nn.h"
#include "utils_log.h"

#include "g729.h"
#include "mopus.h"


#define NN_URL "ipc:///tmp/pipeline.ipc"

//#define CODEC_G729   0   //1 -- decode G729 , 0 -- decode PCM .
#define CODEC_TYPE_G729 1
#define CODEC_TYPE_OPUS 2

#define CODEC_TYPE    CODEC_TYPE_OPUS
#define SEND_ENCODE_DATA   1

struct G729_codec *g729_codec = NULL;

struct mopus_codec *opus_codec = NULL;

struct aenc_ctx{
    char file[64];
    int push_socket;
    FILE *pPFile;
    FILE *pGFile;
};

static int audio_tick = 0;

struct aenc_ctx ctx;
int aenc_callback(AENC_CHN aeChn,AUDIO_STREAM_S *pstrm,void *parg)
{
    int out_size;
    unsigned char out_buffer[640];

    struct aenc_ctx *pctx =(struct aenc_ctx *)parg;

    if(pctx->pPFile == NULL){
        char audio_file[64];

        snprintf(audio_file,strlen(pctx->file) + 5, "%s.pcm",pctx->file);
        pctx->pPFile = fopen(audio_file,"w+");
        printf("[INFO] %s:L%d,pcm file :%s\n",__FUNCTION__,__LINE__,audio_file );

    }

    if(pctx->pPFile)
        fwrite(pstrm->pStream,1,pstrm->u32Len, pctx->pPFile);

#if CODEC_TYPE == CODEC_TYPE_G729
    out_size = g729_encode(g729_codec, out_buffer, pstrm->pStream, pstrm->u32Len);

    if(pctx->pGFile == NULL){
        char audio_file[64];

        snprintf(audio_file,strlen(pctx->file) + 6, "%s.g729",pctx->file);
        pctx->pGFile = fopen(audio_file,"w+");
        printf("[INFO] %s:L%d,g729 file :%s\n",__FUNCTION__,__LINE__,audio_file );
    }

    if(pctx->pGFile)
        fwrite(out_buffer,1,out_size, pctx->pGFile);

#elif CODEC_TYPE == CODEC_TYPE_OPUS
    if((audio_tick++ % 100 ) == 0)
        printf("[INFO] %s:L%d, before encode %lu bytes.\n",__FUNCTION__,__LINE__,pstrm->u32Len );
    out_size = mopus_encode(opus_codec, out_buffer, pstrm->pStream, pstrm->u32Len);

    if((audio_tick % 100 ) == 0)
        printf("[INFO] %s:L%d,after encode %lu bytes.\n",__FUNCTION__,__LINE__,out_size );

    if(pctx->pGFile == NULL){
        char audio_file[64];

        snprintf(audio_file,strlen(pctx->file) + 6, "%s.opus",pctx->file);
        pctx->pGFile = fopen(audio_file,"w+");
        printf("[INFO] %s:L%d,opus file :%s\n",__FUNCTION__,__LINE__,audio_file );
    }

    if(out_size > 0 && pctx->pGFile)
        fwrite(out_buffer,1,out_size, pctx->pGFile);
#else
    printf("%s\n", "ERRROR");

#endif



#ifndef SEND_ENCODE_DATA
    int bytes = nn_send(pctx->push_socket, (void *)pstrm->pStream, (size_t) pstrm->u32Len, 0);
#else
    int bytes = nn_send(pctx->push_socket, (void *)out_buffer, (size_t) out_size, 0);
#endif /* !CODEC_G729 */

    return 0;
}

volatile int bStart = 1;

static void sig_handler(int signum)
{
	printf("signal %d(%s) received\n", signum, strsignal(signum));

    if(bStart)
        bStart = 0;
    else
        exit(-1);
}

/*
 * 这个工程用于验证bcg729库的编解码功能
 * `g729.h` 和 `g729.c` 是bcg729库的封装。
 * 工程实现：
 * 父进程，AI --> AENC -- PCM --- 经过bcg729库编码成G729 -->将G729数据发送给子进程.
 * 子进程，获取父进程G729数据 --> 经过bcg729库解码成 PCM ---> ADEC ---> AO.
 * 说明：
 * 进程间通讯用了`nanomsg`这个第三方库。
 * AI，AO， 采样率为8K,mono 模式。
 * AENC,ADEC 设置格式为 `PT_LPCM`.
 * 文件上面宏`CODEC_G729`用于配置开启G729解码输出还是直接PCM解码输出。
 * 程序会生成两个和应用程序同名的音频文件，名称为：bcg729_test.g729、bcg729_test.pcm,这两个文件是同源文件，用于对比G729和PCM文件。
 */


int main(int argc, char *argv[])
{
    char ch;
    HI_S32 s32Ret = HI_FAILURE;

    AUDIO_DEV aiDev = HINK_AUDIO_TLV320_AI_DEV;
    AI_CHN aiChn = 0;
    AENC_CHN aeChn = 0;

    AIO_ATTR_S stAioAttr;
    char audio_file[128];
    AUDIO_SAMPLE_RATE_E sample_rate = AUDIO_SAMPLE_RATE_8000;


#if CODEC_TYPE == CODEC_TYPE_G729
    g729_codec = g729_init();
    if(g729_codec == NULL)
        return -1;
#elif   CODEC_TYPE == CODEC_TYPE_OPUS
    printf("[INFO]\tmopus_init\n");
    opus_codec = mopus_init(16000,1);
    if(opus_codec == NULL)
        return -1;


        sample_rate  = AUDIO_SAMPLE_RATE_16000;
#else
    printf("[ERROR]\tinit...\n");
#endif



    /* 初始化海思mpp系统 */
    HI_U32 u32BlkSize = hink_sys_calcPicVbBlkSize(VIDEO_ENCODING_MODE_AUTO, PIC_HD1080, HINK_PIXEL_FORMAT, HINK_SYS_ALIGN_WIDTH, COMPRESS_MODE_SEG);
    s32Ret = hink_sys_init(u32BlkSize, 18);


    hink_aenc_reset();
    hink_adec_reset();
    hink_ai_reset();
    hink_ao_reset();

    stAioAttr.enSamplerate = sample_rate;
    stAioAttr.enBitwidth  = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE; //for aio,for sil9233 in

    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO; //AUDIO_SOUND_MODE_STEREO;
    stAioAttr.u32EXFlag = 1;										//À©Õ¹³É16 Î»£¬8bitµ½16bit À©Õ¹±êÖ¾Ö»¶ÔAI²ÉÑù¾«¶ÈÎª8bit Ê±ÓÐÐ§
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = HINK_AUDIO_PTNUMPERFRM;//1024;//SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 1;	// 2									//stereo mode must be 2
    stAioAttr.u32ClkChnCnt   = 1;//2
    stAioAttr.u32ClkSel = 0; //for sil9233 = 0;

/*设置TLV320音频解码芯片参数*/
    hink_tlv320_cfg(stAioAttr.enWorkmode,stAioAttr.enSamplerate);


    pid_t pid = fork();

    if(pid == 0) { // child pid to decode and play the audio data.
        ADEC_CHN    adChn = 0;
        AO_CHN aoChn = 0;
        AUDIO_DEV aoDev = HINK_AUDIO_TLV320_AO_DEV;
        unsigned char out_buf[1024*4] = {0};
        unsigned int out_size = 0;
        MPP_CHN_S stSrcChn,stDestChn;
        printf("Chirld PID to recv and decode data.\n");
        //stAioAttr.u32PtNumPerFrm = 960;
        s32Ret = hink_ao_init(aoDev,aoChn,&stAioAttr);
        s32Ret = hink_adec_start(adChn,PT_LPCM);

        HINK_MPP_CHN_INIT(stSrcChn, HI_ID_ADEC, 0, adChn);
        HINK_MPP_CHN_INIT(stDestChn, HI_ID_AO, aoDev, aoChn);

        s32Ret = hink_sys_bind(&stSrcChn, &stDestChn);//ht_sys_aoBindAdec(aoDev,aoChn,adChn);

        int sock = nn_socket(AF_SP, NN_PULL);
        assert(sock >= 0);
        assert(nn_bind(sock, NN_URL) >= 0);
        while (bStart) {
            unsigned char *buf = NULL;
            int bytes = nn_recv(sock, &buf, NN_MSG, 0);


            if((bytes == 2 && buf[0]== 'q')){
                printf("[DBG]  child recv q L%d\n",__LINE__);
                break;
            }

            else {
                AUDIO_STREAM_S stStream;

#if defined (SEND_ENCODE_DATA)


#if CODEC_TYPE == CODEC_TYPE_G729
                out_size = g729_decode(g729_codec, out_buf, buf, bytes);
                /* printf("[INFO] %s:L%d@%s,out_size = %d.\n",__FUNCTION__,__LINE__,__FILE__,out_size); */
                stStream.pStream = out_buf;//TODO:
                stStream.u32Len = out_size;
#elif CODEC_TYPE == CODEC_TYPE_OPUS
                out_size = mopus_decode(opus_codec, out_buf, buf, bytes);//g729_decode(g729_codec, out_buf, buf, bytes);
                if((audio_tick++ % 100) == 0)
                    printf("[INFO] %s:L%d@%s,decode out_size = %d.\n",__FUNCTION__,__LINE__,__FILE__,out_size);
                stStream.pStream = out_buf;//TODO:
                stStream.u32Len = out_size ;

#endif

#else //!SEND_ENCODE_DATA
                stStream.pStream = buf;//TODO:
                stStream.u32Len = bytes;
#endif /*SEND_ENCODE_DATA*/
                s32Ret = HI_MPI_ADEC_SendStream(adChn, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret ){
                    printf("[ERR]%s:L%d, HI_MPI_ADEC_SendStream(%d), failed with %#x!\n", \
                           __FUNCTION__,__LINE__, adChn, s32Ret);
                    nn_freemsg(buf);
                    continue;
                }
            }

            nn_freemsg(buf);
        }

        /* s32Ret = hink_sys_unBind(&stSrcChn, &stDestChn); *///ht_sys_aoUnBindAdec(aoDev, aoChn, adChn);

        nn_shutdown(sock, 0);
        printf("*************************Child exit!\n");
        exit(0);
    }else {
        printf("Parent PID to encoder and send to adec.\n");
        /* Setup signal handlers */
        signal(SIGINT, &sig_handler);
        signal(SIGTERM, &sig_handler);
        signal(SIGPIPE, SIG_IGN);

    }

    s32Ret = hink_ai_init(aiDev,aiChn,&stAioAttr,AUDIO_SAMPLE_RATE_BUTT);




    hink_aenc_t stAenc;
    stAenc.bufSize = 30;
    stAenc.payload = PT_LPCM;
    stAenc.aeChn = aeChn;
    stAenc.pstAioAttr = &stAioAttr;

    s32Ret = hink_aenc_start(&stAenc);

    MPP_CHN_S stAeChn,stAiChn;
    HINK_MPP_CHN_INIT(stAeChn, HI_ID_AENC, 0, aeChn);
    HINK_MPP_CHN_INIT(stAiChn, HI_ID_AI, aiDev, aiChn);

    s32Ret = hink_sys_bind(&stAiChn, &stAeChn);//ht_sys_aencBindAi(aeChn,aiDev,aiChn);

    int push_sock = nn_socket(AF_SP, NN_PUSH);
    assert(push_sock >= 0);
    assert(nn_connect(push_sock, NN_URL) >= 0);




    hink_aenc_recv_t *pst = calloc(sizeof(hink_aenc_recv_t), 1);

    if(pst == NULL){
        printf("%s\n", "MEMO ERROR!");
        return -1;
    }
    memcpy(ctx.file, strchr(argv[0], '/') + 1, strlen(argv[0]) - (strchr(argv[0], '/') - argv[0] - 1));
    ctx.push_socket = push_sock;
    pst->s32Cnt = 1;
    pst->aeChn[0] = 0;
    pst->uargs = &ctx;
    pst->cb = aenc_callback;
    hink_aenc_recv(pst);

    printf("`Ctrl + C` to quit.\n");

    while(bStart){

        sleep(1);
        /* int bytes = nn_send(push_sock, "q", 2, 0); */
    }


    printf("[DBG] L%d\n",__LINE__);
    int bytes = nn_send(push_sock, "q", 2, 0);
    usleep(500);
    printf("[DBG] L%d\n",__LINE__);
    hink_aenc_dest(pst);
    hink_sys_unBind(&stAiChn, &stAeChn);
    printf("[DBG] L%d\n",__LINE__);
    if(ctx.pPFile){
        fclose(ctx.pPFile);
    }
    if(ctx.pGFile){
        fclose(ctx.pGFile);
    }
    printf("[DBG] L%d\n",__LINE__);
    free(pst);
    printf("[DBG] L%d\n",__LINE__);
    if(g729_codec != NULL)
        g729_destroy(g729_codec);

    if(opus_codec != NULL)
        mopus_destroy(opus_codec);

    printf("[DBG] L%d\n",__LINE__);
    hink_ai_uninit(aiDev, aiChn);
    printf("[DBG] L%d\n",__LINE__);
    hink_sys_unInit();
    printf("[DBG] L%d\n",__LINE__);
    nn_shutdown(push_sock, 0);
    printf("[DBG] L%d\n",__LINE__);
    return 0;
}
