#include "sevensegmentgaugereader.h"
#include "mainwindow.h"

SevenSegmentGaugeReader::SevenSegmentGaugeReader()
{
    imageAnalizer = ImageAnalizer();
}

void SevenSegmentGaugeReader::EnhanceImage(Mat src, OutputArray dst)
{
    Mat grayScaled(src.rows, src.cols, CV_8UC1);
    Mat sizeScaled(IMG_SIZE, CV_8UC1);
    Mat adaptThreshold(IMG_SIZE, CV_8UC1);
    Mat blurred(IMG_SIZE, CV_8UC1);
    Mat filteredGaussian(IMG_SIZE, CV_8UC1);

    cvtColor(src, grayScaled, COLOR_RGB2GRAY);

    // Fixed input resolution, to make kernel sizes independent of scale.
    resize(grayScaled, sizeScaled, IMG_SIZE, 0, 0, INTER_LINEAR);

    // Blur is too strong on lowest resolution of Honeywell Dolphin. Maybe useful for higher resolutions

    blur(sizeScaled, blurred, Size(gaussianBlurSize, gaussianBlurSize));
    //GaussianBlur(sizeScaled, blurred, Size(gaussianBlurSize, gaussianBlurSize), 0, 0);  // remove small noise

    // Canny without adaptiveThreshold gives better result.

    // THRESH_BINARY_INV: Display segments are lighted. We need black segments.
    adaptiveThreshold(blurred, adaptThreshold, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, adaptiveThresholdBlockSize, adaptivethresholdC);

    //GaussianBlur(adaptThreshold, dst, Size(gaussianBlurSize, gaussianBlurSize), 0, 0);

    adaptThreshold.copyTo(dst);

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    //    imageAnalizer.showImage("EnhanceImage: src", src);
    //    imageAnalizer.showImage("EnhanceImage: grayScaled", grayScaled);
    //    imageAnalizer.showImage("EnhanceImage: blurred", blurred);
    //    imageAnalizer.showImage("EnhanceImage: adaptThreshold", adaptThreshold);
    //    imageAnalizer.showImage("EnhanceImage: filteredGaussian", filteredGaussian);
    //    imageAnalizer.showImage("EnhanceImage: dst", dst.getMat());

}

void SevenSegmentGaugeReader::SegmentImage(Mat src, OutputArray dst)
{
    Mat cannyEdges(IMG_SIZE, CV_8UC1, 1);
    Canny(src, cannyEdges, cannyThreshold1, cannyThreshold2, 3, true);


    Mat dilated;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(dilateKernelSize, dilateKernelSize));
    //Mat element = getStructuringElement(MORPH_RECT, Size(5, 5), Point(2, 2));
    //     morphologyEx(srcCanny,srcCanny, MORPH_CLOSE, kernel);
    dilate(cannyEdges, dilated, kernel);



    Mat labeledContours = Mat::zeros(IMG_SIZE, CV_8UC3);

    // Derived from https://docs.opencv.org/trunk/d6/d6e/group__imgproc__draw.html#ga746c0625f1781f1ffc9056259103edbc
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(cannyEdges, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE );

    vector<vector<Point> > contours_poly( contours.size() );
    vector<Rect> boundRect( contours.size() );

    // Derived from http://answers.opencv.org/question/19374/number-plate-segmentation-c/
    for( int i = 0; i < contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
    }

    int x=1;
    Mat srcWithRectangles;
    src.copyTo(srcWithRectangles);
    cvtColor(srcWithRectangles, srcWithRectangles, COLOR_GRAY2RGB);

    Mat drawing(srcWithRectangles);
    for( int i = 0; i< contours.size(); i++ )
    {
        Rect rect = boundRect[i];

        Scalar color = Scalar(0,255,0);
        int width = rect.width;
        int height = rect.height;
        //         int antal = contours[i].size();

        if(height>2 && height < 150 && width>1 && width < 150)
            //if(height>2 && width>1)
        {
            rectangle( drawing, rect.tl(), rect.br(), color, 2, 8, 0 );
            putText(drawing, to_string(1), boundRect[i].br(),FONT_HERSHEY_DUPLEX, 2, (0, 255, 255), 3);
            x++;
        }
    }



    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    //    int idx = 0;
    //    for( ; idx >= 0; idx = hierarchy[idx][0] )
    //    {
    //        Scalar color( rand()&255, rand()&255, rand()&255 );
    //        drawContours( labeledContours, contours, idx, color, FILLED, 8, hierarchy );
    //    }

    srcWithRectangles.copyTo(dst);

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("SegmentImage: src", src);
    imageAnalizer.showImage("SegmentImage: cannyEdges", cannyEdges);
    //    imageAnalizer.showImage("SegmentImage: labeledContours", labeledContours);
    imageAnalizer.showImage("SegmentImage: srcWithRectangles", srcWithRectangles);
}

ImageObject *SevenSegmentGaugeReader::ExtractFeatures(Mat src)
{
    throw Exception();
}

ReaderResult SevenSegmentGaugeReader::Classify(ImageObject *features)
{
    throw Exception();
}

ReaderResult SevenSegmentGaugeReader::ReadGaugeImage(Mat src)
{
    throw Exception();
}
