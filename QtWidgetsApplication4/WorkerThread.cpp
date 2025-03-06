#include "WorkerThread.h"
#include"Common.h"

AVFilterGraph* agraph;
int filter_nbthreads = 0;

AVFilterContext* in_audio_filter;   // the first filter in the audio chain
AVFilterContext* out_audio_filter;
int bmp_encode_frameTest(const AVFrame* pict);
static int configure_filtergraph(AVFilterGraph* graph, const char* filtergraph,
    AVFilterContext* source_ctx, AVFilterContext* sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut* outputs = NULL, * inputs = NULL;

    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
            goto fail;
    }
    else {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
    for (i = 0; i < graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}
static int configure_audio_filters(const char* afilters, int force_output_format)
{
    static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int sample_rates[2] = { 0, -1 };
    int64_t channel_layouts[2] = { 0, -1 };
    int channels[2] = { 0, -1 };
    AVFilterContext* filt_asrc = NULL, * filt_asink = NULL;
    char aresample_swr_opts[512] = "";
    AVDictionaryEntry* e = NULL;
    char asrc_args[256];
    int ret;

    avfilter_graph_free(&agraph);
    if (!(agraph = avfilter_graph_alloc()))
        return AVERROR(ENOMEM);
    agraph->nb_threads = filter_nbthreads;

    /* while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
         av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);*/
    if (strlen(aresample_swr_opts))
        aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
    av_opt_set(agraph, "aresample_swr_opts", aresample_swr_opts, 0);

    /* ret = snprintf(asrc_args, sizeof(asrc_args),
         "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
         is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
         is->audio_filter_src.channels,
         1, is->audio_filter_src.freq);
     if (audio_filter_src.channel_layout)
         snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
             ":channel_layout=0x%"PRIx64, is->audio_filter_src.channel_layout);*/
    memcpy(asrc_args, "sample_rate=48000:sample_fmt=fltp:channels=2:time_base=1/48000:channel_layout=0x3", 82);
    ret = avfilter_graph_create_filter(&filt_asrc,
        avfilter_get_by_name("abuffer"), "ffplay_abuffer",
        asrc_args, NULL, agraph);
    if (ret < 0)
        goto end;


    ret = avfilter_graph_create_filter(&filt_asink,
        avfilter_get_by_name("abuffersink"), "ffplay_abuffersink",
        NULL, NULL, agraph);
    if (ret < 0)
        goto end;

    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;

    if (force_output_format) {
        /* channel_layouts[0] = is->audio_tgt.channel_layout;
         channels[0] = is->audio_tgt.channel_layout ? -1 : is->audio_tgt.channels;
         sample_rates[0] = is->audio_tgt.freq;
         if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
             goto end;
         if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
             goto end;
         if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
             goto end;
         if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
             goto end;*/
    }


    if ((ret = configure_filtergraph(agraph, afilters, filt_asrc, filt_asink)) < 0)
        goto end;

    in_audio_filter = filt_asrc;
    out_audio_filter = filt_asink;

end:
    if (ret < 0)
        avfilter_graph_free(&agraph);
    return ret;
}
WorkerThread::WorkerThread(QObject *parent)
	: QThread(parent)
{
   srcdata = (uint8_t*)malloc(960*540 * 4);
   PlayAudioStream();
   
}

WorkerThread::~WorkerThread()
{}

void WorkerThread::run()
{
  //  encodeImage();
    read_thread();
}


AVDictionary* format_opts;
#define color(r,g,b) ((uint32_t)(b|g<<8|r<<16|0xff<<24))


int WorkerThread::bmp_encode_frame(AVCodecContext* avctx, AVPacket* pkt,
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
    uint8_t* dstData[4];

    dstData[0] = srcdata;
    int distlinesize[AV_NUM_DATA_POINTERS] = { 960 * 4 };
    sws_scale(img_convert_ctx, pict->data, pict->linesize, 0, avctx->height,
        dstData, distlinesize);
    tranData = (uint32_t*)srcdata;
    //emit sigImg((uint32_t*)srcdata);
    return 0;
    // STRUCTURE.field refer to the MSVC documentation for BITMAPFILEHEADER
   // and related pages.
#define SIZE_BITMAPFILEHEADER 14
#define SIZE_BITMAPINFOHEADER 40
    hsize = SIZE_BITMAPFILEHEADER + SIZE_BITMAPINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;

    pkt->data = (uint8_t*)malloc(n_bytes);
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
    fp = fopen("csQtYUV.bmp", "wb+");
    fwrite(pkt->data, sizeof(uint8_t), n_bytes, fp);
    fflush(fp);
    fclose(fp);
    return 0;

}
struct YuvData {
    uint8_t* yuvData[AV_NUM_DATA_POINTERS];
    int yumLinesize[AV_NUM_DATA_POINTERS];
};
YuvData change2yuv(uint8_t* rgb_data, int width, int height)
{
    YuvData yuvdata;
    // 创建SwsContext用于转换
    SwsContext* sws_ctx = sws_getContext(width, height,
        AV_PIX_FMT_RGB32, // 输入格式
        width, height,
        AV_PIX_FMT_YUV420P, // 输出格式
        SWS_BILINEAR, NULL, NULL, NULL);

    // 将RGB数据转换为YUV420P
    uint8_t* yuvData[AV_NUM_DATA_POINTERS] = { 0 };
    int yumLinesize[AV_NUM_DATA_POINTERS] = { 0 };
    if (av_image_alloc(yuvData, yumLinesize, width, height, AV_PIX_FMT_YUV420P, 1) < 0)
    {
        std::cerr << "Error allocating image: " << std::endl;
    }
    uint8_t* rgbDatas[AV_NUM_DATA_POINTERS] = { 0 };
    int rgbLinesize[AV_NUM_DATA_POINTERS] = { 0 };
    rgbDatas[0] = rgb_data;
    rgbLinesize[0] = width * 4;
    sws_scale(sws_ctx, rgbDatas, rgbLinesize, 0, height, yuvData, yumLinesize);
    sws_freeContext(sws_ctx);
    yuvdata.yuvData[0] = yuvData[0];
    yuvdata.yuvData[1] = yuvData[1];
    yuvdata.yuvData[2] = yuvData[2];

    yuvdata.yumLinesize[0] = yumLinesize[0];
    yuvdata.yumLinesize[1] = yumLinesize[1];
    yuvdata.yumLinesize[2] = yumLinesize[2];





    return yuvdata;
}
void WorkerThread::encodeImage()
{
    FILE* fp = NULL;
    fp = fopen("csQt.bmp", "rb");
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t* data = (uint8_t*)malloc(size);
    ;
    fread(data, 1, size, fp);
    uint8_t* data2 = (uint8_t*)malloc(((size - 54) / 3) * 4);
    int idx = 0;
    int idx2 = 0;
   for (; idx < size; ) {
        data2[idx2] = data[idx + 54];
        idx2++;
        idx++;
        data2[idx2] = data[idx + 54];
        idx2++;
        idx++;
        data2[idx2] = data[idx + 54];
        idx2++;
        idx++;
        data2[idx2] = 0xff;
        idx2++;
    }
    data = data + 54;
    char imgData[100] = { 0 };
    memcpy(imgData, data,100);
    const AVCodec* pEncoderH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
    //视频编码器上下文
    AVCodecContext* m_pEncoderH264Ctx = avcodec_alloc_context3(pEncoderH264);
    m_pEncoderH264Ctx->bit_rate = 400000;
    m_pEncoderH264Ctx->width = 960;
    m_pEncoderH264Ctx->height = 540;
    m_pEncoderH264Ctx->time_base.num = 1;
    m_pEncoderH264Ctx->time_base.den = 10; // 帧率为10fps
    m_pEncoderH264Ctx->framerate = { 10, 1 };
    m_pEncoderH264Ctx->gop_size = 1;      // 关键帧间隔
    m_pEncoderH264Ctx->max_b_frames = 0;
    m_pEncoderH264Ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    int ret = avcodec_open2(m_pEncoderH264Ctx, pEncoderH264, NULL);
    //编码器不用等待缓冲区填满，接收到数据即开始编码
    av_opt_set(m_pEncoderH264Ctx->priv_data, "tune", "zerolatency", 0);
    YuvData yuvData = change2yuv(data2, 960, 540);
    AVFrame* frame = av_frame_alloc();
    memset(frame, 0, sizeof(AVFrame));
    frame->data[0] = yuvData.yuvData[0];
    frame->data[1] = yuvData.yuvData[1];
    frame->data[2] = yuvData.yuvData[2];
    frame->linesize[0] = yuvData.yumLinesize[0];
    frame->linesize[1] = yuvData.yumLinesize[1];
    frame->linesize[2] = yuvData.yumLinesize[2];
    frame->height = 540;
    frame->width = 960;
    frame->color_range = AVCOL_RANGE_MPEG;
    frame->color_primaries = AVCOL_PRI_UNSPECIFIED;
    frame->color_trc = AVCOL_TRC_UNSPECIFIED;
    frame->colorspace = AVCOL_SPC_UNSPECIFIED;
    frame->chroma_location = AVCHROMA_LOC_UNSPECIFIED;
    frame->format = m_pEncoderH264Ctx->pix_fmt;
    AVPacket* pkt = av_packet_alloc();
    int got_pictuure = 0;
    bmp_encode_frameTest( frame);

    
    ret = avcodec_send_frame(m_pEncoderH264Ctx, frame);
   
    AVPacket* h264_pkt = av_packet_alloc();//存读取和编码后的H264数据
    while (1) {
        ret = avcodec_receive_packet(m_pEncoderH264Ctx, h264_pkt);
        avcodec_send_frame(m_pEncoderH264Ctx, NULL);
        if (ret > 0) {
            uint8_t packetData[1024] = { 0 };
            // memcpy(packetData,(void*)h264_pkt->data[0],h264_pkt->size);
            packetData[5] = 0;
        }
        
    }
   
}



void WorkerThread::read_thread()
{
    filename = "rtmp://ns8.indexforce.com/home/mystream";
    filename = "rtmp://127.0.0.1:5236/home/mystream";
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
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    //这句话有用  是获根据名字获取iformat的  但stream不是从这设置的
    err = avformat_open_input(&ic, filename.toStdString().c_str(), NULL, &format_opts);
    av_format_inject_global_side_data(ic);
    static int find_stream_info = 1;
    if (find_stream_info) {

        AVDictionary** opts = NULL;
        int orig_nb_streams = ic->nb_streams;

        err = avformat_find_stream_info(ic, opts);  //这句找的stream

        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (err < 0) {

            ret = -1;
            return;
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
        int num = sar.num;
        int den = sar.den;
    }
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(ic,st_index[AVMEDIA_TYPE_AUDIO],&audioAvctx);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(ic, st_index[AVMEDIA_TYPE_VIDEO], &videoAvctx);
    }
    int countSample = 0;
    char* audioBuf = (char*)malloc(8192 * 2);
    configure_audio_filters(0, 0);
    while (1) {
        ret = av_read_frame(ic, pkt);
        if (pkt->stream_index ==0) {           //vedio
            avcodec_send_packet(videoAvctx, pkt);

            AVFrame* frame = av_frame_alloc();
            ret = avcodec_receive_frame(videoAvctx, frame);

            if (ret >= 0) {
                int got_pictuure;
                AVPacket* pkt = av_packet_alloc();
                bmp_encode_frame(videoAvctx, pkt, frame, &got_pictuure);

            }
        }
        if (pkt->stream_index == 2) {
            avcodec_send_packet(audioAvctx, pkt);

            AVFrame* frame = av_frame_alloc();
            ret = avcodec_receive_frame(audioAvctx, frame);

            if (ret >= 0) {
                AVRational tb;
                tb.den = 1;
                tb.den = frame->sample_rate;
              //  av_rescale_q(frame->pts, audioAvctx->pkt_timebase, tb);
                // memset(audioBuf, 0xcd, 8192 * 2);
                /*av_buffersrc_add_frame(in_audio_filter, frame);
                av_buffersink_get_frame_flags(out_audio_filter, frame, 0);
                memcpy(audioBuf, frame->data[0], 8192);*/

                /*int sampleCount = 0;
                char name[100] = { 0 };
                sprintf(name, "./frameData/0.dat", sampleCount);
                sampleCount++;
                FILE* fileptr = fopen(name, "rb");
                int Read_Size = fread(frame->data[0], 1, 8192, fileptr);
                fflush(fileptr);
                fclose(fileptr);

                sprintf(name, "./frameData/1.dat", sampleCount);
                sampleCount++;
                fileptr = fopen(name, "rb");
                Read_Size = fread(frame->data[1], 1, 8192, fileptr);
                fflush(fileptr);
                fclose(fileptr);*/
                SwrContext* sw= swr_alloc();
                
                sw= swr_alloc_set_opts(sw,
                    3, (AVSampleFormat)1, 4800,
                    3, (AVSampleFormat)8, 4800,
                    0, 0);
                swr_init(sw);
                uint8_t* out= (uint8_t*)malloc(8192*2); 
               // memcpy(audioBuf, frame->data[0], 8192);
                //const uint8_t** in = (const uint8_t**)&(audioBuf);
                swr_convert(sw, &out, 1056,(const uint8_t**) frame->data, 1024);      
                /*sprintf(name, "./frameData/selfdata.dat", sampleCount);
                sampleCount++;
                fileptr = fopen(name, "wb");
                Read_Size = fwrite(out, 1, 8192, fileptr);
                fflush(fileptr);
                fclose(fileptr);*/
                for (int i = 4096; i < 8192; i++) {
                    out[i] = 0;
                }
                playSound((char*)out);
            
               
            }
        }
    }
    return;
}

int WorkerThread::stream_component_open(AVFormatContext* ic, int stream_index, AVCodecContext** avctx1)
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

    *avctx1 = avcodec_alloc_context3(NULL);
    AVCodecContext* avctx = *avctx1;
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

int bmp_encode_frameTest(const AVFrame* pict)
{
   
    int bit_count = 24;
    const AVFrame* const p = pict;
    int n_bytes_image, n_bytes_per_row, n_bytes, i, n, hsize, ret;
    const uint32_t* pal = NULL;
    uint32_t palette256[256];
    int pad_bytes_per_row, pal_entries = 0, compression = 0;

    uint8_t* buf;

    if (pal && !pal_entries) pal_entries = 1 << bit_count;
    n_bytes_per_row = ((int64_t)480 * (int64_t)bit_count + 7LL) >> 3LL;
    pad_bytes_per_row = (4 - n_bytes_per_row) & 3;
    n_bytes_image =270* (n_bytes_per_row + pad_bytes_per_row);
    struct SwsContext* img_convert_ctx = sws_getContext(960, 540,
        AV_PIX_FMT_YUV420P, 480, 270,
        AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    int imgSize = 480 * 270;
    uint8_t* dstData[4];
    uint8_t* srcdata = (uint8_t*)malloc(480 * 270 * 4);
    dstData[0] = srcdata;
    int distlinesize[AV_NUM_DATA_POINTERS] = { 480 * 4 };
    sws_scale(img_convert_ctx, pict->data, pict->linesize, 0, 540,
        dstData, distlinesize);
    
    //emit sigImg((uint32_t*)srcdata);
   // return 0;
    // STRUCTURE.field refer to the MSVC documentation for BITMAPFILEHEADER
   // and related pages.
#define SIZE_BITMAPFILEHEADER 14
#define SIZE_BITMAPINFOHEADER 40
    hsize = SIZE_BITMAPFILEHEADER + SIZE_BITMAPINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;

    uint8_t*  fileptr=(uint8_t*)malloc(n_bytes);
    buf = fileptr;
    bytestream_put_byte((char**)&buf, 'B');                   // BITMAPFILEHEADER.bfType
    bytestream_put_byte((char**)&buf, 'M');                   // do.
    bytestream_put_le32((uint32_t**)&buf, n_bytes);               // BITMAPFILEHEADER.bfSize
    bytestream_put_le16((uint16_t**)&buf, 0);                     // BITMAPFILEHEADER.bfReserved1
    bytestream_put_le16((uint16_t**)&buf, 0);                     // BITMAPFILEHEADER.bfReserved2
    bytestream_put_le32((uint32_t**)&buf, hsize);                 // BITMAPFILEHEADER.bfOffBits
    bytestream_put_le32((uint32_t**)&buf, SIZE_BITMAPINFOHEADER); // BITMAPINFOHEADER.biSize
    bytestream_put_le32((uint32_t**)&buf, 480);          // BITMAPINFOHEADER.biWidth
    bytestream_put_le32((uint32_t**)&buf, 270);         // BITMAPINFOHEADER.biHeight
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




    uint8_t* lineData = (uint8_t*)malloc(480 * 3);
    int bmpHeight = 270;
    int bmpWidth = 480;
    char imgData[100] = { 0 };
    memcpy(imgData, srcdata, 100);
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
        memcpy(buf, lineData, 480 * 3);
        buf = buf + (480 * 3);
    }

    FILE* fp = NULL;
    fp = fopen("csQtYUV.bmp", "wb");
    fwrite(fileptr, sizeof(uint8_t), n_bytes, fp);
    fflush(fp);
    fclose(fp);
    return 0;

}