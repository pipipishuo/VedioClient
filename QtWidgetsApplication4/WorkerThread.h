#pragma once

#include <QThread>
#include<qimage.h>
#include<stdint.h>
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
}
class WorkerThread  : public QThread
{
	Q_OBJECT

public:

	WorkerThread(QObject *parent=NULL);
	~WorkerThread();
    uint32_t* data;
    uint32_t* tranData;
    uint8_t* srcdata;
    void read_thread();
    int width;
    int height;
    QString filename;
    AVCodecContext* videoAvctx;
    AVCodecContext* audioAvctx;
    int stream_component_open(AVFormatContext* ic, int stream_index, AVCodecContext** avctx1);
    int bmp_encode_frame(AVCodecContext* avctx, AVPacket* pkt,
        const AVFrame* pict, int* got_packet);
    void encodeImage();
signals:
    void sigImg(uint32_t*);
protected:
	virtual void run();
};
