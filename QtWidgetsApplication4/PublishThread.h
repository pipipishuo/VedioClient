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

class PublishThread : public QThread
{
	Q_OBJECT
public:
	
protected:
	virtual void run();
};

