#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtWidgetsApplication1.h"
#include<stdint.h>
extern "C" {
#include"libavfilter/avfilter.h"
#include"libavutil/bprint.h"
#include"libavfilter/buffersrc.h"
#include"libavutil/pixdesc.h"
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavcodec/codec.h"
#include"libavfilter/buffersink.h"
#include"libavfilter/buffersrc.h"
}

class QtWidgetsApplication1 : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsApplication1(QWidget *parent = nullptr);
    ~QtWidgetsApplication1();
    void cstestConfigFilter();
    void cstestEncodeCtx();
    int Cstestencode(AVCodecContext* enc_ctx, AVFrame* frame);
    void cstest();
    void CSgetFrame();
    int ff_avc_parse_nal_units(uint8_t** pb, const uint8_t* buf_in, int size);
    void avio_write(uint8_t** s, const unsigned char* buf, int size);
    void avio_wb32(uint8_t** s, unsigned int val);
    void avio_w8(uint8_t** s, int b);
    const uint8_t* ff_avc_find_startcode(const uint8_t* p, const uint8_t* end);
    const uint8_t* avc_find_startcode_internal(const uint8_t* p, const uint8_t* end);
    AVFrame* CSTestFrame;
    AVFilterContext* cstestInputFilter;
    AVFilterContext* cstestOutFilter;
    AVCodecContext* csTestEnc_ctx;
    AVFrame* filtered_frame;
private:
    Ui::QtWidgetsApplication1Class ui;
};
