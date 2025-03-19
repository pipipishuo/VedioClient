#include "PublishThread.h"
#include"winsock2.h"
#include"Common.h"
#include"rtmpTool.h"
extern "C"
{
#include"libavcodec/avcodec.h"
#include"libswscale/swscale.h"
#include "libavformat/avformat.h"
#include"libavfilter/avfilter.h"
#include<libavutil/opt.h>
#include<libavfilter/buffersrc.h>
#include<libavfilter/buffersink.h>
#include<libswresample/swresample.h>
#include<libavutil/imgutils.h>
#include<libavutil/log.h>
#include<libavutil/lfg.h>
}
#define BUFFER_SIZE 1024
int rtmp_handshake(int server_fd);
int gen_connect(int server_fd);
void sendWindowSize(int new_socket, int size);
int gen_create_stream(int new_socket);
int chunkSize = 0;
int gen_publish(int new_socket);
void PublishThread::run() {
    chunkSize = 0;
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return ;
    }
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char* buffer = (char*)malloc(BUFFER_SIZE);

    ;
    const char* hello = "Hello from server";

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 绑定IP和端口
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(7777);
    err = bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    if (err < 0) {
        perror("Bind failed");
        closesocket(server_fd);
        exit(EXIT_FAILURE);
    }
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
    clientService.sin_port = htons(5236);
    err = ::connect(server_fd, (SOCKADDR*)&clientService, sizeof(clientService));
    if (err < 0) {
        err=WSAGetLastError();
        perror("connect failed");
        closesocket(server_fd);
        exit(EXIT_FAILURE);
    }
   //开始握手
    rtmp_handshake(server_fd);
    //建立连接
    gen_connect(server_fd);
    //接收发回来的东西
    //WindowSize
    RTMPPacket p;
    RTMPPacket* prev_pkt = (RTMPPacket*)malloc(sizeof(RTMPPacket) * 64);
    memset(prev_pkt, 0, sizeof(RTMPPacket) * 64);
    int* nb_prev_pkt = 0;
    uint8_t hdr = 0;
    ff_rtmp_packet_read_internal(server_fd, &p, 128,
        &prev_pkt, nb_prev_pkt,
        hdr);
    //PeerBandwidth
    ff_rtmp_packet_read_internal(server_fd, &p, 128,
        &prev_pkt, nb_prev_pkt,
        hdr);
    //UserControl
    ff_rtmp_packet_read_internal(server_fd, &p, 128,
        &prev_pkt, nb_prev_pkt,
        hdr);
    //ChunkSize
    ff_rtmp_packet_read_internal(server_fd, &p, 128,
        &prev_pkt, nb_prev_pkt,
        hdr);
    if (p.type == RTMP_PT_CHUNK_SIZE) {
       chunkSize=AV_RB32( p.data);
    }
    //ConnectReponse
    ff_rtmp_packet_read_internal(server_fd, &p, chunkSize,
        &prev_pkt, nb_prev_pkt,
        hdr);
    sendWindowSize(server_fd, 2500000);
    gen_create_stream(server_fd);
    //CreateStreamReponse
    ff_rtmp_packet_read_internal(server_fd, &p, chunkSize,
        &prev_pkt, nb_prev_pkt,
        hdr);
    //发送publish
    gen_publish(server_fd);
}

#define RTMP_HANDSHAKE_PACKET_SIZE 1536
#define RTMP_CLIENT_VER1    9
#define RTMP_CLIENT_VER2    0
#define RTMP_CLIENT_VER3  124
#define RTMP_CLIENT_VER4    2
int rtmp_handshake(int server_fd)
{
    AVLFG rnd;
    uint8_t tosend    [RTMP_HANDSHAKE_PACKET_SIZE+1] = {
        3,                // unencrypted data
        0, 0, 0, 0,       // client uptime
        RTMP_CLIENT_VER1,
        RTMP_CLIENT_VER2,
        RTMP_CLIENT_VER3,
        RTMP_CLIENT_VER4,
    };
    uint8_t clientdata[RTMP_HANDSHAKE_PACKET_SIZE];
    uint8_t serverdata[RTMP_HANDSHAKE_PACKET_SIZE+1];
    int i;
    int server_pos, client_pos;
    uint8_t digest[32], signature[32];
    int ret, type = 0;

   // av_log(s, AV_LOG_DEBUG, "Handshaking...\n");

    av_lfg_init(&rnd, 0xDEADC0DE);
    // generate handshake packet - 1536 bytes of pseudorandom data
    for (i = 9; i <= RTMP_HANDSHAKE_PACKET_SIZE; i++)
        tosend[i] = av_lfg_get(&rnd) >> 24;

    

    client_pos = rtmp_handshake_imprint_with_digest(tosend + 1,0);
    if (client_pos < 0)
        return client_pos;

    if ((ret = send(server_fd, (const char*)tosend,
                           RTMP_HANDSHAKE_PACKET_SIZE + 1,0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot write RTMP handshake request\n");
        return ret;
    }

    if ((ret = recv(server_fd, (char*)serverdata,
                                   RTMP_HANDSHAKE_PACKET_SIZE + 1,0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot read RTMP handshake response\n");
        return ret;
    }

    if ((ret = recv(server_fd, (char*)clientdata,
                                   RTMP_HANDSHAKE_PACKET_SIZE,0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot read RTMP handshake response\n");
        return ret;
    }

    av_log(NULL, AV_LOG_DEBUG, "Type answer %d\n", serverdata[0]);
    av_log(NULL, AV_LOG_DEBUG, "Server version %d.%d.%d.%d\n",
           serverdata[5], serverdata[6], serverdata[7], serverdata[8]);
    //截至到这就是两个c0 和c1交互  但c1也不太对啊这
    if (1 && serverdata[5] >= 3) {
        server_pos = rtmp_validate_digest(serverdata + 1, 772);
        if (server_pos < 0)
            return server_pos;

        if (!server_pos) {
            type = 1;
            server_pos = rtmp_validate_digest(serverdata + 1, 8);
            if (server_pos < 0)
                return server_pos;

            if (!server_pos) {
                av_log(NULL, AV_LOG_ERROR, "Server response validating failed\n");
                return AVERROR(EIO);
            }
        }

        

        ret = ff_rtmp_calc_digest(tosend + 1 + client_pos, 32, 0,
                                  rtmp_server_key, sizeof(rtmp_server_key),
                                  digest);
        if (ret < 0)
            return ret;

        ret = ff_rtmp_calc_digest(clientdata, RTMP_HANDSHAKE_PACKET_SIZE - 32,
                                  0, digest, 32, signature);
        if (ret < 0)
            return ret;

       
        if (memcmp(signature, clientdata + RTMP_HANDSHAKE_PACKET_SIZE - 32, 32)) {
            av_log(NULL, AV_LOG_ERROR, "Signature mismatch\n");
            return AVERROR(EIO);
        }

        for (i = 0; i < RTMP_HANDSHAKE_PACKET_SIZE; i++)
            tosend[i] = av_lfg_get(&rnd) >> 24;
        ret = ff_rtmp_calc_digest(serverdata + 1 + server_pos, 32, 0,
                                  rtmp_player_key, sizeof(rtmp_player_key),
                                  digest);
        if (ret < 0)
            return ret;

        ret = ff_rtmp_calc_digest(tosend, RTMP_HANDSHAKE_PACKET_SIZE - 32, 0,
                                  digest, 32,
                                  tosend + RTMP_HANDSHAKE_PACKET_SIZE - 32);
        if (ret < 0)
            return ret;

        

        // write reply back to the server
        if ((ret = send(server_fd, (const char*)tosend,
                               RTMP_HANDSHAKE_PACKET_SIZE,0)) < 0)
            return ret;

        
    } else {
        
    }

    return 0;
}
struct URLContext {

};
struct RTMPContext {

};


int gen_connect(int server_fd)
{
    RTMPPacket pkt;
    uint8_t* p;
    int ret;

    if ((ret = ff_rtmp_packet_create(&pkt, RTMP_SYSTEM_CHANNEL, RTMP_PT_INVOKE,
        0, 4096 + APP_MAX_LENGTH)) < 0)
        return ret;

    p = pkt.data;

    ff_amf_write_string(&p, "connect");
    ff_amf_write_number(&p, 1);
    ff_amf_write_object_start(&p);
    ff_amf_write_field_name(&p, "app");
    ff_amf_write_string2(&p, "home", NULL);

    
    ff_amf_write_field_name(&p, "flashVer");
    ff_amf_write_string(&p, "LNX 9,0,124,2");

    

    ff_amf_write_field_name(&p, "tcUrl");
    ff_amf_write_string2(&p, "rtmp://127.0.0.1:5236/home",NULL);
    if (1) {
        ff_amf_write_field_name(&p, "fpad");
        ff_amf_write_bool(&p, 0);
        ff_amf_write_field_name(&p, "capabilities");
        ff_amf_write_number(&p, 15.0);

        /* Tell the server we support all the audio codecs except
         * SUPPORT_SND_INTEL (0x0008) and SUPPORT_SND_UNUSED (0x0010)
         * which are unused in the RTMP protocol implementation. */
        ff_amf_write_field_name(&p, "audioCodecs");
        ff_amf_write_number(&p, 4071.0);
        ff_amf_write_field_name(&p, "videoCodecs");
        ff_amf_write_number(&p, 252.0);
        ff_amf_write_field_name(&p, "videoFunction");
        ff_amf_write_number(&p, 1.0);

       
    }
    ff_amf_write_object_end(&p);

    

    pkt.size = p - pkt.data;
    RTMPPacket* prev_pkt_ptr = NULL;
    int nb_prev_pkt = 0;
    return ff_rtmp_packet_write(server_fd, &pkt, 128,&prev_pkt_ptr,&nb_prev_pkt);
}
void sendWindowSize(int new_socket, int size) {
    RTMPPacket p;
    p.channel_id = 3;
    p.ts_field = 0;
    p.size = 4;
    p.type = RTMP_PT_WINDOW_ACK_SIZE;
    p.extra = 0;
    uint8_t* windowsize = (uint8_t*)malloc(4);
    AV_WB32(windowsize, size);
    p.data = windowsize;
    RTMPPacket* prev_pkt_ptr = NULL;
    int nb_prev_pkt = 0;
    ff_rtmp_packet_write(new_socket, &p, chunkSize, &prev_pkt_ptr, &nb_prev_pkt);
}
int gen_create_stream(int new_socket)
{
    RTMPPacket pkt;
    uint8_t* p;
    int ret;

    //av_log(s, AV_LOG_DEBUG, "Creating stream...\n");

    if ((ret = ff_rtmp_packet_create(&pkt, RTMP_SYSTEM_CHANNEL, RTMP_PT_INVOKE,
        0, 25)) < 0)
        return ret;

    p = pkt.data;
    ff_amf_write_string(&p, "createStream");
    ff_amf_write_number(&p, 2);
    ff_amf_write_null(&p);
    RTMPPacket* prev_pkt_ptr = NULL;
    int nb_prev_pkt = 0;
    ff_rtmp_packet_write(new_socket, &pkt, chunkSize, &prev_pkt_ptr, &nb_prev_pkt);
    return 1;
}

int gen_publish(int new_socket)
{
    RTMPPacket pkt;
    uint8_t* p;
    int ret;

   // av_log(s, AV_LOG_DEBUG, "Sending publish command for '%s'\n", rt->playpath);

    if ((ret = ff_rtmp_packet_create(&pkt, RTMP_SOURCE_CHANNEL, RTMP_PT_INVOKE,
        0, 30 + strlen("test"))) < 0)
        return ret;

    pkt.extra =3;

    p = pkt.data;
    ff_amf_write_string(&p, "publish");
    ff_amf_write_number(&p, 1);
    ff_amf_write_null(&p);
    ff_amf_write_string(&p, "test");
    ff_amf_write_string(&p, "live");
    RTMPPacket* prev_pkt_ptr = NULL;
    int nb_prev_pkt = 0;
    ff_rtmp_packet_write(new_socket, &pkt, chunkSize, &prev_pkt_ptr, &nb_prev_pkt);
    return 1;
}