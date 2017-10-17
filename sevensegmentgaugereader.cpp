#include "sevensegmentgaugereader.h"
#include "mainwindow.h"
#include <QMessageBox>

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
    Canny(src, cannyEdges, cannyThreshold1, cannyThreshold2, cannyAppertureSize, true);

    cannyEdges.copyTo(dst);

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("SegmentImage: src", src);
    imageAnalizer.showImage("SegmentImage: cannyEdges", cannyEdges);
}

ImageObject * SevenSegmentGaugeReader::ExtractFeatures(Mat src, Mat srcOriginal)
{
    //-----------------------------------
    // Correct rotation
    //-----------------------------------
    // Derived from http://felix.abecassis.me/2011/09/opencv-detect-skew-angle/
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(src, lines, houghDistanceResolution, houghAngleResolutionDegrees * CV_PI/180.0, houghVotesThreshold, houghMinLineLength, houghMaxLineGap);

    cv::Mat disp_lines(IMG_SIZE, CV_8UC1, cv::Scalar(0, 0, 0));
    double angleRad;
    unsigned nLines = lines.size();

    // RQ18: +/- 20 degrees must be corrected. We take +/-45 degrees.
    double maxRotationRad = 45 * CV_PI/180.0;
    vector<double> angles;
    for (unsigned i = 0; i < nLines; ++i)
    {
        angleRad = atan2((double)lines[i][3] - lines[i][1], (double)lines[i][2] - lines[i][0]);

        if (abs(angleRad) > maxRotationRad)
            continue;

        line(disp_lines, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), cv::Scalar(255, 0 ,0));
        angles.push_back (angleRad);
    }

    double medianAngleRad = median(angles);
    double medianAngleDegr = medianAngleRad * 180.0 / CV_PI;

    std::cout << "mean angle: " << medianAngleRad << "rad, " << medianAngleDegr << "degr" << std::endl;

    //    cv::destroyWindow(filename);

    Mat rotationCorrected(IMG_SIZE, CV_8UC1);
    //TODO: configurable
    if(abs(medianAngleDegr) > 0.5)
    {
        // Derived from https://stackoverflow.com/questions/2289690/opencv-how-to-rotate-iplimage
        Point2f centerPoint(src.cols/2.0F, src.rows/2.0F);
        Mat rotationMatrix = getRotationMatrix2D(centerPoint, medianAngleDegr, 1.0);
        warpAffine(src, rotationCorrected, rotationMatrix, src.size());
    }
    else
    {
        src.copyTo(rotationCorrected);
    }
    //-----------------------------------
    // Label segments
    //-----------------------------------
    Mat dilated;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(dilateKernelSize, dilateKernelSize));
    //Mat element = getStructuringElement(MORPH_RECT, Size(5, 5), Point(2, 2));
    //     morphologyEx(srcCanny,srcCanny, MORPH_CLOSE, kernel);
    dilate(rotationCorrected, dilated, kernel);


    //Mat labeledContours = Mat::zeros(IMG_SIZE, CV_8UC3);

    // Derived from https://docs.opencv.org/trunk/d6/d6e/group__imgproc__draw.html#ga746c0625f1781f1ffc9056259103edbc
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(dilated, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE );

    //Sort contours from left to right
    struct contour_sorter
    {
        bool operator ()(const vector<Point> a, const vector<Point> b)
        {
            Rect ra(boundingRect(a));
            Rect rb(boundingRect(b));
            return (ra.x < rb.x);
        }
    };
    sort(contours.begin(), contours.end(), contour_sorter());

    vector<vector<Point>> contours_poly( contours.size() );
    vector<Rect> boundRect( contours.size() );

    // Derived from http://answers.opencv.org/question/19374/number-plate-segmentation-c/
    for( int i = 0; i < contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
    }

    string number;
    Mat markedDigits;
    resize(srcOriginal, markedDigits, IMG_SIZE, 0, 0, INTER_LINEAR);

    vector<string> values { ".", "0", "1", "2", "3", "4", "5", "6", "7", "8","9" };

    Mat drawing(markedDigits);
    for( int i = 0; i< contours.size(); i++ )
    {
        Rect rect = boundRect[i];

        Scalar color = Scalar(0,255,0);
        int width = rect.width;
        int height = rect.height;


        if(height>2 && height < 150 && width>1 && width < 150)
            //if(height>2 && width>1)
        {
            Mat imageroi = markedDigits(rect);
            Mat roi, sample;
            resize(imageroi, roi, Size(30, 40));
            roi.reshape(1, 1).convertTo(sample, CV_32F);

            Mat results;
            float result = kNearest->findNearest(sample, kNearest->getDefaultK(), results);
            int val = (int)result;
            string charValue = to_string(val);
            if (val == 10) {
                charValue = ".";
            }

            bool found = find(values.begin(), values.end(), charValue) != values.end();
            if (found == true)
            {
                rectangle( drawing, rect.tl(), rect.br(), color, 2, 8, 0 );
                color = Scalar(255,255,255);
                putText(drawing, charValue, rect.br(), FONT_HERSHEY_DUPLEX, 1, color, 3);
                //final number
                number += charValue;
            }
        }
    }
    //Show final number
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(number));
    msgBox.exec();


    // iterate through all the top-level contours,
    // draw each connected component with its own random color
    //    int idx = 0;
    //    for( ; idx >= 0; idx = hierarchy[idx][0] )
    //    {
    //        Scalar color( rand()&255, rand()&255, rand()&255 );
    //        drawContours( labeledContours, contours, idx, color, FILLED, 8, hierarchy );
    //    }


    ImageObject * result = new ImageObject();

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("disp_lines", disp_lines);
    imageAnalizer.showImage("rotationCorrected", rotationCorrected);
        imageAnalizer.showImage("SegmentImage: dilated", dilated);
    //    imageAnalizer.showImage("SegmentImage: labeledContours", labeledContours);
    imageAnalizer.showImage("SegmentImage: markedDigits", markedDigits);

    return result;
}
//Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();
bool SevenSegmentGaugeReader::loadKNNDataAndTrainKNN(){
    Mat samples, responses;
   for (int i = 0; i < 21; i++) {
        string path;
        if (i >= 10) {
            switch (i)
            {
            case 11:
                path = imageDir + "ImagesNumber\\BottomArrow.png";
                break;
            case 12:
                path = imageDir + "ImagesNumber\\ESC.png";
                break;
            case 13:
                path = imageDir + "ImagesNumber\\KG.png";
                break;
            case 14:
                path = imageDir + "ImagesNumber\\L2.png";
                break;
            case 15:
                path = imageDir + "ImagesNumber\\L3.png";
                break;
            case 16:
                path = imageDir + "ImagesNumber\\L4.png";
                break;
            case 17:
                path = imageDir + "ImagesNumber\\LeftArrow.png";
                break;
            case 18:
                path = imageDir + "ImagesNumber\\LeftArrow2.png";
                break;
            case 19:
                path = imageDir + "ImagesNumber\\RedSquares.png";
                break;
            case 20:
                path = imageDir + "ImagesNumber\\RightArrow.png";
                break;
            default:
                path = imageDir + "ImagesNumber\\punt.png";
                break;
            }
        }
        else
        {
            path = imageDir + "ImagesNumber\\" + to_string(i) + ".png";
        }
        Mat img = imread(path);
        responses.push_back(Mat(1, 1, CV_32F, i));

        Mat roi, sample;
        resize(img, roi, Size(30, 40));
        roi.reshape(1, 1).convertTo(sample, CV_32F);
        samples.push_back(sample);
    }
    kNearest->setDefaultK(1);
    kNearest->train(samples, ml::ROW_SAMPLE, responses);

    return true;
}

// Derived from https://stackoverflow.com/questions/2114797/compute-median-of-values-stored-in-vector-c
double SevenSegmentGaugeReader::median(vector<double> collection)
{
    size_t size = collection.size();
    sort(collection.begin(), collection.end());
    if (size  % 2 == 0)
        return (collection[size / 2 - 1] + collection[size / 2]) / 2;
    else
        return collection[size / 2];
}

ReaderResult SevenSegmentGaugeReader::Classify(ImageObject *features)
{
    throw Exception();
}

ReaderResult SevenSegmentGaugeReader::ReadGaugeImage(Mat src)
{
    throw Exception();
}
