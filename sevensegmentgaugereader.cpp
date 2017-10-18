#include "sevensegmentgaugereader.h"
#include "mainwindow.h"
#include "QMessageBox"
#include "QDebug"

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
    imageAnalizer.showImage("EnhanceImage: src", src);
    imageAnalizer.showImage("EnhanceImage: grayScaled", grayScaled);
    imageAnalizer.showImage("EnhanceImage: blurred", blurred);
    imageAnalizer.showImage("EnhanceImage: adaptThreshold", adaptThreshold);
    imageAnalizer.showImage("EnhanceImage: filteredGaussian", filteredGaussian);
    imageAnalizer.showImage("EnhanceImage: dst", dst.getMat());

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

    // R`18: +/- 20 degrees must be corrected. We take +/-45 degrees.
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

        //TODO: rotate earlier version of the image and repeat segmentation (and enhancement) for better contours search
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


    Size digitSize = calculateDigitSize(dilated);


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
    for( uint i = 0; i < contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
    }

    string number;
    Mat markedDigits;
    resize(srcOriginal, markedDigits, IMG_SIZE, 0, 0, INTER_LINEAR);

    vector<string> values { ".", "0", "1", "2", "3", "4", "5", "6", "7", "8","9" };

    Mat drawing(markedDigits);
    for( uint i = 0; i< contours.size(); i++ )
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
bool SevenSegmentGaugeReader::loadKNNDataAndTrainKNN() {
    Mat samples;
    vector<int> responseLabels;
    for (int i = 0; i < 21; i++) {
        string path;
        if (i >= 10) {
            switch (i)
            {
            case 11:
                path = referenceImageDir + "BottomArrow.png";
                break;
            case 12:
                path = referenceImageDir + "ESC.png";
                break;
            case 13:
                path = referenceImageDir + "KG.png";
                break;
            case 14:
                path = referenceImageDir + "L2.png";
                break;
            case 15:
                path = referenceImageDir + "L3.png";
                break;
            case 16:
                path = referenceImageDir + "L4.png";
                break;
            case 17:
                path = referenceImageDir + "LeftArrow.png";
                break;
            case 18:
                path = referenceImageDir + "LeftArrow2.png";
                break;
            case 19:
                path = referenceImageDir + "RedSquares.png";
                break;
            case 20:
                path = referenceImageDir + "RightArrow.png";
                break;
            default:
                path = referenceImageDir + "Point.png";
                break;
            }
        }
        else
        {
            path = referenceImageDir + to_string(i) + ".png";
        }
        Mat img = imread(path);
        responseLabels.push_back(i);

        // convert to single row matrix
        Mat roi, sample;
        resize(img, roi, Size(30, 40));
        roi.reshape(1, 1).convertTo(sample, CV_32F);
        samples.push_back(sample);
    }
    kNearest->setDefaultK(1);
    kNearest->train(samples, ml::ROW_SAMPLE, responseLabels);

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("samples", samples);

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

Mat SevenSegmentGaugeReader::loadReferenceImage(string fileName)
{
    Mat dst = imread(referenceImageDir + fileName, CV_LOAD_IMAGE_GRAYSCALE); // _COLOR);
    if(!dst.data)
        throw Exception(0, "Could not load image", "loadReferenceImage", "SevenSegmentGaugeReader.cpp", 0);

    return dst;
}

Size SevenSegmentGaugeReader::calculateDigitSize(Mat img)
{
    return Size(0, 0);

    // TODO: performance


    // Only for test
    int maskSizeTestCounter = 0;
    int imageRowTestCounter = 0;
    int imageColTestCounter = 0;
    int maskRowTestCounter = 0;
    int maskColTestCounter = 0;
    //

    Mat maskDigit8pSrc = loadReferenceImage("MaskDigit8_SegmentP.png");
    Mat maskSegmentBcSrc = loadReferenceImage("MaskSegmentBC.png");
    Mat maskSegmentAgdSrc = loadReferenceImage("MaskSegmentAGD.png");
    Mat maskDigit8p, maskSegmentBc, maskSegmentAgd;

    double outsideMaskTolerance = 0.1;
    double misingInMaskTolerance = 0.1;

    int maskHStepSize = 1;
    double maskAspectRatio = maskDigit8pSrc.cols / (double)maskDigit8pSrc.rows;

    //TODO: values based on requirements
    int minMaskH = (int)(img.rows / 24.0 + 0.5);
    int maxMaskH = (int)(img.rows / 16.0 + 0.5);

    qDebug() << "maskAspectRatio " << maskAspectRatio;
    qDebug() << "minMaskH " << minMaskH;
    qDebug() << "maxMaskH " << maxMaskH;

    int maskW, maskH;
    bool isMatch = false;

    //TODO: start with most likely size and most likely position
    // Iterate over mask sizes
    for (maskH = minMaskH; maskH < maxMaskH && !isMatch; maskH += maskHStepSize) {
        // Only for test
        maskSizeTestCounter++;
        //

        maskW = (int)(maskH * maskAspectRatio + 0.5);

        qDebug() << "maskW " << maskW;
        qDebug() << "maskH " << maskH;

        resize(maskDigit8pSrc, maskDigit8p, Size(maskW, maskH));
        resize(maskSegmentBcSrc, maskSegmentBc, Size(maskW, maskH));
        resize(maskSegmentAgdSrc, maskSegmentAgd, Size(maskW, maskH));

        // TODO: configurable
        int colStart = (int)(img.cols * 1 / 8.0 + 0.5);
        int colEnd = img.cols - colStart - maskW;
        int rowStart = (int)(img.rows * 1 / 3.0 + 0.5);
        int rowEnd = img.rows - rowStart - maskH;
        int maskMoveStepSize = 1;
        int samplingStepSize = 1;

        qDebug() << "colStart " << colStart;
        qDebug() << "colEnd " << colEnd;
        qDebug() << "rowStart " << rowStart;
        qDebug() << "rowEnd " << rowEnd;


        int objectPixelsOutsideMask = 0;
        int objectPixelsMissingInMask = 0;
        int maxObjectPixelsOutsideMask = (int)(maskW * maskH * outsideMaskTolerance + 0.5);
        int maxObjectPixelsMissingInMask = (int)(maskW * maskH * misingInMaskTolerance + 0.5);

        // Iterate over image rows and cols
        for (int imgRow = rowStart; imgRow < rowEnd && !isMatch; imgRow += maskMoveStepSize)
        {
            // Only for test
            imageRowTestCounter++;
            //

            for (int imgCol = colStart; imgCol < colEnd && !isMatch; imgCol++)
            {
                // Only for test
                imageColTestCounter++;
                //

                bool canMatch = true;
                // iterate over mask rows and cols
                for (int maskRow = 0; maskRow < maskH && canMatch; maskRow += samplingStepSize)
                {
                    // Only for test
                    maskRowTestCounter++;
                    qDebug() << "maskW " << maskW;
                    qDebug() << "maskH " << maskH;
                    //

                    for (int maskCol = 0; maskCol < maskW && canMatch; maskCol += samplingStepSize)
                    {
                        // Only for test
                        maskColTestCounter++;
                        qDebug() << "maskW " << maskW;
                        qDebug() << "maskH " << maskH;
                        //

                        uchar imgPixel = maskDigit8pSrc.at<uchar>(imgRow + maskRow, imgCol + maskCol);
                        uchar maskDigit8pPixel = maskDigit8pSrc.at<uchar>(maskRow, maskCol);
                        uchar maskSegmentBcPixel = maskSegmentBcSrc.at<uchar>(maskRow, maskCol);
                        uchar maskSegmentAgdPixel = maskSegmentAgdSrc.at<uchar>(maskRow, maskCol);
                        // Pixels outside mask pattern shoud be 0.
                        if ((imgPixel & maskDigit8pPixel) != imgPixel)
                            objectPixelsOutsideMask++;
                        // Pixels within mask pattern should entirely fill segment B and C or segment A and G and D,
                        // because 7-segment digits 0...9 match at least 1 of these masks.
                        else if ((imgPixel & maskSegmentBcPixel) != maskSegmentBcPixel &&
                                 (imgPixel & maskSegmentAgdPixel) != maskSegmentAgdPixel)
                            objectPixelsMissingInMask++;

                        if (objectPixelsOutsideMask > maxObjectPixelsOutsideMask ||
                                objectPixelsMissingInMask > maxObjectPixelsMissingInMask)
                            canMatch = false;
                    }
                }
                isMatch = canMatch;
            }
        }
    }

    // TEST
    qDebug() << "maskSizeTestCounter: " << maskSizeTestCounter;
    qDebug() << "imageRowTestCounter: " << imageRowTestCounter;
    qDebug() << "imageColTestCounter: " << imageColTestCounter;
    qDebug() << "maskRowTestCounter: " << maskRowTestCounter;
    qDebug() << "maskColTestCounter: " << maskColTestCounter;

    if (isMatch)
    {
        qDebug() << "digit size: " << maskW << "*" << maskH;

        return Size(maskW, maskH);
    }
    else
    {
        qDebug() << "No digits recognized";

        return Size(0, 0);
        //throw Exception(0, "No digits recognized", "calculateDigitSize", "SevenSegmentGaugeReader.cpp", 0);
    }
}
