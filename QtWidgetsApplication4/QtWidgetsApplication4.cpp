#include "QtWidgetsApplication4.h"
#include"qpainter.h"
#include<iostream>
#include<stdint.h>
#include"Common.h"

#define color(r,g,b) ((uint32_t)(b|g<<8|r<<16|0xff<<24))


int QtWidgetsApplication4::bmp_encode_frame(AVCodecContext* avctx, AVPacket* pkt,
    const AVFrame* pict, int* got_packet)
{
    if (avctx->sw_pix_fmt != AV_PIX_FMT_YUV420P) {
        return 0;
    }
    int bit_count = 24;
    const AVFrame* const p = pict;
    int n_bytes_image, n_bytes_per_row, n_bytes, i, n, hsize, ret;
    const uint32_t* pal = NULL;
    uint32_t palette256[256];
    int pad_bytes_per_row, pal_entries = 0, compression = 0;

    uint8_t* buf;

    if (pal && !pal_entries) pal_entries = 1 << bit_count;
    n_bytes_per_row = ((int64_t)avctx->width / 2 * (int64_t)bit_count + 7LL) >> 3LL;
    pad_bytes_per_row = (4 - n_bytes_per_row) & 3;
    n_bytes_image = avctx->height / 2 * (n_bytes_per_row + pad_bytes_per_row);
    struct SwsContext* img_convert_ctx = sws_getContext(avctx->width, avctx->height,
        AV_PIX_FMT_YUV420P, avctx->width / 2, avctx->height / 2,
        AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    int imgSize = avctx->height / 2 * avctx->width / 2;
    uint8_t* srcdata = (uint8_t*)malloc(imgSize * 4);
    uint8_t* dstData[4];
    dstData[0] = srcdata;
    int distlinesize[AV_NUM_DATA_POINTERS] = { 960 * 4 };
    sws_scale(img_convert_ctx, pict->data, pict->linesize, 0, avctx->height,
        dstData, distlinesize);
    tranData = (uint32_t*)srcdata;

    // STRUCTURE.field refer to the MSVC documentation for BITMAPFILEHEADER
   // and related pages.
#define SIZE_BITMAPFILEHEADER 14
#define SIZE_BITMAPINFOHEADER 40
    hsize = SIZE_BITMAPFILEHEADER + SIZE_BITMAPINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;
   
    pkt->data =(uint8_t*) malloc(n_bytes);
    buf = pkt->data;
    bytestream_put_byte((char**)&buf, 'B');                   // BITMAPFILEHEADER.bfType
    bytestream_put_byte((char**)&buf, 'M');                   // do.
    bytestream_put_le32((uint32_t**)&buf, n_bytes);               // BITMAPFILEHEADER.bfSize
    bytestream_put_le16((uint16_t**)&buf, 0);                     // BITMAPFILEHEADER.bfReserved1
    bytestream_put_le16((uint16_t**)&buf, 0);                     // BITMAPFILEHEADER.bfReserved2
    bytestream_put_le32((uint32_t**)&buf, hsize);                 // BITMAPFILEHEADER.bfOffBits
    bytestream_put_le32((uint32_t**)&buf, SIZE_BITMAPINFOHEADER); // BITMAPINFOHEADER.biSize
    bytestream_put_le32((uint32_t**)&buf, avctx->width / 2);          // BITMAPINFOHEADER.biWidth
    bytestream_put_le32((uint32_t**)&buf, avctx->height / 2);         // BITMAPINFOHEADER.biHeight
    bytestream_put_le16((uint16_t**)&buf, 1);                     // BITMAPINFOHEADER.biPlanes
    bytestream_put_le16((uint16_t**)&buf, bit_count);             // BITMAPINFOHEADER.biBitCount
    bytestream_put_le32((uint32_t**)&buf, compression);           // BITMAPINFOHEADER.biCompression
    bytestream_put_le32((uint32_t**)&buf, n_bytes_image);         // BITMAPINFOHEADER.biSizeImage
    bytestream_put_le32((uint32_t**)&buf, 0);                     // BITMAPINFOHEADER.biXPelsPerMeter
    bytestream_put_le32((uint32_t**)&buf, 0);                     // BITMAPINFOHEADER.biYPelsPerMeter
    bytestream_put_le32((uint32_t**)&buf, 0);                     // BITMAPINFOHEADER.biClrUsed
    bytestream_put_le32((uint32_t**)&buf, 0);                     // BITMAPINFOHEADER.biClrImportant
    for (i = 0; i < pal_entries; i++)
        bytestream_put_le32((uint32_t**)&buf, pal[i] & 0xFFFFFF);
    // BMP files are bottom-to-top so we start from the end...




    uint8_t* lineData = (uint8_t*)malloc(960 * 3);
    int bmpHeight = avctx->height / 2;
    int bmpWidth = avctx->width / 2;
    for (int i = bmpHeight - 1; i > 0; i--) {

        int heightIdx = i * bmpWidth;
        for (int j = 0; j < bmpWidth; j++) {
            int bmpIdx = heightIdx + j;
            int srcIdx = bmpIdx * 4;
            uint8_t bVal = srcdata[srcIdx];
            srcIdx++;
            uint8_t gVal = srcdata[srcIdx];
            srcIdx++;
            uint8_t rVal = srcdata[srcIdx];
            int lineIdx = j * 3;
            lineData[lineIdx] = bVal;
            lineIdx++;
            lineData[lineIdx] = gVal;
            lineIdx++;
            lineData[lineIdx] = rVal;
        }
        memcpy(buf, lineData, 960 * 3);
        buf = buf + (960 * 3);
    }
   
    FILE* fp = NULL;
    fp = fopen("csQt.bmp", "wb+");
    fwrite(pkt->data, sizeof(uint8_t), n_bytes, fp);
    fflush(fp);
    fclose(fp);
    return 0;

}
QtWidgetsApplication4::QtWidgetsApplication4(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    width = 1920;
    height = 1080;
    
    uint8_t* srcdata[3] = { 0 };
    FILE* fp = NULL;
    fp = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/ydata", "r");
    int bmpsize = height * width;
    srcdata[0] = (uint8_t*)malloc(bmpsize);
    fread(srcdata[0], sizeof(uint8_t), bmpsize, fp);


    fp = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/udata", "r");
    bmpsize = bmpsize / 2;
    srcdata[1] = (uint8_t*)malloc(bmpsize);
    fread(srcdata[1], sizeof(uint8_t), bmpsize, fp);

   
    fp = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/vdata", "r");
    srcdata[2] = (uint8_t*)malloc(bmpsize);
    fread(srcdata[2], sizeof(uint8_t), bmpsize, fp);
    

    struct SwsContext* img_convert_ctx = sws_getContext(width, height,
        AV_PIX_FMT_YUV420P, width/2, height/2,
        AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    int imgSize = bmpsize;
    
    uint8_t* disdata = (uint8_t*)malloc(imgSize*4);
    memset(disdata, 0, imgSize*4);
    
   
    uint8_t* dstData[4];
    dstData[0] = disdata;
    
    int srclinesize[AV_NUM_DATA_POINTERS] = { 1920,960,960 };
    int distlinesize[AV_NUM_DATA_POINTERS] = { 1920*2};
    sws_scale(img_convert_ctx, srcdata, srclinesize, 0, height,
        dstData, distlinesize);


  /*  fp = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/cstttt.bmp", "r");
    bmpsize=(height )* (width ) * 3 + 54;
    uint8_t* bmpData= (uint8_t*)malloc(bmpsize);
    fread(bmpData, sizeof(uint8_t), bmpsize, fp);
    bmpData = bmpData + 54;*/
    int size = height* width * sizeof(uint32_t);
    data = (uint32_t*)malloc(size);
    uint32_t fline[960] = { 0 };
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++) {
            int idx = i * width/2+ j;
            int imgidx = idx * 4;
            //int a = bmpData[imgidx + 3];
            int r = disdata[imgidx +2];
            int g = disdata[imgidx +1];
            int b = disdata[imgidx];
            uint32_t val = color(r, g, b);
            
            data[idx] = val;
            if (i == 0) {
                fline[idx] = val;
            }
        }
    }

    size = height/2 * width/2 * sizeof(uint32_t);
    tranData = (uint32_t*)malloc(size);
    fp = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/srcData", "r");
    uint32_t* imgdata = (uint32_t*)malloc(size);
    fread(imgdata, sizeof(uint8_t), size, fp);
    char rr[16] = { 0 };
    memcpy(imgdata, rr, 16);
    tranData = imgdata;
    /*uint32_t sline[960] = { 0 };
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++) {
            int idx = i * width/2+ j;
            
            uint32_t val = imgdata[idx];
            val = val | ((0xff) << 24);
            tranData[idx] = val;
            if (i == 0) {
                sline[idx] = val;
            }
        }
    }*/

    read_thread();
}

QtWidgetsApplication4::~QtWidgetsApplication4()
{}

void QtWidgetsApplication4::read_thread()
{
    filename= "rtmp://ns8.indexforce.com/home/mystream";
    filename= "rtmp://127.0.0.1:5236/home/mystream";
    //VideoState* is = arg;
    AVFormatContext* ic = NULL;
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket* pkt = NULL;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    AVDictionaryEntry* t;
   // SDL_mutex* wait_mutex = SDL_CreateMutex();
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;
    
  /*  if (!wait_mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", "s");
        ret = AVERROR(ENOMEM);
        return ;
    }*/

    memset(st_index, -1, sizeof(st_index));
   

    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate packet.\n");
        ret = AVERROR(ENOMEM);
        return;
    }
    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        return;
    }
    //ic->interrupt_callback.callback = decode_interrupt_cb;
    //ic->interrupt_callback.opaque = is;
    AVDictionary* format_opts=NULL;
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    //这句话有用  是获根据名字获取iformat的  但stream不是从这设置的
    err = avformat_open_input(&ic, filename.toStdString().c_str(), NULL, &format_opts);
    av_format_inject_global_side_data(ic);
    static int find_stream_info = 1;
    if (find_stream_info) {

        AVDictionary** opts=NULL;
        int orig_nb_streams = ic->nb_streams;

        err = avformat_find_stream_info(ic, opts);  //这句找的stream

        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (err < 0) {
            
            ret = -1;
            return ;
        }
    }
    if (ic->pb)
        ic->pb->eof_reached = 0;
    static int seek_by_bytes = -1;
    if (seek_by_bytes < 0)
        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
    int max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    static int64_t start_time = AV_NOPTS_VALUE;

    
    av_dump_format(ic, 0, filename.toStdString().c_str(), 0);

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream* st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        
    }
    st_index[AVMEDIA_TYPE_VIDEO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
            st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    st_index[AVMEDIA_TYPE_AUDIO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
            st_index[AVMEDIA_TYPE_AUDIO],
            st_index[AVMEDIA_TYPE_VIDEO],
            NULL, 0);
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream* st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters* codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        int num=sar.num;
        int den = sar.den;
    }
    /*if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(st_index[AVMEDIA_TYPE_AUDIO]);
    }*/

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(ic,st_index[AVMEDIA_TYPE_VIDEO]);
    }
    while (1) {
        ret = av_read_frame(ic, pkt);
        if (pkt->stream_index == 1) {           //vedio
            avcodec_send_packet(avctx,pkt);

            AVFrame* frame = av_frame_alloc();
            ret = avcodec_receive_frame(avctx, frame);

            if (ret >= 0) {
                int got_pictuure;
                AVPacket* pkt = av_packet_alloc();
                bmp_encode_frame(avctx, pkt, frame, &got_pictuure);
               
            }
        }
        
    }
    

    return;
}
int QtWidgetsApplication4::stream_component_open(AVFormatContext* ic , int stream_index)
{
    
   
    const AVCodec* codec;
    const char* forced_codec_name = NULL;
    AVDictionary* opts = NULL;
    AVDictionaryEntry* t = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int ret = 0;
    int stream_lowres = 0;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
    //这几行根据ic->streams[stream_index]->codecpar找的对应codec
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        return ret;
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;
    //这几行根据ic->streams[stream_index]->codecpar找的对应codec
    codec = avcodec_find_decoder(avctx->codec_id);

    /*switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO: is->last_audio_stream = stream_index; forced_codec_name = NULL; break;
    case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name = NULL; break;
    case AVMEDIA_TYPE_VIDEO: is->last_video_stream = stream_index; forced_codec_name = NULL; break;
    }*/
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
            "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
            "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        return ret;
    }

    avctx->codec_id = codec->id;
    if (stream_lowres > codec->max_lowres) {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
            codec->max_lowres);
        stream_lowres = codec->max_lowres;
    }
    avctx->lowres = stream_lowres;
    int fast = 0;
    if (fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;

    opts = NULL;
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        goto fail;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    //is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:

        break;
    case AVMEDIA_TYPE_VIDEO:
       // is->video_stream = stream_index;
        //is->video_st = ic->streams[stream_index];

       /* if ((ret = decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread)) < 0)
            goto fail;
        if ((ret = decoder_start(&is->viddec, video_thread, "video_decoder", is)) < 0)
            goto out;*/
        //is->queue_attachments_req = 1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
      
        break;
    default:
        break;
    }
    goto out;

fail:
    avcodec_free_context(&avctx);
out:
    av_dict_free(&opts);

    return ret;
}
void QtWidgetsApplication4::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    QImage img((uchar*)data, width/2, height/2, QImage::Format_RGB32);
    painter.drawImage(QPoint(0, 0), img);

    QImage img2((uchar*)tranData, width / 2, height / 2, QImage::Format_RGB32);
    painter.drawImage(QPoint(960, 540), img2);
}
