#ifndef IGAUGEREADER_H
#define IGAUGEREADER_H

#include <QObject>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "imageobject.h"
#include "readerresult.h"

using namespace cv;

class IGaugeReader
{
public:
    IGaugeReader() {}
    // https://stackoverflow.com/questions/12854778/abstract-class-vs-interface-in-c:
    // virtual and = 0 makes function abstract.
//    virtual Mat EnhanceImage(Mat src) = 0;
    virtual void EnhanceImage(Mat src, OutputArray dst) = 0;
    virtual void SegmentImage(Mat src, OutputArray dst) = 0;
    virtual ImageObject* ExtractFeatures(Mat src) = 0;
    virtual ReaderResult Classify(ImageObject* features) = 0;
    virtual ReaderResult ReadGaugeImage(Mat src) = 0;
};

#endif // IGAUGEREADER_H
