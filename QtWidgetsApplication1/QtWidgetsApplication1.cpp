#include "QtWidgetsApplication1.h"

#include<qfile.h>
QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    
    char name[2500] = { 0 };
    char ffmpeg[8192] = { 0 };
    int sampleCount = 0;
    sprintf(name, "D:/kylinv10/ffmpeg_vs2019/ffmpeg_vs2019/msvc/bin/x64/frameData/ffmpegSWR.dat", sampleCount);
    sampleCount++;
    FILE* fileptr = fopen(name, "rb");
    int Read_Size = fread(ffmpeg,1, 8192, fileptr);
    fflush(fileptr);
    fclose(fileptr);


    char chen[8192] = { 0 };
    
    sprintf(name, "C:/Users/12891/source/repos/QtWidgetsApplication4/QtWidgetsApplication4/frameData/vedioData.dat", sampleCount);
    sampleCount++;
    fileptr = fopen(name, "rb");
    Read_Size = fread(chen, 1, 8192, fileptr);
    fflush(fileptr);
    fclose(fileptr);

    for (int i = 0; i < 8192; i++) {
        if (ffmpeg[i] != chen[i]) {
            char fff = ffmpeg[i];
            char ce = chen[i];
            continue;
        }
    }
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{}
