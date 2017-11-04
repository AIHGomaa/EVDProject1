#include "imageanalizer.h"
#include "QDebug"

ImageAnalizer::ImageAnalizer()
{

}

void ImageAnalizer::showImage(const char *winname, const Mat img, int x, int y)
{
    if (img.cols == 0 || img.rows == 0)
    {
        qDebug() << "ImageAnalizer::showImage(): image width or height is 0." << winname;
        return;
    }
    namedWindow(winname, CV_WINDOW_AUTOSIZE);   // OpenCV doc: If a window with the same name already exists, the function does nothing.
    moveWindow(winname, x, y);
    imshow(winname, img);
}

void ImageAnalizer::showImage(const char *winname, const Mat img, int position)
{
    showImage(winname, img, (position % 4) * 200, (position / 4) * 100);
}

void ImageAnalizer::showImage(const char *winname, const Mat img)
{
    showImage(winname, img, windowCounter++);
}

void ImageAnalizer::resetNextWindowPosition()
{
    windowCounter = 0;
}
