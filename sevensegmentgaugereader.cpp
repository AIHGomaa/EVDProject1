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
    
    if (showImageFlags & SHOW_IMAGE_ENHANCEMENT_FLAG) {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("EnhanceImage: src", src);
        imageAnalizer.showImage("EnhanceImage: grayScaled", grayScaled);
        imageAnalizer.showImage("EnhanceImage: blurred", blurred);
        imageAnalizer.showImage("EnhanceImage: adaptThreshold", adaptThreshold);
        imageAnalizer.showImage("EnhanceImage: dst", dst.getMat());
    }
}

void SevenSegmentGaugeReader::SegmentImage(Mat src, OutputArray dst)
{
    Mat cannyEdges(IMG_SIZE, CV_8UC1, 1);
    Canny(src, cannyEdges, cannyThreshold1, cannyThreshold2, cannyAppertureSize, true);
    
    cannyEdges.copyTo(dst);
    
    if(showImageFlags & SHOW_SEGMENTATION_FLAG) {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("SegmentImage: src", src);
        imageAnalizer.showImage("SegmentImage: cannyEdges", cannyEdges);
    }
}

double SevenSegmentGaugeReader::calculateRotationDegrees(Mat edges)
{
    // Derived from http://felix.abecassis.me/2011/09/opencv-detect-skew-angle/
    vector<cv::Vec4i> hou;
    cv::HoughLinesP(edges, hou, houghDistanceResolution, houghAngleResolutionDegrees * CV_PI/180.0, houghVotesThreshold, houghMinLineLength, houghMaxLineGap);

    cv::Mat disp_lines(IMG_SIZE, CV_8UC1, cv::Scalar(0, 0, 0));
    double angleRad;
    unsigned nLines = hou.size();

    if (nLines == 0)
    {
        qDebug() << "no lines found for rotation calculation";
        return 0;
    }

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

bool SevenSegmentGaugeReader::isPotentialDigitOrDecimalPoint(Rect rect, SevenSegmentDigitCriteria criteria)
{
    return
            // All on reference botom Y
            rect.br().y >= criteria.minDigitBottomY &&
            rect.br().y <= criteria.maxDigitBottomY &&
            // Size of digits 0 and 2..9
            ((rect.height >= criteria.minDigitSize.height && rect.height <= criteria.maxDigitSize.height &&
              rect.width >= criteria.minDigitSize.width && rect.width <= criteria.maxDigitSize.width) ||
             // Size of digit 1
             (rect.height >= criteria.minDigit1Size.height && rect.height <= criteria.maxDigit1Size.height &&
              rect.width >= criteria.minDigit1Size.width && rect.width <= criteria.maxDigit1Size.width) ||
             // Size of decimal point
             (rect.height >= criteria.minDecimalPointSize.height && rect.height <= criteria.maxDecimalPointSize.height &&
              rect.width >= criteria.minDecimalPointSize.width && rect.width <= criteria.maxDecimalPointSize.width));
}

bool SevenSegmentGaugeReader::contourLeftToRightComparer(const vector<Point> a, const vector<Point> b)
{
    Rect ra(boundingRect(a));
    Rect rb(boundingRect(b));
    return (ra.x < rb.x);
}

SevenSegmentDigitFeatures * SevenSegmentGaugeReader::ExtractFeatures(Mat edges, Mat enhancedImage, Mat colorImage)
{
    double rotationDegrees = calculateRotationDegrees(edges);

    //TODO: configurable
    //TODO: in EnhanceImage?
    if(abs(rotationDegrees) > 0.5)
        correctRotation(rotationDegrees, colorImage, enhancedImage, colorImage, enhancedImage);

    // Extract features for one reference digit, to determine the desired image scale.
    // This is necessary for accuracy of findContours(),
    // because we optimize our kernels for a fixed digit size.
    SevenSegmentDigitFeatures referenceDigitFeatures = extractReferenceDigitFeaturesByMultiScaleTemplateMatch(enhancedImage);
    if (referenceDigitFeatures.width == 0)
    {
        // no matching digit found
        return &referenceDigitFeatures;
    }

    double imageScaleFactor = DIGIT_TEMPLATE_Y_RESOLUTION / (double)referenceDigitFeatures.height;

    SevenSegmentDigitCriteria criteria =
            SevenSegmentDigitCriteria::create(referenceDigitFeatures, DIGIT_TEMPLATE_SIZE, imageScaleFactor);

    // Resize image to get desired digit size
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

    // Derived from https://docs.opencv.org/trunk/d6/d6e/group__imgproc__draw.html#ga746c0625f1781f1ffc9056259103edbc
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    Rect roiRect(0, criteria.maxDigitBottomY - criteria.maxDigit1Size.height, dilatedEnhancedInverted.cols, criteria.maxDigit1Size.height);
    Mat contoursRoi = Mat(dilatedEnhancedInverted, roiRect);

    // Edges must be white, background must be black.
    findContours(contoursRoi, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(roiRect.tl()));
    // findContours(dilatedEnhancedInverted, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    // findContours(dilatedEnhancedInverted, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

    qDebug() << "contours.size() " << contours.size();
    
    sort(contours.begin(), contours.end(), contourLeftToRightComparer);
    
    vector<vector<Point>> contoursPoly( contours.size() );
    vector<Rect> boundRect( contours.size() );

    // Derived from http://answers.opencv.org/question/19374/number-plate-segmentation-c/
    for(uint i = 0; i < contours.size(); i++) {
        approxPolyDP(Mat(contours[i]), contoursPoly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contoursPoly[i]));
    }

    Mat colorResizedFiltered;
    string number;

    // Option 1
    //    markedDigits = berekenDigitAlgorithm(dilatedEnhancedInverted, contours);

    // Option 3: K-nearest

    // Option 4
    //classifyDigitsByTemplateMatching(enhancedImage, referenceDigitFeatures);

    //TODO: in classification
    // Filter color image by binary image.
    colorResized.copyTo(colorResizedFiltered, enhancedResizedInverted);

    Mat markedDigits;
    colorResized.copyTo(markedDigits);
    Mat drawing(markedDigits);

    vector<string> values { ".", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

    Scalar contourColor = Scalar(0,255,0);
    Scalar textColor = Scalar(255,255,255);

    for(uint i = 0; i < boundRect.size(); i++) {
        Rect rect = boundRect[i];
        if( showAllContoursForTest || isPotentialDigitOrDecimalPoint(rect, criteria)) {
            Mat imageroi = colorResizedFiltered(rect);
            imageroi.convertTo(imageroi, CV_32F);
            Mat roi, sample;
            resize(imageroi, roi, DIGIT_TEMPLATE_SIZE);
            roi.reshape(1, 1).convertTo(sample, CV_32F);

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
                rectangle(drawing, rect.tl(), rect.br(), contourColor, 2, LINE_4, 0);
                int textX = rect.br().x;
                if (val != 10)  // Exception for "."
                    textX -= DIGIT_TEMPLATE_SIZE.width / 2;
                putText(drawing, charValue, Point(textX, referenceDigitFeatures.bottomY * imageScaleFactor + DIGIT_TEMPLATE_SIZE.height * 0.5), FONT_HERSHEY_DUPLEX, 1.5, textColor, 3);

                //                putText(drawing, charValue, Point(rect.br().x - DIGIT_TEMPLATE_SIZE.width / 2, referenceDigitFeatures.bottomY * imageScaleFactor + DIGIT_TEMPLATE_SIZE.height * 0.5), FONT_HERSHEY_DUPLEX, 1.5, textColor, 3);
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
        imageAnalizer.showImage("Feature extract: contoursRoi", contoursRoi);
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("Feature extract: markedDigits", markedDigits);
    }

    //TODO
    SevenSegmentDigitFeatures * result = new SevenSegmentDigitFeatures(0, 0, 0, 0);

    return result;
}

void SevenSegmentGaugeReader::initialize() {
    if (!loadKNNDataAndTrainKNN()) {
        std::cout << std::endl << std::endl << "error: error: KNN traning was not successful" << std::endl;
    }
}

//TODO: Save/load training data: knn->save("my.yml") and knn->load("my.yml")
//Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();
bool SevenSegmentGaugeReader::loadKNNDataAndTrainKNN() {
    Mat samples;
    vector<int> responseLabels;
    for (int i = 0; i < 21; i++) {
        string fileName;
        if (i >= 10) {
            switch (i)
            {
            case 11:
                fileName = "BottomArrow.png";
                break;
            case 12:
                fileName = "ESC.png";
                break;
            case 13:
                fileName = "KG.png";
                break;
            case 14:
                fileName = "L2.png";
                break;
            case 15:
                fileName = "L3.png";
                break;
            case 16:
                fileName = "L4.png";
                break;
            case 17:
                fileName = "LeftArrow.png";
                break;
            case 18:
                fileName = "LeftArrow2.png";
                break;
            case 19:
                fileName = "RedSquares.png";
                break;
            case 20:
                fileName = "RightArrow.png";
                break;
            default:
                fileName = "Point_Color.png";
                break;
            }
        }
        else
        {
            fileName = "Digit" + to_string(i) + "_Color.png";
        }
        Mat img = loadReferenceImage(fileName, IMREAD_COLOR);
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
    kNearest->save("kNearest.yml");

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

ReaderResult SevenSegmentGaugeReader::Classify(SevenSegmentDigitFeatures *features)
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

Mat SevenSegmentGaugeReader::loadReferenceImage(string fileName, int flags)
{
    string path = referenceImageDir + fileName;
    Mat dst = imread(path, flags);
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

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// (C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
// and https://docs.opencv.org/3.3.0/da/d53/MatchTemplate_Demo_8cpp-example.html#a15
SevenSegmentDigitFeatures SevenSegmentGaugeReader::extractReferenceDigitFeaturesByMultiScaleTemplateMatch(Mat src)
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

    if (showImageFlags & SHOW_FEATURE_EXTRACT_REFERENCE_DIGIT_FLAG) {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("multiScaleTemplateMatch: templateDigit8p", templateDigit8p);
        imageAnalizer.showImage("multiScaleTemplateMatch: templateEdges", templateEdges);
    }

    const float SCALE_START = 1;
    const float SCALE_END = 0.2;
    const int SCALE_POINTS = 20;

    TemplateMatchFeatures foundMatchFeatures = TemplateMatchFeatures();
    for(float scale = SCALE_START; scale >= SCALE_END; scale -= (SCALE_START - SCALE_END)/SCALE_POINTS) {
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

        if(maxVal > 0 && maxVal > foundMatchFeatures.maxVal) {
            foundMatchFeatures.maxVal = maxVal;
            foundMatchFeatures.maxLoc = maxLoc;
            foundMatchFeatures.scale = scale;
        }

        if (showImageFlags & SHOW_FEATURE_EXTRACT_REFERENCE_DIGIT_FLAG)
        {
            Mat target_clone;
            resize(src, target_clone, Size(0,0), scale, scale);
            rectangle(target_clone, Point(maxLoc.x, maxLoc.y), Point(maxLoc.x + templateWidth, maxLoc.y + templateHeight), Scalar(127), 1);
            imageAnalizer.resetNextWindowPosition();
            imageAnalizer.showImage("FindReferenceImage multiscale", target_clone);
            waitKey(100);
        }
    }

    if (foundMatchFeatures.maxVal == TemplateMatchFeatures::UNDEFINED)
    {
        qDebug() << "No reference digit found.";
        return SevenSegmentDigitFeatures(0, 0, 0, 0);
    }

    int startX, startY, endX, endY;
    startX = (int)(foundMatchFeatures.maxLoc.x / foundMatchFeatures.scale + 0.5);
    startY = (int)(foundMatchFeatures.maxLoc.y / foundMatchFeatures.scale + 0.5);

    endX= (int)((foundMatchFeatures.maxLoc.x + templateWidth) / foundMatchFeatures.scale + 0.5);
    endY= (int)((foundMatchFeatures.maxLoc.y + templateHeight) / foundMatchFeatures.scale + 0.5);

    Mat markedMatch;
    src.copyTo(markedMatch);

    // draw a bounding box around the detected result and display the image
    rectangle(markedMatch, Point(startX, startY), Point(endX, endY), Scalar(0, 0, 255), 1);

    if (showImageFlags & SHOW_FEATURE_EXTRACT_REFERENCE_DIGIT_FLAG) {
        imageAnalizer.showImage("multiScaleTemplateMatch, markedMatch", markedMatch);
    }

    return SevenSegmentDigitFeatures(endX - startX, endY - startY, endY, startX, -1, foundMatchFeatures.scale);
}
