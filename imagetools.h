#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"

class ImageTools
{
public:
    ImageTools();
    static bool contourLeftToRightComparer(const std::vector<cv::Point> a, const std::vector<cv::Point> b);
    static double median(std::vector<double> collection);
    static cv::Mat correctGamma(cv::Mat &img, double gamma);
};

#endif // IMAGETOOLS_H
