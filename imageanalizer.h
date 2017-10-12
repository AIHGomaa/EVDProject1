#ifndef IIMAGEANALIZER_H
#define IIMAGEANALIZER_H

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"

using namespace cv;

class ImageAnalizer
{
public:
    ImageAnalizer();
    void showImage(const char *winname, const Mat tex, int x, int y);
    void showImage(const char *winname, const Mat tex, int position);
    void showImage(const char *winname, const Mat img);
    void resetNextWindowPosition();
private:
    int windowCounter;
};

#endif // IIMAGEANALIZER_H
