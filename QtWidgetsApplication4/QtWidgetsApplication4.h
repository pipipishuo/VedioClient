#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtWidgetsApplication4.h"
#include<qimage.h>
#include<stdint.h>
extern "C"
{
#include"libavcodec/avcodec.h"
#include"libswscale/swscale.h"
#include "libavformat/avformat.h"
}
class QtWidgetsApplication4 : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsApplication4(QWidget *parent = nullptr);
    ~QtWidgetsApplication4();
    uint32_t* data;
    uint32_t* tranData;
    void read_thread();
    int width ;
    int height;
    QString filename;
    AVCodecContext* avctx;
    int stream_component_open(AVFormatContext* ic, int stream_index);
    int bmp_encode_frame(AVCodecContext* avctx, AVPacket* pkt,
        const AVFrame* pict, int* got_packet);
protected:
    void paintEvent(QPaintEvent* event);
private:
    Ui::QtWidgetsApplication4Class ui;
};
