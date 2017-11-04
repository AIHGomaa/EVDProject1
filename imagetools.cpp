#include "imagetools.h"

using namespace cv;
using namespace std;

ImageTools::ImageTools()
{

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
    sort(collection.begin(), collection.end());
    if (size  % 2 == 0)
        return (collection[size / 2 - 1] + collection[size / 2]) / 2;
    else
        return collection[size / 2];
}

// From https://subokita.com/2013/06/18/simple-and-fast-gamma-correction-on-opencv/
Mat ImageTools::correctGamma(Mat& img, double gamma)
{
    double inverse_gamma = 1.0 / gamma;

    Mat lut_matrix(1, 256, CV_8UC1 );
    uchar * ptr = lut_matrix.ptr();
    for( int i = 0; i < 256; i++ )
        ptr[i] = (int)(pow((double) i / 255.0, inverse_gamma) * 255.0);

    Mat result;
    LUT(img, lut_matrix, result);

    return result;
}
