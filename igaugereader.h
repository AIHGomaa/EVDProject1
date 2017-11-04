#ifndef IGAUGEREADER_H
#define IGAUGEREADER_H

#include <QObject>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "readerresult.h"

using namespace cv;

class IGaugeReader
{
public:
    IGaugeReader() {}
    virtual ReaderResult ReadGaugeImage(Mat src) = 0;
    virtual Mat getMarkedImage() = 0;
};

#endif // IGAUGEREADER_H
