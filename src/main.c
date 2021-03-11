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

#include "ht_audio.h"
#include "ht_hicomm.h"
#include "ht_sys.h"

#include "nanomsg/pipeline.h"
#include "nanomsg/nn.h"
#include "utils_log.h"

#include "g729.h"


#define NN_URL "ipc:///tmp/pipeline.ipc"

#define CODEC_G729   0   //1 -- decode G729 , 0 -- decode PCM .

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
    int sock;
    char file_name[64];
} SAMPLE_AENC_S;


SAMPLE_AENC_S my_aenc;
struct G729_codec *g729_codec = NULL;

FILE *pcm_file = NULL;



void * audio_aencProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;
    int len = 0;

    char audio_file[128];

    unsigned char buffer[320];

    prctl(PR_SET_NAME, "hi_AencProc", 0, 0, 0);

    FD_ZERO(&read_fds);
    AencFd = HI_MPI_AENC_GetFd(pstAencCtl->AeChn);
    FD_SET(AencFd, &read_fds);


    unsigned int size;
    unsigned char out_buffer[320];
    unsigned char *in_buf,*out_buf;


    while (pstAencCtl->bStart)
    {
        unsigned int out_size = 0;
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(AencFd,&read_fds);
        s32Ret = select(AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0){
            break;
        }else if (0 == s32Ret){
            LOGE_print("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = HI_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret ){
                LOGE_print("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }

#if 0 // dirctly to adec, no use at this moment.
            /* send stream to decoder and play for testing */
            if (HI_TRUE == pstAencCtl->bSendAdChn){
                s32Ret = HI_MPI_ADEC_SendStream(pstAencCtl->AdChn, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret ){
                    printf("%s: HI_MPI_ADEC_SendStream(%d), failed with %#x!\n",\
                           __FUNCTION__, pstAencCtl->AdChn, s32Ret);
                    pstAencCtl->bStart = HI_FALSE;
                    return NULL;
                }
            }
#endif
            /* save audio stream to file */


            if(pcm_file == NULL){

                snprintf(audio_file,strlen(pstAencCtl->file_name) + 5, "%s.pcm",pstAencCtl->file_name);
                pcm_file = fopen(audio_file,"w+");
                printf("[INFO] %s:L%d,pcm file :%s\n",__FUNCTION__,__LINE__,audio_file );

            }else {
                fwrite(stStream.pStream,1,stStream.u32Len, pcm_file);
            }

            out_size = g729_encode(g729_codec, out_buffer, stStream.pStream, stStream.u32Len);
            if(out_size > 0){
                if(pstAencCtl->pfd == NULL){
                    snprintf(audio_file,strlen(pstAencCtl->file_name) + 6, "%s.g729",pstAencCtl->file_name);
                    pstAencCtl->pfd = fopen(audio_file,"w+");
                    printf("[INFO] %s:L%d,g729 file :%s\n",__FUNCTION__,__LINE__,audio_file );
                }
                if(pstAencCtl->pfd)
                    fwrite(out_buffer,1,out_size, pstAencCtl->pfd);
            }
            /* LOGI_print("write %d bytes\n",out_size); */
            /* fwrite(stStream.pStream+4,1,stStream.u32Len - 4, pstAencCtl->pfd); */
            /* LOGI_print("write %d bytes \n",stStream.u32Len -4); */
#ifndef CODEC_G729
            int bytes = nn_send(pstAencCtl->sock, (void *)stStream.pStream, (size_t) stStream.u32Len, 0);
#else
            int bytes = nn_send(pstAencCtl->sock, (void *)out_buffer, (size_t) out_size, 0);
#endif /* !CODEC_G729 */




            /* finally you must release the stream */
            s32Ret = HI_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
            if (HI_SUCCESS != s32Ret ){
                printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
        }
    }

    fclose(pstAencCtl->pfd);
    if(pcm_file != NULL)
        fclose(pcm_file);

    printf("fclose ...\n");

    pstAencCtl->bStart = HI_FALSE;
    return NULL;


}


static void sig_handler(int signum)
{
	printf("signal %d(%s) received\n", signum, strsignal(signum));

	my_aenc.bStart = 0;
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

    AUDIO_DEV aiDev = SAMPLE_AUDIO_TLV320_AI_DEV;
    AI_CHN aiChn = 0;
    AENC_CHN aeChn = 0;

    AIO_ATTR_S stAioAttr;
    char audio_file[128];



    g729_codec = g729_init();
    if(g729_codec == NULL)
        return -1;


    /* Setup signal handlers */
	signal(SIGINT, &sig_handler);
	signal(SIGTERM, &sig_handler);
	signal(SIGPIPE, SIG_IGN);


    /* 初始化海思mpp系统 */
    s32Ret = ht_sys_init();

    s32Ret = HI_MPI_AENC_AacInit();
    if(HI_SUCCESS != s32Ret)
    {
                printf("%s: aac aenc init failed with %d!\n", __FUNCTION__, s32Ret);
                return HI_FAILURE;
        }
    s32Ret = HI_MPI_ADEC_AacInit();
    if(HI_SUCCESS != s32Ret)
    {
        printf("%s: aac adec init failed with %d!\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }

    ht_audio_reset();

    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth  = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE; //for aio,for sil9233 in

    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO; //AUDIO_SOUND_MODE_STEREO;
    stAioAttr.u32EXFlag = 1;										//À©Õ¹³É16 Î»£¬8bitµ½16bit À©Õ¹±êÖ¾Ö»¶ÔAI²ÉÑù¾«¶ÈÎª8bit Ê±ÓÐÐ§
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;//1024;//SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 2;	// 2									//stereo mode must be 2
    stAioAttr.u32ClkChnCnt   = 2;//2
    stAioAttr.u32ClkSel = 0; //for sil9233 = 0;

    /*设置TLV320音频解码芯片参数*/
    Tlv320_Cfg(stAioAttr.enWorkmode,stAioAttr.enSamplerate);


    pid_t pid = fork();

    if(pid == 0) { // child pid to decode and play the audio data.
        ADEC_CHN    adChn = 0;
        AO_CHN aoChn = 0;
        AUDIO_DEV aoDev = SAMPLE_AUDIO_TLV320_AO_DEV;
        unsigned char out_buf[640] = {0};
        unsigned int out_size = 0;
        printf("Chirld PID to recv and decode data.\n");
        s32Ret = ht_ao_init(aoDev,aoChn,&stAioAttr);
        s32Ret = ht_adec_start(adChn,PT_LPCM);

        s32Ret = ht_sys_aoBindAdec(aoDev,aoChn,adChn);

        int sock = nn_socket(AF_SP, NN_PULL);
        assert(sock >= 0);
        assert(nn_bind(sock, NN_URL) >= 0);
        while (1) {
            unsigned char *buf = NULL;
            int bytes = nn_recv(sock, &buf, NN_MSG, 0);
            assert(bytes >= 0);
            if(bytes == 2 && buf[0]== 'q'){
                break;
            }else {
                AUDIO_STREAM_S stStream;

#if defined (CODEC_G729)
                out_size = g729_decode(g729_codec, out_buf, buf, bytes);
                /* printf("[INFO] %s:L%d@%s,out_size = %d.\n",__FUNCTION__,__LINE__,__FILE__,out_size); */
                stStream.pStream = out_buf;//TODO:
                stStream.u32Len = out_size;
#else //CODEC PCM
                stStream.pStream = buf;//TODO:
                stStream.u32Len = bytes;
#endif /*CODEC_G729*/
                s32Ret = HI_MPI_ADEC_SendStream(adChn, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret ){
                    printf("[ERR]%s:L%d, HI_MPI_ADEC_SendStream(%d), failed with %#x!\n", \
                           __FUNCTION__,__LINE__, adChn, s32Ret);
                }
            }
            nn_freemsg(buf);
        }

        s32Ret = ht_sys_aoUnBindAdec(aoDev, aoChn, adChn);
        s32Ret = ht_ao_uninit(aoDev, aoChn);
        nn_shutdown(sock, 0);
        printf("Child exit!\n");
        exit(0);
    }else {
        printf("Parent PID to encoder and send to adec.\n");
    }

    s32Ret = ht_ai_init(aiDev,aiChn,&stAioAttr,AUDIO_SAMPLE_RATE_BUTT);


    s32Ret = ht_aenc_start(aeChn,&stAioAttr,/* PT_G711U */PT_LPCM);

    s32Ret = ht_sys_aencBindAi(aeChn,aiDev,aiChn);




    memcpy(my_aenc.file_name, strchr(argv[0], '/') + 1, strlen(argv[0]) - (strchr(argv[0], '/') - argv[0] - 1));
    //printf("%s:%d\n", my_aenc.file_name,strlen(argv[0]));
    my_aenc.AeChn = aeChn;
    my_aenc.bStart = HI_TRUE;
    my_aenc.pfd = NULL;

    int push_sock = nn_socket(AF_SP, NN_PUSH);
    assert(push_sock >= 0);
    assert(nn_connect(push_sock, NN_URL) >= 0);

    my_aenc.sock = push_sock;


    pthread_create(&my_aenc.stAencPid,0,audio_aencProc,&my_aenc);



    printf("`Ctrl + C` to quit.\n");

    while(my_aenc.bStart == HI_TRUE){
        usleep(500);
    }


    int bytes = nn_send(push_sock, "q", 2, 0);
    sleep(1);
    if(g729_codec != NULL)
        g729_destroy(g729_codec);
    ht_ai_uninit(aiDev, aiChn);

    ht_sys_unInit();
    nn_shutdown(push_sock, 0);

    return 0;
}
