#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtWidgetsApplication1.h"
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
    AVFrame* CSTestFrame;
    AVFilterContext* cstestInputFilter;
    AVFilterContext* cstestOutFilter;
    AVCodecContext* csTestEnc_ctx;
    AVFrame* filtered_frame;
private:
    Ui::QtWidgetsApplication1Class ui;
};
