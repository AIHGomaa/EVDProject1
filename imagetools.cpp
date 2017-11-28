#include "imagetools.h"
#include "QDebug"

using namespace cv;
using namespace std;


ImageTools::ImageTools()
{

}

int ImageTools::windowCounter = 0;

void ImageTools::showImage(const char *winname, const Mat img, int x, int y)
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

void ImageTools::showImage(const char *winname, const Mat img, int position)
{
    showImage(winname, img, (position % 4) * 200, (position / 4) * 100);
}

void ImageTools::showImage(const char *winname, const Mat img)
{
    showImage(winname, img, windowCounter++);
}

void ImageTools::resetNextWindowPosition()
{
    windowCounter = 0;
}

bool ImageTools::contourLeftToRightComparer(const vector<Point> a, const vector<Point> b)
{
    Rect ra(boundingRect(a));
    Rect rb(boundingRect(b));
    return (ra.x < rb.x);
}

// Derived from https://stackoverflow.com/questions/2114797/compute-median-of-values-stored-in-vector-c
double ImageTools::median(vector<double> collection)
{
    size_t size = collection.size();
    if (size == 0)
        return 0;
    sort(collection.begin(), collection.end());
    if (size  % 2 == 0)
        return (collection[size / 2 - 1] + collection[size / 2]) / 2;
    else
        return collection[size / 2];
}

// Derived from https://subokita.com/2013/06/18/simple-and-fast-gamma-correction-on-opencv/
Mat ImageTools::correctGamma(Mat& img, double gamma)
{
    Mat lut_matrix(1, 256, CV_8UC1);
    uchar * ptr = lut_matrix.ptr();
    for( int i = 0; i < 256; i++ )
        ptr[i] = (int)((pow((double)i / 255.0, gamma) * 255.0) + 0.5);

    Mat result;
    LUT(img, lut_matrix, result);
    return result;
}
