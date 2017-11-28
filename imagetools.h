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
    static int mean(cv::Mat *img);
    static cv::Mat correctGamma(cv::Mat &img, double gamma);
    static void showImage(const char *winname, const cv::Mat tex, int x, int y);
    static void showImage(const char *winname, const cv::Mat tex, int position);
    static void showImage(const char *winname, const cv::Mat img);
    static void resetNextWindowPosition();
private:
    static int windowCounter;
};

#endif // IMAGETOOLS_H
