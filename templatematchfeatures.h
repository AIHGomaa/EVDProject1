#ifndef TEMPLATEMATCHFEATURES_H
#define TEMPLATEMATCHFEATURES_H

#include "opencv2/opencv.hpp"

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// and C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
class TemplateMatchFeatures
{
public:
    TemplateMatchFeatures();
    static const int UNDEFINED;
    double maxVal = UNDEFINED;
    cv::Point maxLoc = cv::Point(UNDEFINED, UNDEFINED);
    double scale = 1;
};

#endif // TEMPLATEMATCHFEATURES_H
