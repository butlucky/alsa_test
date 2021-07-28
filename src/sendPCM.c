
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "rtp.h"
#include "g711codec.h"

//网络参数,有些端口vlc是不支持的
#define RTP_IP      "192.168.3.32"
#define RTP_PORT    9832

//发送文件参数
#define SEND_CHN    1
#define SEND_FREQ   8000
//每包数据发出后的延时
#define SEND_DELAYUS   20000

//rtp包时间戳增量
//不同于AAC格式,PCMA/PCMU格式RTP包的时间戳用于记录数据量即160
#define SEND_TIMESTAMP 160 //320 bytes PCM --> 160 bytes PCMA/PCMU
unsigned char pcm[320] = { 0 };

static int  stero_to_mono(unsigned char *src,unsigned char *dst, int len )
{
    int i = 0,j = 0;

    for(i;i<len;i+=4,j+=2){
        pcm[j] = src[i];
        pcm[j+1] = src[i+1];
    }

    return len/2;
}
int main(int argc, char* argv[])
{
    int ret;
    char wsdp = 0;
    __time_t tick1, tick2;
    int pkgType = RTP_PAYLOAD_TYPE_PCMA;
    //各种句柄
    SocketStruct *ss;
    RtpPacket rtpPacket;
    int fd;
    int seekStart = 0;
    char audiofile[64] = {"./8000.wav"};
    int chn = SEND_CHN ;
    unsigned char *srcpcm = NULL,*dstpcm = NULL;
    int packet_len = 0;

    if(argc < 2)  {
        printf("Usage: %s <pkg type: 0/pcma 1/pcmu> <filename: default %s> <ch: default %d>\n", argv[0],audiofile,SEND_CHN);
        return -1;
    }

    sscanf(argv[1], "%d", &pkgType);
    if(pkgType == 0)
        pkgType = RTP_PAYLOAD_TYPE_PCMA;
    else
        pkgType = RTP_PAYLOAD_TYPE_PCMU;

    if(argc >= 3){
        memset(audiofile,0,64);
        sscanf(argv[2], "%s", &audiofile);
    }

   if(argc >= 4)
        sscanf(argv[3], "%d", &chn);

    packet_len = chn*2*160;
    srcpcm = malloc(packet_len);
    memset(srcpcm,0,packet_len);
    dstpcm = srcpcm;

    fd = open(audiofile, O_RDONLY);
    if(fd < 0) {
        printf("failed to open %s\n", audiofile);
        return -1;
    }

    if(strstr(audiofile, ".wav")) {
        seekStart = 44;
        read(fd, rtpPacket.payload, seekStart);
    }

    ss = rtp_socket(RTP_IP, RTP_PORT, 1);
    if(!ss) {
        printf("failed to create udp socket\n");
        close(fd);
        return -1;
    }

    rtp_header(&rtpPacket, 0, 0, 0, RTP_VESION, pkgType, 0, 0, 0, 0);
    while(1)  {
        tick1 = getTickUs();

        printf("--------------------------------\n");
        if (!wsdp)  {
            wsdp = 1;
            rtp_create_sdp("./test.sdp", RTP_IP, RTP_PORT, SEND_CHN, SEND_FREQ, pkgType);
        }

        ret = read(fd, srcpcm, packet_len);
        if(ret < sizeof(srcpcm)) {
            lseek(fd, seekStart, SEEK_SET);//循环播放,wav格式的跳过文件头
            continue;
        }
        if(chn == 2){
            dstpcm = pcm;
            ret = stero_to_mono(srcpcm,dstpcm,packet_len);
        }

        if(pkgType == 1)
            ret = PCM2G711u(dstpcm, rtpPacket.payload, ret, 0);//PCM -> PCMU
        else
            ret = PCM2G711a(dstpcm, rtpPacket.payload, ret, 0);//PCM -> PCMA

        //发包
        ret = rtp_send(ss, &rtpPacket, ret);
        if(ret > 0)
            printf("send: %d, %d\n", ret, rtpPacket.rtpHeader.seq);
        
        //时间戳增量
        rtpPacket.rtpHeader.timestamp += SEND_TIMESTAMP;

        //保证每次发包延时一致
        tick2 = getTickUs();
        if(tick2 > tick1 && tick2 - tick1 < SEND_DELAYUS)
            usleep(SEND_DELAYUS - (tick2 - tick1));
        else
            usleep(1000);
        // printf("tick1: %d, tick2: %d, delay: %d\n", tick1, tick2, SEND_DELAYUS-(tick2-tick1));
    }

    close(fd);
    close(ss->fd);
    free(ss);

    return 0;
}
