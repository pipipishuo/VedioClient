#include "QtWidgetsApplication1.h"

#include<qfile.h>
extern "C"{
#include"libavfilter/avfilter.h"
#include"libavutil/bprint.h"
#include"libavfilter/buffersrc.h"
#include"libavutil/pixdesc.h"
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavcodec/codec.h"
}


QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
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
            return ;
        memset(par, 0, sizeof(*par));
        par->format = AV_PIX_FMT_NONE;
        par->hw_frames_ctx = NULL;
        avfilter_graph_create_filter(&last_filter, buffer_filt, name, args.str, NULL, graph);
        av_buffersrc_parameters_set(last_filter, par);
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(AV_PIX_FMT_BGR24);
        avfilter_link(last_filter, 0, inputs->filter_ctx, inputs->pad_idx);
        inputFilter = last_filter;
    }
    {
        char name[255];
        AVFilterContext *filter=NULL;
        AVFilterContext* last_filter = outputs->filter_ctx;
        snprintf(name, sizeof(name), "out_%d_%d", 0, 0);
         avfilter_graph_create_filter(&filter,
            avfilter_get_by_name("buffersink"),
            name, NULL, NULL, graph);
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
        av_buffersrc_add_frame_flags(inputFilter, NULL, AV_BUFFERSRC_FLAG_PUSH);
    }
    getFrame();
    return;
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{}

void QtWidgetsApplication1::getFrame()
{
    AVFormatContext* ic = avformat_alloc_context();
    ic->video_codec_id =  AV_CODEC_ID_NONE;
    ic->audio_codec_id =  AV_CODEC_ID_NONE;
    ic->subtitle_codec_id =  AV_CODEC_ID_NONE;
    ic->data_codec_id =  AV_CODEC_ID_NONE;
    ic->flags |= AVFMT_FLAG_NONBLOCK;
    //ic->interrupt_callback = int_cb;
    AVDictionary* format_opts=NULL;
    av_dict_set(&format_opts, "framerate", "25", 0);
    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    avformat_open_input(&ic, "D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-001.bmp", 0, &format_opts);
    avformat_find_stream_info(ic, NULL);
    AVPacket* pkt= av_packet_alloc();
    av_read_frame(ic, pkt);



    AVStream* st = ic->streams[0];
    AVCodecParameters* par = st->codecpar;
    const AVCodec* dec=avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext*  dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx, par);
    if (pkt) {
        int ret=avcodec_send_packet(dec_ctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0 && ret != AVERROR_EOF)
            return ;
    }
    AVFrame* frame = av_frame_alloc();
    avcodec_receive_frame(dec_ctx, frame);
    return;
}
AVFrame* CSTestFrame;
AVFilterContext* cstestInputFilter;
AVFilterContext* cstestOutFilter;
void cstestConfigFilter() {
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
static void cstest() {
    for (int i = 0; i < nb_output_streams; i++) {
        OutputStream* ost = output_streams[i];
        ost->filter->filter;
        AVFrame* filtered_frame = av_frame_alloc();

        /*int ret = av_buffersink_get_frame_flags(ost->filter->filter, filtered_frame,
            AV_BUFFERSINK_FLAG_NO_REQUEST);*/
        int ret = av_buffersink_get_frame_flags(cstestOutFilter, filtered_frame,
            AV_BUFFERSINK_FLAG_NO_REQUEST);
        init_output_stream_wrapper(ost, filtered_frame, 1);
        ret = avcodec_send_frame(ost->enc_ctx, filtered_frame);
        AVPacket* pkt = ost->pkt;
        while ((ret = avcodec_receive_packet(ost->enc_ctx, pkt)) == AVERROR(EAGAIN)) {
            ret = avcodec_send_frame(ost->enc_ctx, NULL);
            if (ret < 0) {

                exit_program(1);
            }
        }
        {
            FILE* ptr = fopen("firstCodePacket.dat", "wb");
            int wsize = fwrite(pkt->data, 1, pkt->size, ptr);
            fflush(ptr);
            fclose(ptr);
        }
        av_log(NULL, AV_LOG_INFO,
            "bench: utime=%0.3fs stime=%0.3fs rtime=%0.3fs\n",
            1000000.0, 1000000.0, 1000000.0);
    }
}

void CSgetFrame()
{
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
    avformat_open_input(&ic, "D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/image-001.bmp", 0, &format_opts);
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
    if (pkt) {
        int ret = avcodec_send_packet(dec_ctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0 && ret != AVERROR_EOF)
            return;
    }
    CSTestFrame = av_frame_alloc();
    avcodec_receive_frame(dec_ctx, CSTestFrame);
    cstestConfigFilter();
    av_buffersrc_add_frame_flags(cstestInputFilter, CSTestFrame, AV_BUFFERSRC_FLAG_PUSH);
    cstest();
    return;
}