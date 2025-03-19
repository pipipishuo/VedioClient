#include "QtWidgetsApplication1.h"

#include<qfile.h>



QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    
    CSgetFrame();
    return;
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{}


void QtWidgetsApplication1::cstestConfigFilter() {
    AVFilterGraph* graph;
    graph = avfilter_graph_alloc();
    char graph_desc[5] = "null";
    AVFilterInOut* inputs, * outputs, * cur;
    AVFilterContext* inputFilter;
    avfilter_graph_parse2(graph, graph_desc, &inputs, &outputs);
    {
        char name[255];
        snprintf(name, sizeof(name), "graph %d input from stream %d:%d", 0,
            0, 0);
        AVFilterContext* last_filter;
        const AVFilter* buffer_filt = avfilter_get_by_name("buffer");
        AVBPrint args;
        av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
        av_bprintf(&args,
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:"
            "pixel_aspect=%d/%d",
            960, 540, 3,
            1, 25, 0, 1);
        av_bprintf(&args, ":frame_rate=%d/%d", 25, 1);

        AVBufferSrcParameters* par = av_buffersrc_parameters_alloc();

        if (!par)
            return;
        memset(par, 0, sizeof(*par));
        par->format = AV_PIX_FMT_NONE;
        par->hw_frames_ctx = NULL;
        avfilter_graph_create_filter(&last_filter, buffer_filt, name, args.str, NULL, graph);
        cstestInputFilter = last_filter;
        av_buffersrc_parameters_set(last_filter, par);
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(AV_PIX_FMT_BGR24);
        avfilter_link(last_filter, 0, inputs->filter_ctx, inputs->pad_idx);
        inputFilter = last_filter;
    }
    {
        char name[255];
        AVFilterContext* filter = NULL;
        AVFilterContext* last_filter = outputs->filter_ctx;
        snprintf(name, sizeof(name), "out_%d_%d", 0, 0);
        avfilter_graph_create_filter(&filter,
            avfilter_get_by_name("buffersink"),
            name, NULL, NULL, graph);
        cstestOutFilter = filter;
        {
            AVFilterContext* filter = NULL;
            avfilter_graph_create_filter(&filter,
                avfilter_get_by_name("format"),
                "format", "yuv420p", NULL, graph);
            avfilter_link(last_filter, 0, filter, 0);
            last_filter = filter;

        }
        avfilter_link(last_filter, 0, filter, 0);
    }
    {
        avfilter_graph_config(graph, NULL);

    }
}
void QtWidgetsApplication1::cstestEncodeCtx()
{
    filtered_frame = av_frame_alloc();
    const AVCodec* enc = avcodec_find_encoder(AV_CODEC_ID_H264);
    csTestEnc_ctx = avcodec_alloc_context3(enc);
    csTestEnc_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    // ost->enc_ctx = csTestEnc_ctx;
     //init_output_stream_encode(ost, filtered_frame);
    Cstestencode(csTestEnc_ctx, filtered_frame);
    //csTestEnc_ctx->flags =ost->enc_ctx->flags;;
    csTestEnc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //csTestEnc_ctx->export_side_data = ost->enc_ctx->export_side_data;
    avcodec_open2(csTestEnc_ctx, enc, NULL);
}
int QtWidgetsApplication1::Cstestencode(AVCodecContext* enc_ctx, AVFrame* frame) {

    enc_ctx->chroma_sample_location = AVCHROMA_LOC_UNSPECIFIED;
    enc_ctx->width = 960;
    enc_ctx->height = 540;
    enc_ctx->time_base = { 1, 30 };
    enc_ctx->sample_aspect_ratio = { 0, 1 };
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->bits_per_raw_sample = 0;
    if (frame) {
        enc_ctx->color_range = frame->color_range;
        enc_ctx->color_primaries = frame->color_primaries;
        enc_ctx->color_trc = frame->color_trc;
        enc_ctx->colorspace = frame->colorspace;
        enc_ctx->chroma_sample_location = frame->chroma_location;
    }
    enc_ctx->framerate ={ 30, 1 };;
    enc_ctx->field_order = AV_FIELD_PROGRESSIVE;
    return 0;
}
void QtWidgetsApplication1::cstest() {
    for (int i = 0; i < 1; i++) {

        cstestEncodeCtx();

        filtered_frame = av_frame_alloc();
        int ret = av_buffersink_get_frame_flags(cstestOutFilter, filtered_frame,
            AV_BUFFERSINK_FLAG_NO_REQUEST);
       
        
        ret = avcodec_send_frame(csTestEnc_ctx, filtered_frame);
        AVPacket* pkt = av_packet_alloc();;
        while ((ret = avcodec_receive_packet(csTestEnc_ctx, pkt)) == AVERROR(EAGAIN)) {
            ret = avcodec_send_frame(csTestEnc_ctx, NULL);
            if (ret < 0) {

               // exit_program(1);
            }
        }
        {
            FILE* ptr = fopen("firstCodePacket.dat", "wb");
            int wsize = fwrite(pkt->data, 1, pkt->size, ptr);
            fflush(ptr);
            fclose(ptr);
        }
        
        {
            uint8_t* mdatBuf = (uint8_t*)malloc(pkt->size * 2);
            memset(mdatBuf, 0, pkt->size * 2);
            uint8_t* temp = mdatBuf;
            int size = ff_avc_parse_nal_units(&mdatBuf, pkt->data, pkt->size);
            FILE* ptr = fopen("mdatCodePacket.dat", "wb");
            char vedioPacket[5] = { 0x17,01,00,00,00 };
            ret = fwrite(&vedioPacket, 1, 5, ptr);
            int wsize = fwrite(temp, 1, size, ptr);
            fflush(ptr);
            fclose(ptr);
        }
        av_log(NULL, AV_LOG_INFO,
            "bench: utime=%0.3fs stime=%0.3fs rtime=%0.3fs\n",
            1000000.0, 1000000.0, 1000000.0);
    }
}

void QtWidgetsApplication1::CSgetFrame()
{
    cstestConfigFilter();
    
    QStringList files;
    files.append("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-001.bmp");
    files.append("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-002.bmp");
    AVFormatContext* ic = avformat_alloc_context();
    ic->video_codec_id = AV_CODEC_ID_NONE;
    ic->audio_codec_id = AV_CODEC_ID_NONE;
    ic->subtitle_codec_id = AV_CODEC_ID_NONE;
    ic->data_codec_id = AV_CODEC_ID_NONE;
    ic->flags |= AVFMT_FLAG_NONBLOCK;
    //ic->interrupt_callback = int_cb;
    AVDictionary* format_opts = NULL;
    av_dict_set(&format_opts, "framerate", "25", 0);
    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    avformat_open_input(&ic, "D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-002.bmp", 0, &format_opts);
    avformat_find_stream_info(ic, NULL);
    AVPacket* pkt = av_packet_alloc();
    av_read_frame(ic, pkt);
    AVStream* st = ic->streams[0];
    AVCodecParameters* par = st->codecpar;
    const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext* dec_ctx = avcodec_alloc_context3(dec);
    AVDictionary* decoder_opts = NULL;
    av_dict_set(&decoder_opts, "threads", "auto", 0);
    avcodec_open2(dec_ctx, dec, &decoder_opts);
    avcodec_parameters_to_context(dec_ctx, par);
   
    for(int i=0;i<2;i++)
    {
        CSTestFrame = av_frame_alloc();
        FILE* file = fopen(files.at(i).toStdString().c_str(), "rb");
      //  FILE* file = fopen("D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-001.bmp", "rb");
        uint8_t* data = (uint8_t*)malloc(1555254);
        int rsize = fread(data, 1, 1555254, file);
        fclose(file);
        memcpy(pkt->data, data, 1555254);
        if (pkt) {
            int ret = avcodec_send_packet(dec_ctx, pkt);
            // In particular, we don't expect AVERROR(EAGAIN), because we read all
            // decoded frames with avcodec_receive_frame() until done.
            if (ret < 0 && ret != AVERROR_EOF)
                return;
        }

        avcodec_receive_frame(dec_ctx, CSTestFrame);


        av_buffersrc_add_frame_flags(cstestInputFilter, CSTestFrame, AV_BUFFERSRC_FLAG_PUSH);
        cstest();
      }
    
    
    

   
    
    return;
}
const uint8_t* QtWidgetsApplication1::avc_find_startcode_internal(const uint8_t* p, const uint8_t* end)
{
    const uint8_t* a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
        //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
        //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p + 1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p + 2;
                if (p[4] == 0 && p[5] == 1)
                    return p + 3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}
const uint8_t* QtWidgetsApplication1::ff_avc_find_startcode(const uint8_t* p, const uint8_t* end) {
    const uint8_t* out = avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1]) out--;
    return out;
}
void QtWidgetsApplication1::avio_w8(uint8_t** s, int b) {
    uint8_t* temp = *s;
    *temp = b;
    (*s)++;
}
void QtWidgetsApplication1::avio_wb32(uint8_t** s, unsigned int val)
{
    avio_w8(s, val >> 24);
    avio_w8(s, (uint8_t)(val >> 16));
    avio_w8(s, (uint8_t)(val >> 8));
    avio_w8(s, (uint8_t)val);
}
void QtWidgetsApplication1::avio_write(uint8_t** s, const unsigned char* buf, int size)
{
    uint8_t* temp = *s;
    memcpy(temp, buf, size);
    (*s) = (*s) + size;
}
int QtWidgetsApplication1::ff_avc_parse_nal_units(uint8_t** pb, const uint8_t* buf_in, int size)
{
    const uint8_t* p = buf_in;
    const uint8_t* end = p + size;
    const uint8_t* nal_start, * nal_end;

    size = 0;
    nal_start = ff_avc_find_startcode(p, end);
    for (;;) {
        while (nal_start < end && !*(nal_start++));
        if (nal_start == end)
            break;

        nal_end = ff_avc_find_startcode(nal_start, end);
        avio_wb32(pb, nal_end - nal_start);
        avio_write(pb, nal_start, nal_end - nal_start);
        size += 4 + nal_end - nal_start;
        nal_start = nal_end;
    }
    return size;
}