#include "sevensegmentgaugereader.h"
#include "mainwindow.h"
#include "QMessageBox"
#include "QDebug"

SevenSegmentGaugeReader::SevenSegmentGaugeReader()
{
    imageAnalizer = ImageAnalizer();
}

void SevenSegmentGaugeReader::EnhanceImage(Mat src, OutputArray dst, OutputArray srcScaled)
{
    Mat grayScaled(src.rows, src.cols, CV_8UC1);
    //    Mat sizeScaled;
    Mat adaptThreshold(IMG_SIZE, CV_8UC1);
    Mat blurred(IMG_SIZE, CV_8UC1);
    //    Mat filteredGaussian(IMG_SIZE, CV_8UC1);
    
    // Fixed input resolution, to make kernel sizes independent of scale.
    resize(src, srcScaled, IMG_SIZE, 0, 0, INTER_LINEAR);

    cvtColor(srcScaled, grayScaled, COLOR_RGB2GRAY);
    imageAnalizer.resetNextWindowPosition();
    //    imageAnalizer.showImage("sizeScaled", sizeScaled);
    
    // Blur is too strong on lowest resolution of Honeywell Dolphin. Maybe useful for higher resolutions
    
    //    blur(grayScaled, blurred, Size(gaussianBlurSize, gaussianBlurSize));
    GaussianBlur(grayScaled, blurred, Size(gaussianBlurSize, gaussianBlurSize), 0, 0);  // remove small noise
    
    // Canny without adaptiveThreshold gives better result.
    
    // THRESH_BINARY_INV: Display segments are lighted. We need black segments.
    adaptiveThreshold(blurred, adaptThreshold, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, adaptiveThresholdBlockSize, adaptivethresholdC);
    
    //GaussianBlur(adaptThreshold, dst, Size(gaussianBlurSize, gaussianBlurSize), 0, 0);
    
    adaptThreshold.copyTo(dst);
    
    // Only for test
    //    imageAnalizer.resetNextWindowPosition();
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
    //     imageAnalizer.resetNextWindowPosition();
    //     imageAnalizer.showImage("SegmentImage: src", src);
    //     imageAnalizer.showImage("SegmentImage: cannyEdges", cannyEdges);
}

double SevenSegmentGaugeReader::calculateRotationDegrees(Mat edges)
{
    // Derived from http://felix.abecassis.me/2011/09/opencv-detect-skew-angle/
    vector<cv::Vec4i> hou;
    cv::HoughLinesP(edges, hou, houghDistanceResolution, houghAngleResolutionDegrees * CV_PI/180.0, houghVotesThreshold, houghMinLineLength, houghMaxLineGap);

    cv::Mat disp_lines(IMG_SIZE, CV_8UC1, cv::Scalar(0, 0, 0));
    double angleRad;
    unsigned nLines = hou.size();

    // RQ18: +/- 20 degrees must be corrected. We take +/-45 degrees.
    double maxRotationRad = 45 * CV_PI/180.0;
    vector<double> angles;
    for (unsigned i = 0; i < nLines; ++i)
    {
        angleRad = atan2((double)hou[i][3] - hou[i][1], (double)hou[i][2] - hou[i][0]);
        if (abs(angleRad) > maxRotationRad)
            continue;

        line(disp_lines, Point(hou[i][0], hou[i][1]), Point(hou[i][2], hou[i][3]), cv::Scalar(255, 0 ,0));
        angles.push_back (angleRad);
    }

    double medianAngleRad = median(angles);
    double medianAngleDegr = medianAngleRad * 180.0 / CV_PI;

    std::cout << "mean angle: " << medianAngleRad << "rad, " << medianAngleDegr << "degr" << std::endl;

    if (showImageFlags & SHOW_ROTATION_CORRECTION_FLAG)
    {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("Feature extract: disp_lines", disp_lines);
    }

    return medianAngleDegr;
}

void SevenSegmentGaugeReader::correctRotation(double rotationDegrees, Mat srcColor, Mat srcGrayScale, OutputArray dstColor, OutputArray dstGrayScale)
{
    // Derived from https://stackoverflow.com/questions/2289690/opencv-how-to-rotate-iplimage
    Point2f centerPoint(srcGrayScale.cols/2.0F, srcGrayScale.rows/2.0F);
    Mat rotationMatrix = getRotationMatrix2D(centerPoint, rotationDegrees, 1.0);
    Mat rotationCorrected(IMG_SIZE, CV_8UC1);

    // White border values
    warpAffine(srcGrayScale, rotationCorrected, rotationMatrix, srcGrayScale.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(255));
    warpAffine(srcColor, dstColor, rotationMatrix, srcGrayScale.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(255, 255, 255));

    Mat thresAfterRotate;
    // High threshold for thicker digit segments.
    threshold(rotationCorrected, thresAfterRotate, 191, 255, THRESH_BINARY);

    Mat morphKernel = getStructuringElement(MORPH_RECT, Size(dilateKernelSize, dilateKernelSize));
    Mat reEnhancedAfterWarp(IMG_SIZE, CV_8UC1);
    // Mat morphKernel = getStructuringElement(MORPH_RECT, Size(5, 5), Point(2, 2));
    // morphologyEx(srcCanny, srcCanny, MORPH_CLOSE, morphKernel);
    morphologyEx(thresAfterRotate, reEnhancedAfterWarp, MORPH_OPEN, morphKernel);

    if (showImageFlags & SHOW_ROTATION_CORRECTION_FLAG)
    {
        imageAnalizer.showImage("Feature extract: rotationCorrected", rotationCorrected);
        imageAnalizer.showImage("Feature extract: thresAfterRotate", thresAfterRotate);
        imageAnalizer.showImage("Feature extract: reEnhancedAfterWarp", reEnhancedAfterWarp);
    }
    reEnhancedAfterWarp.copyTo(dstGrayScale);
}

DigitFeatures * SevenSegmentGaugeReader::ExtractFeatures(Mat edges, Mat enhancedImage, Mat colorImage)
{
    double rotationDegrees = calculateRotationDegrees(edges);

    //TODO: configurable
    //TODO: in EnhanceImage?
    if(abs(rotationDegrees) > 0.5)
        correctRotation(rotationDegrees, colorImage, enhancedImage, colorImage, enhancedImage);

    // Extract features for one reference digit, to determine the desired image scale.
    // This is necessary for accuracy of findContours(),
    // because we optimize our kernels for a fixed digit size.
    DigitFeatures referenceDigitFeatures = extractReferenceDigitFeaturesByMultiScaleTemplateMatch(enhancedImage);

    qDebug() << "referenceDigitFeatures" << referenceDigitFeatures.width << "*" << referenceDigitFeatures.height << referenceDigitFeatures.bottomY;

    // Resize image to get desired digit size
    double imageScaleFactor = DIGIT_TEMPLATE_Y_RESOLUTION / (double)referenceDigitFeatures.height;
    Mat enhancedResized;
    Mat colorResized;
    resize(enhancedImage, enhancedResized, Size(0, 0), imageScaleFactor, imageScaleFactor, INTER_LINEAR);
    resize(colorImage, colorResized, Size(0, 0), imageScaleFactor, imageScaleFactor, INTER_LINEAR);
    threshold(enhancedResized, enhancedResized, 127, 255, THRESH_BINARY);

    qDebug() << "imageScaleFactor" << imageScaleFactor << "enhancedImage.rows" << enhancedImage.rows << "enhancedResized.rows" << enhancedResized.rows;

    // Use kernel height > kernel width,
    // to remove gaps between segments for findContours() but keep decimal point separated from nearest digit.
    Mat dilateKernel = getStructuringElement(MORPH_RECT, Size(digitDilateKernelWidth, digitDilateKernelHeight));
    Mat enhancedResizedInverted;
    bitwise_not(enhancedResized, enhancedResizedInverted);
    Mat dilatedEnhancedInverted;
    dilate(enhancedResizedInverted, dilatedEnhancedInverted, dilateKernel);

    // The feature extraction

    // Derived from https://docs.opencv.org/trunk/d6/d6e/group__imgproc__draw.html#ga746c0625f1781f1ffc9056259103edbc
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // Edges must be white, background must be black.
    //TODO: minimize search by roi in image, based on requirements. (offset parameter must be used).
    findContours(dilatedEnhancedInverted, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    //    findContours(dilatedEnhancedInverted, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

    qDebug() << "contours.size() " << contours.size();
    
    //TODO: refactor
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
    for( uint i = 0; i < contours.size(); i++ ) {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
    }

    Mat colorResizedFiltered;
    string number;

    // Option 1
    //    markedDigits = berekenDigitAlgorithm(dilatedEnhancedInverted, contours);

    // Option 3: K-nearest
    //TODO: optimalisation: use grayscale image instead? Then we also need grayscale samples.
    //        srcResized.copyTo(markedDigits);

    // Option 4
    //classifyDigitsByTemplateMatching(enhancedImage, referenceDigitFeatures);

    //TODO: in classification
    // Filter color image by binary image.
    colorResized.copyTo(colorResizedFiltered, enhancedResizedInverted);

    Mat markedDigits;
    colorResized.copyTo(markedDigits);
    Mat drawing(markedDigits);

    vector<string> values { ".", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

    //TODO: configurable in ui
    //    double digitSizeTolerance = 0.5;
    double digitYTolerance = 0.03;
    //TOOD: use imageScaleFactor for size
    //    Size minDigitSize = Size(digitInfo.width * (1 - digitSizeTolerance) + 0.5, digitInfo.height * (1 - digitSizeTolerance) + 0.5);
    //    Size maxDigitSize = Size(digitInfo.width * (1 + digitSizeTolerance) + 0.5, digitInfo.height * (1 + digitSizeTolerance) + 0.5);
    int minDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 - digitYTolerance) + 0.5;
    int maxDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 + digitYTolerance) + 0.5;

    Scalar contourColor = Scalar(0,255,0);
    Scalar textColor = Scalar(255,255,255);

    for(uint i = 0; i < boundRect.size(); i++) {
        Rect rect = boundRect[i];
        
        //        int width = rect.width;
        //        int height = rect.height;

        if( //height >= minDigitSize.height && height <= maxDigitSize.height &&
                //width>= minDigitSize.width && width <= maxDigitSize.width)
                rect.br().y >= minDigitBottomY &&
                rect.br().y <= maxDigitBottomY) {
            Mat imageroi = colorResizedFiltered(rect);
            imageroi.convertTo(imageroi, CV_32F);
            Mat roi, sample;
            resize(imageroi, roi, DIGIT_TEMPLATE_SIZE);
            roi.reshape(1, 1).convertTo(sample, CV_32F);

            qDebug() << "testsample.type" << sample.type();

            // Classification
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
                rectangle( drawing, rect.tl(), rect.br(), contourColor, 2, LINE_4, 0 );
                putText(drawing, charValue, Point(rect.br().x - DIGIT_TEMPLATE_SIZE.width, referenceDigitFeatures.bottomY * imageScaleFactor + DIGIT_TEMPLATE_SIZE.height * 0.5), FONT_HERSHEY_DUPLEX, 1.5, textColor, 3);
                //final number
                number += charValue;
            }
        }
    }
    //Show final number
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(number));
    msgBox.exec();

    if (showImageFlags & SHOW_FEATURE_EXTRACTION_FLAG)
    {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("Feature extract: enhancedImage", enhancedImage);
//        imageAnalizer.showImage("Feature extract: edges dilated", dilatedEdges);
        imageAnalizer.showImage("Feature extract: enhancedResizedInverted", enhancedResizedInverted);
        imageAnalizer.showImage("Feature extract: dilatedEnhancedInverted", dilatedEnhancedInverted);
        imageAnalizer.showImage("Feature extract: colorResized", colorResized);
        imageAnalizer.showImage("Feature extract: colorResizedFiltered", colorResizedFiltered);
        imageAnalizer.showImage("Feature extract: markedDigits", markedDigits);
    }

    //TODO
    DigitFeatures * result = new DigitFeatures(0, 0, 0, 0);

    return result;
}

//TODO: Save/load training data: knn->save("my.yml") and knn->load("my.yml")
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
                path = referenceImageDir + "Point_Color.png";
                break;
            }
        }
        else
        {
            path = referenceImageDir + "Digit" + to_string(i) + "_Color.png";
        }
        Mat img = imread(path);
        responseLabels.push_back(i);

        // First convert to desired template size, next to single row matrix.
        Mat roi, sample;
        resize(img, roi, DIGIT_TEMPLATE_SIZE);
        roi.reshape(1, 1).convertTo(sample, CV_32F);
        samples.push_back(sample);
    }
    kNearest->setDefaultK(1);
    kNearest->train(samples, ml::ROW_SAMPLE, responseLabels);

    //TODO?
    kNearest->save("test.yml");

    if (showImageFlags & SHOW_KNN_TRAINING_FLAG) {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("KNN training samples", samples);
    }
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

ReaderResult SevenSegmentGaugeReader::Classify(DigitFeatures *features)
{
    return ReaderResult();
}

ReaderResult SevenSegmentGaugeReader::ReadGaugeImage(Mat src)
{
    Mat enhanced, segmented, srcScaled;
    EnhanceImage(src, enhanced, srcScaled);
    SegmentImage(enhanced, segmented);
    return Classify(ExtractFeatures(segmented, enhanced, srcScaled));
}

Mat SevenSegmentGaugeReader::loadReferenceImage(string fileName)
{
    string path = referenceImageDir + fileName;
    Mat dst = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
    if(!dst.data)
    {
        qDebug() << QString::fromStdString(path) << " can not be loaded";
        throw Exception(0, "Could not load image", "loadReferenceImage", "SevenSegmentGaugeReader.cpp", 0);
    }
    qDebug() << QString::fromStdString(referenceImageDir + fileName) << " loaded";

    return dst;
}

Mat SevenSegmentGaugeReader::classifyDigitsBySegmentPositions(Mat src, vector<vector<Point>> contours){
    Mat markedDigits;
    src.copyTo(markedDigits);

    Mat drawing(markedDigits);

    //Mask derived from https://www.pyimagesearch.com/2017/02/13/recognizing-digits-with-opencv-and-python/
    map<vector<int>, int> DIGITS_LOOKUP;
    vector<int> zero { 1, 1, 1, 0, 1, 1, 1 }; DIGITS_LOOKUP[zero] = 1;
    vector<int> one{ 0, 0, 1, 0, 0, 1, 0 }; DIGITS_LOOKUP[one] = 2;
    vector<int> two{ 1, 0, 1, 1, 1, 0, 1 }; DIGITS_LOOKUP[two] = 3;
    vector<int> three{ 1, 0, 1, 1, 0, 1, 1 }; DIGITS_LOOKUP[three] = 4;
    vector<int> four{ 0, 1, 1, 1, 0, 1, 0 }; DIGITS_LOOKUP[four] = 5;
    vector<int> five{ 1, 1, 0, 1, 0, 1, 1 }; DIGITS_LOOKUP[five] = 6;
    vector<int> six{ 1, 1, 0, 1, 1, 1, 1 }; DIGITS_LOOKUP[six] = 7;
    vector<int> seven{ 1, 0, 1, 0, 0, 1, 0 }; DIGITS_LOOKUP[seven] = 8;
    vector<int> eight{ 1, 1, 1, 1, 1, 1, 1 }; DIGITS_LOOKUP[eight] = 9;
    vector<int> nine{ 1, 1, 1, 1, 0, 1, 1 }; DIGITS_LOOKUP[nine] = 10;

    string digits = "";

    for (uint i = 0; i< contours.size(); i++)
    {
        Rect rect = boundingRect(Mat(contours[i]));
        int width = rect.width;
        int height = rect.height;

        //TODO: the correct size depends on display size in source image/ distance to display
        if (height>20 && height < 80 && width>10 && width < 80)
            //	if(height>2 && width>1)
        {
            Mat roiTest = markedDigits(rect);

            int w = rect.width;
            int h = rect.height;
            int roiH = roiTest.rows;
            int roiW = roiTest.cols;
            int dW = int(roiW * 0.25);
            int dH = int(roiH * 0.15);
            int dHC = int(roiH * 0.05);

            //# define the set of 7 segments
            vector<vector<Point2d>> segments;
            vector<Point2d> segment0 = getPoint(Point2d(5, 0), Point2d(w - 2, dH + 2)); // top										0
            vector<Point2d> segment1 = getPoint(Point2d(3, 5), Point2d(dW + 5, h / 2)); // top left									1
            vector<Point2d> segment2 = getPoint(Point2d(w - dW, 0), Point2d(w, h / 2)); // top right                                2
            vector<Point2d> segment3 = getPoint(Point2d(5, (h / 2) - (dHC + 2)), Point2d(w - 5, (h / 2) + (dHC + 2))); // center	3
            vector<Point2d> segment4 = getPoint(Point2d(0, (h / 2)), Point2d(dW, h)); // bottom left								4
            vector<Point2d> segment5 = getPoint(Point2d(w - dW - 5, h / 2), Point2d(w - 5, h)); // bottom right						5
            vector<Point2d> segment6 = getPoint(Point2d(0, h - dH - 2), Point2d(w - 5, h)); // bottom
            segments.push_back(segment0);
            segments.push_back(segment1);
            segments.push_back(segment2);
            segments.push_back(segment3);
            segments.push_back(segment4);
            segments.push_back(segment5);
            segments.push_back(segment6);

            //Mat element = getStructuringElement(MORPH_RECT, Size(1, 1));
            //dilate(roiTest, roiTest, element);

            vector<int> on = { 0, 0, 0, 0, 0, 0, 0 };
            int i1 = 0;
            for (auto& seg : segments) {
                try
                {
                    Point2d p1 = seg[0];
                    Point2d p2 = seg[1];
                    Rect rect2(p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);
                    Mat segROI = roiTest(rect2);// [yA:yB, xA : xB]
                    int	total = countNonZero(segROI);
                    double area = (p2.x - p1.x) * (p2.y - p1.y);
                    double v = total / area;
                    if (v > 0.5) {
                        on[i1] = 1;
                    }
                }
                catch (cv::Exception & e)
                {
                    cerr << e.msg << endl; // output exception message
                }

                i1++;
            }

            int digit = DIGITS_LOOKUP[on];
            if (width < 20 && height > 20 && digit == 9) { //with digit 1 the contour is not wide
                digit = 2;
            }
            if (digit > 0) {
                digit--;
                Scalar color = Scalar(255, 255, 255);

                Point2d p = Point2d(rect.x - 10, rect.y - 10);
                rectangle(drawing, rect.tl(), rect.br(), color, 2, 8, 0);
                putText(drawing, to_string(digit), p, FONT_HERSHEY_DUPLEX, 3, color, 4);

                digits += to_string(digit);
            }
        }
    }
    /* if(digits != ""){
        QMessageBox msgBox;
        msgBox.setText(QString::fromStdString("digits are " + digits));
        msgBox.exec();
    }*/
    imshow("classifyDigitsBySegmentPositions: markedDigits", markedDigits);

    return markedDigits;
}

vector<Point2d> SevenSegmentGaugeReader::getPoint(Point2d p1 , Point2d p2) {
    vector<Point2d> segment;
    segment.push_back(p1);
    segment.push_back(p2);
    return segment;
}

DigitFeatures SevenSegmentGaugeReader::extractDigitFeaturesByCustomTemplateMatching(Mat src)
{
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("calculateDigitSize: src", src);

    // Only for test
    int imgSizeTestCounter = 0;
    int imageRowTestCounter = 0;
    int maxMaskColTest = 0;
    ulong imageColTestCounter = 0;
    ulong maskRowTestCounter = 0;
    ulong maskColTestCounter = 0;
    int sampleMaskMatchCounter = 0;
    //

    Mat maskDigit8p = loadReferenceImage("MaskDigit8SegmentP_30x40.png");
    Mat maskSegmentBc = loadReferenceImage("MaskSegmentBC_30x40.png");
    Mat maskSegmentAgd = loadReferenceImage("MaskSegmentAGD_30x40.png");

    bitwise_not(src, src);
    bitwise_not(maskDigit8p, maskDigit8p);
    bitwise_not(maskSegmentBc, maskSegmentBc);
    bitwise_not(maskSegmentAgd, maskSegmentAgd);

    imageAnalizer.showImage("maskDigit8p", maskDigit8p);

    // TEST
    //    imageAnalizer.showImage("calculateDigitSize: maskDigit8p", maskDigit8p);
    //    imageAnalizer.showImage("calculateDigitSize: maskSegmentBc", maskSegmentBc);
    //    imageAnalizer.showImage("calculateDigitSize: maskSegmentAgd", maskSegmentAgd);

    Mat scaledImg;

    // TODO: configurable
    int maskMoveStepSize = 1;
    // TODO: relative to actual image height per loop
    int imgHScaleStepSize = 3;
    double imgAspectRatio = src.cols / (double)src.rows;

    //TODO: values based on requirements
    int minImgH = maskDigit8p.rows * 18;
    int maxImgH = maskDigit8p.rows * 22;

    qDebug() << "imgAspectRatio " << imgAspectRatio;

    int imgW, imgH;

    int maxSamplePixelsOutsideMask = 0;                             // This requires entirely empty background

    int maxObjectPixelsOutsideMask = 75;
    int maxObjectPixelsMissingInMaskBc = 20;
    int maxObjectPixelsMissingInMaskAgd = 30;

    bool isMatch = false;

    vector<Point> sampleMask;
    sampleMask.push_back(Point(24, 10));              // Segment B
    sampleMask.push_back(Point(21, 27));              // Segment C

    sampleMask.push_back(Point(16, 4));                // Segment A
    sampleMask.push_back(Point(12, 36));              // Segment D

    sampleMask.push_back(Point(14, 20));              // Segment G

    sampleMask.push_back(Point(12, 15));              // Top center left
    sampleMask.push_back(Point(18, 10));              // Top center right
    sampleMask.push_back(Point(10, 31));              // Bottom center left
    sampleMask.push_back(Point(16, 25));              // Bottom center right

    // Tested
    // show sample points
    //    Mat samplePoints(maskDigit8p.size(), CV_8UC1, cvScalar(255));
    //    for(vector<Point>::iterator it = sampleMask.begin(); it != sampleMask.end(); ++it) {
    //        Point samplePoint = *it;
    //        samplePoints.at<uchar>(samplePoint) = 0;
    //    }
    //    imageAnalizer.showImage("samplePoints", samplePoints);
    //    return DigitInfo(0, 0, 0);
    //

    int imgRow, imgCol;

    // Start with smallest image size: faster calculation
    // Iterate over image sizes
    for (imgH = minImgH; (imgH <= maxImgH) && !isMatch; imgH += imgHScaleStepSize)
    {
        // Only for test
        imgSizeTestCounter++;
        //

        imgW = (int)(imgH * imgAspectRatio + 0.5);
        // Tested
        //scaledImg = loadReferenceImage("MaskDigit8SegmentP_30x40.png");   // match
        //scaledImg = loadReferenceImage("Digit1_30x40.png");   // match (missing 3 pixels in Adg)
        //scaledImg = loadReferenceImage("Digit2_30x40.png");   // match (missing 1 pixel in Bc)
        //scaledImg = loadReferenceImage("testSegmentEF.png");             // missing 2 pixels in Bc, 3 pixels in Adg
        //scaledImg = loadReferenceImage("testDigit8PartlyFilled.png");     // 2 pixels outside mask
        //scaledImg = loadReferenceImage("Digit8p_30x40InIMage300x400.png");     // match at point 135,220
        //bitwise_not(scaledImg, scaledImg);

        resize(src, scaledImg, Size(imgW, imgH), INTER_NEAREST);
        threshold(scaledImg, scaledImg, 127, 255, THRESH_BINARY);

        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("calculateDigitSize: scaledImg", scaledImg);

        // TODO: show desired roi in camera view
        // TODO: configurable roi
        int colStart = (int)(scaledImg.cols / 3.0 + 0.5);   // We need just one digit, so roi width can be small
        int colEnd = scaledImg.cols - colStart - maskDigit8p.cols;
        int rowStart = (int)(scaledImg.rows / 4.0 + 0.5);
        int rowEnd = scaledImg.rows - rowStart - maskDigit8p.rows;

        qDebug() << "scaledImg.rows " << scaledImg.rows;
        qDebug() << "scaledImg.cols " << scaledImg.cols;

        qDebug() << "colStart " << colStart;
        qDebug() << "colEnd " << colEnd;
        qDebug() << "rowStart " << rowStart;
        qDebug() << "rowEnd " << rowEnd;

        // Iterate over image rows and cols
        for (imgRow = rowStart; (imgRow <= rowEnd) && !isMatch; imgRow += maskMoveStepSize)
        {
            // Only for test
            imageRowTestCounter++;
            //

            for (imgCol = colStart; (imgCol <= colEnd) && !isMatch; imgCol += maskMoveStepSize)
            {
                // Only for test
                imageColTestCounter++;
                //

                int objectPixelsOutsideMask = 0;
                int objectPixelsMissingInMaskBc = 0;
                int objectPixelsMissingInMaskAdg = 0;

                int samplePixelsOutsideMask = 0;
                int samplePixelsMissingInMaskBc = 0;
                int samplePixelsMissingInMaskAdg = 0;
                bool canMatch = true;

                int maxMaskColTestCounter = 0;

                // iterate over sample points
                for(vector<Point>::iterator it = sampleMask.begin(); it != sampleMask.end() && canMatch; ++it)
                {
                    Point samplePoint = *it;
                    // Only for test
                    maskColTestCounter++;
                    maxMaskColTestCounter++;

                    //
                    uchar imgPixel = scaledImg.at<uchar>(imgRow + samplePoint.y, imgCol + samplePoint.x);
                    uchar maskDigit8pPixel = maskDigit8p.at<uchar>(samplePoint);
                    uchar maskSegmentBcPixel = maskSegmentBc.at<uchar>(samplePoint);
                    uchar maskSegmentAgdPixel = maskSegmentAgd.at<uchar>(samplePoint);

                    if ((imgPixel & maskDigit8pPixel) != imgPixel) {
                        samplePixelsOutsideMask++;
                    } else {
                        if ((imgPixel & maskSegmentBcPixel) != maskSegmentBcPixel)
                            samplePixelsMissingInMaskBc++;
                        if ((imgPixel & maskSegmentAgdPixel) != maskSegmentAgdPixel)
                            samplePixelsMissingInMaskAdg++;
                    }
                    if (samplePixelsOutsideMask > maxSamplePixelsOutsideMask ||
                            (samplePixelsMissingInMaskBc > 0 &&
                             samplePixelsMissingInMaskAdg > 0))
                        canMatch = false;
                }

                //                if (canMatch) {
                //qDebug() << "canMatch = true";
                //                }
                //                else
                //                {
                //                    qDebug() << "canMatch = false";
                //                }
                //TEST
                if (maxMaskColTestCounter > maxMaskColTest)
                    maxMaskColTest = maxMaskColTestCounter;

                //                qDebug() << "samplePixelsOutsideMask:" << samplePixelsOutsideMask;
                //                qDebug() << "samplePixelsMissingInMaskBc:" << samplePixelsMissingInMaskBc;
                //                qDebug() << "samplePixelsMissingInMaskAdg:" << samplePixelsMissingInMaskAdg;

                isMatch = canMatch;
                if (isMatch)
                {
                    sampleMaskMatchCounter++;

                    qDebug() << "sampleMaskMatchCounter: " << sampleMaskMatchCounter;

                    // Verify match by refined search for all mask pixels
                    for (int maskRow = 0; (maskRow < maskDigit8p.rows) && canMatch; maskRow++)
                    {
                        for (int maskCol = 0; (maskCol < maskDigit8p.cols) && canMatch; maskCol++)
                        {
                            uchar imgPixel = scaledImg.at<uchar>(imgRow + maskRow, imgCol + maskCol);
                            uchar maskDigit8pPixel = maskDigit8p.at<uchar>(maskRow, maskCol);
                            uchar maskSegmentBcPixel = maskSegmentBc.at<uchar>(maskRow, maskCol);
                            uchar maskSegmentAgdPixel = maskSegmentAgd.at<uchar>(maskRow, maskCol);
                            // Pixels outside mask pattern shoud be 0.
                            if ((imgPixel & maskDigit8pPixel) != imgPixel) {
                                objectPixelsOutsideMask++;
                                // Pixels within mask pattern should entirely fill segment B and C or segment A and G and D,
                                // because 7-segment digits 0...9 match at least 1 of these masks.
                            } else {
                                if ((imgPixel & maskSegmentBcPixel) != maskSegmentBcPixel)
                                    objectPixelsMissingInMaskBc++;
                                if ((imgPixel & maskSegmentAgdPixel) != maskSegmentAgdPixel)
                                    objectPixelsMissingInMaskAdg++;
                            }
                            if (objectPixelsOutsideMask > maxObjectPixelsOutsideMask ||
                                    (objectPixelsMissingInMaskBc > maxObjectPixelsMissingInMaskBc &&
                                     objectPixelsMissingInMaskAdg > maxObjectPixelsMissingInMaskAgd))
                            {
                                canMatch = false;
                                isMatch = false;
                            }
                        }
                    }
                    if (isMatch)
                    {
                        qDebug() << "match verified, ";
                        qDebug() << "   objectPixelsOutsideMask: " << objectPixelsOutsideMask;
                        qDebug() << "   objectPixelsMissingInMaskBc: " << objectPixelsMissingInMaskBc;
                        qDebug() << "   objectPixelsMissingInMaskAdg: " << objectPixelsMissingInMaskAdg;
                    }
                }
            }
        }
    }
    // TEST
    qDebug() << "imgSizeTestCounter: " << imgSizeTestCounter;
    qDebug() << "imageRowTestCounter: " << imageRowTestCounter;
    qDebug() << "imageColTestCounter: " << imageColTestCounter;
    qDebug() << "maskRowTestCounter: " << maskRowTestCounter;
    qDebug() << "maskColTestCounter: " << maskColTestCounter;
    qDebug() << "maxMaskColTest: " << maxMaskColTest;
    qDebug() << "sampleMaskMatchCounter" << sampleMaskMatchCounter;

    if (isMatch)
    {
        int digitW = (int)(maskDigit8p.cols * src.cols / (double)scaledImg.cols + 0.5);
        int digitH = (int)(maskDigit8p.rows * src.rows / (double)scaledImg.rows + 0.5);
        int digitX = (int)(imgCol * src.cols / (double)scaledImg.cols + 0.5);
        int digitY = (int)(imgRow * src.rows / (double)scaledImg.rows + 0.5);

        //TEST
        Mat markedMatch;
        src.copyTo(markedMatch);
        // draw a bounding box around the detected result and display the image
        rectangle(markedMatch, Point(digitX, digitY), Point(digitX + digitW, digitY + digitH), Scalar(255, 255, 255), 1);
        imageAnalizer.showImage("found match:", markedMatch);

        return DigitFeatures(digitW, digitH, digitY + digitH, digitX);
    }
    else
    {
        qDebug() << "No digits recognized";

        return DigitFeatures(0, 0, src.rows-1, 0);
        //throw Exception(0, "No digits recognized", "calculateDigitSize", "SevenSegmentGaugeReader.cpp", 0);
    }
}

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// and C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
// Contains the description of the match
typedef struct TemplateMatchFeatures {
    bool init;
    double maxVal;
    Point maxLoc;
    double scale;
    TemplateMatchFeatures(): init(0){}
} TemplateMatchFeatures;

double SevenSegmentGaugeReader::classifyDigitsByTemplateMatching(Mat src, DigitFeatures digitFeatures)
{
    Mat scaledImage;
    resize(src, scaledImage, Size(0,0), digitFeatures.imageScale, digitFeatures.imageScale);
    threshold(scaledImage, scaledImage, 127, 255, THRESH_BINARY);

    qDebug() << "readDigits, digitInfo:" << digitFeatures.width << digitFeatures.height;

    // maxvalue = 1?
    //    int templateMatchMethod = TM_CCORR_NORMED;   // TM_CCOEFF, TM_SQDIFF_NORMED, TM_SQDIFF
    int templateMatchMethod = TM_CCORR_NORMED;   // TM_CCOEFF, TM_SQDIFF_NORMED, TM_SQDIFF

    bitwise_not(scaledImage,scaledImage);

    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("scaledImage", scaledImage);

    vector<Mat> templates;
    for(int i=0; i < 10; i++)
    {
        Mat temp = loadReferenceImage("Digit" + std::to_string(i) + "_30x40.png");
        bitwise_not(temp,temp);
        templates.push_back(temp);
    }
    templates.push_back(loadReferenceImage("DecimalPoint_7x11.png"));

    int marginH = 20;
    Rect roiRect(0, digitFeatures.bottomY - digitFeatures.height - marginH, src.cols, digitFeatures.bottomY + marginH);
    Mat roi = src(roiRect);
    vector<DigitFeatures> recognizedDigits;

    imageAnalizer.resetNextWindowPosition();

    Mat endResultMat(scaledImage.size(), CV_8UC1, Scalar(255));
    //TODO: also for last item
    for(int i=0; i< 10; i++)//; i < templates.size()-1; i++)
    {
        Mat matchResult;
        matchTemplate(scaledImage, templates[i], matchResult, templateMatchMethod);

        // Optional: normalize result to range 0..1 (https://docs.opencv.org/3.3.0/da/d53/MatchTemplate_Demo_8cpp-example.html#a15)
        //        normalize(matchResult, matchResult, 0, 1, NORM_MINMAX, -1, Mat());

        // Find multiple matches (retrieved from https://stackoverflow.com/questions/12328438/opencv-finding-multiple-matches)
        Mat thresholdedMatchResult;

        threshold(matchResult, thresholdedMatchResult, templateMatchThreshold, 255, THRESH_BINARY);

        string str = "readDigits: matchResult " + to_string(i);
        string str1 = "readDigits: thresholdedMatchResult " + to_string(i);
        const char* c = str.c_str();
        const char* c1 = str1.c_str();

        imageAnalizer.showImage(c, matchResult);
        imageAnalizer.showImage(c1, thresholdedMatchResult);

        for (int r = 0; r < thresholdedMatchResult.rows; ++r) {
            for (int c = 0; c < thresholdedMatchResult.cols; ++c) {
                // qDebug() << "thresholdedMatchResult.at<uchar>(r, c)" << r << c << thresholdedMatchResult.at<uchar>(r, c);

                //TODO: value is often 0 while there are few black pixels
                if (thresholdedMatchResult.at<uchar>(r, c) > 0) // black // = thresholdedImage(r,c) == 0
                {
                    uchar px = matchResult.at<uchar>(r, c);
                    qDebug() << "found digit: " << i;

                    uchar resultPx = endResultMat.at<uchar>(r, c);
                    if (resultPx == 255 || resultPx < px)
                    {
                        qDebug() << "digit added to result: " << i;
                        endResultMat.at<uchar>(r, c) = px;
                        recognizedDigits.push_back(DigitFeatures(digitFeatures.width, digitFeatures.height, r + digitFeatures.height, c, i));
                    }
                }
            }
        }
    }

    // TODO: determine per pixel location and per matchResult which template has the highest value.

    //TODO
    return 0;
}

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// (C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
// and https://docs.opencv.org/3.3.0/da/d53/MatchTemplate_Demo_8cpp-example.html#a15
DigitFeatures SevenSegmentGaugeReader::extractReferenceDigitFeaturesByMultiScaleTemplateMatch(Mat src)
{
    Mat templateDigit8p = loadReferenceImage("MaskDigit8SegmentP_30x40.png");
    Mat scaledImage;

    int templateMatchCannyThreshold1 = 50;
    int templateMatchCannyThreshold2 = 50 * 4;
    bool templayteMatchByEdges = true;
    int templateMatchMethod = TM_CCORR_NORMED;   // TM_CCOEFF, TM_SQDIFF_NORMED, TM_SQDIFF, TM_CCORR_NORMED

    int templateWidth = templateDigit8p.cols;
    int templateHeight = templateDigit8p.rows;

    // Optional, for match by edges
    Mat templateEdges, scaledEdges;
    Canny(templateDigit8p, templateEdges, templateMatchCannyThreshold1, templateMatchCannyThreshold2);

    const float SCALE_START = 1;
    const float SCALE_END = 0.2;
    const int SCALE_POINTS = 20;

    TemplateMatchFeatures foundMatchInfo;
    for(float scale = SCALE_START; scale >= SCALE_END; scale -= (SCALE_START - SCALE_END)/SCALE_POINTS){
        resize(src, scaledImage, Size(0,0), scale, scale);

        if(templateWidth > scaledImage.cols || templateHeight > scaledImage.rows)
            break;  // scaled image size < template size

        // Optional: match edges instead of image
        Canny(scaledImage, scaledEdges, templateMatchCannyThreshold1, templateMatchCannyThreshold2);

        Mat matchResult;
        int result_cols =  scaledImage.cols - templateWidth + 1;
        int result_rows = scaledImage.rows - templateHeight + 1;
        matchResult.create( result_rows, result_cols, CV_32FC1 );

        if (templayteMatchByEdges)
            matchTemplate(scaledEdges, templateEdges, matchResult, templateMatchMethod);
        else
            matchTemplate(scaledImage, templateDigit8p, matchResult, templateMatchMethod);

        // Optional: normalize result to range 0..1 (https://docs.opencv.org/3.3.0/da/d53/MatchTemplate_Demo_8cpp-example.html#a15)
        //        normalize(matchResult, matchResult, 0, 1, NORM_MINMAX, -1, Mat());

        // Find the minimum and maximum element values and their positions
        double minVal; double maxVal; Point minLoc; Point maxLoc;
        Point matchLoc;

        //        minMaxLoc(matchResult, NULL, &maxVal, NULL, &maxLoc);
        minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );
        if( templateMatchMethod  == TM_SQDIFF || templateMatchMethod == TM_SQDIFF_NORMED )
            matchLoc = minLoc;
        else
            matchLoc = maxLoc;

        // keep new match if it is better
        if( foundMatchInfo.init == false || maxVal > foundMatchInfo.maxVal ){
            foundMatchInfo.init = true;
            foundMatchInfo.maxVal = maxVal;
            foundMatchInfo.maxLoc = maxLoc;
            foundMatchInfo.scale = scale;
        }

        // START VISUALIZATION CODE
        //        Mat target_clone;
        //        resize(src, target_clone, Size(0,0), scale, scale);// Resize
        //        rectangle(target_clone, Point(maxLoc.x, maxLoc.y), Point(maxLoc.x + templateWidth, maxLoc.y + templateHeight), Scalar(0, 255, 255), 3);
        //        imshow("DBG", target_clone);
        //        waitKey(100);
        // END VISUALIZATION CODE
    }

    int startX, startY, endX, endY;
    startX = (int)(foundMatchInfo.maxLoc.x / foundMatchInfo.scale + 0.5);
    startY = (int)(foundMatchInfo.maxLoc.y / foundMatchInfo.scale + 0.5);

    endX= (int)((foundMatchInfo.maxLoc.x + templateWidth) / foundMatchInfo.scale + 0.5);
    endY= (int)((foundMatchInfo.maxLoc.y + templateHeight) / foundMatchInfo.scale + 0.5);

    Mat markedMatch;
    src.copyTo(markedMatch);

    // draw a bounding box around the detected result and display the image
    rectangle(markedMatch, Point(startX, startY), Point(endX, endY), Scalar(0, 0, 255), 1);

    if (showImageFlags & SHOW_FEATURE_EXTRACT_REFERENCE_DIGIT_FLAG) {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("multiScaleTemplateMatch: templateDigit8p", templateDigit8p);
        imageAnalizer.showImage("multiScaleTemplateMatch: templateEdges", templateEdges);
        imageAnalizer.showImage("multiScaleTemplateMatch, markedMatch", markedMatch);
    }

    //TODO: handle not found
    return DigitFeatures(endX - startX, endY - startY, endY, startX, -1, foundMatchInfo.scale);
}
