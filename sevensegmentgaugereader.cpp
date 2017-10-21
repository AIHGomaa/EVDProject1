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

// TODO: use enhanced instead of srcOriginal?
ImageObject * SevenSegmentGaugeReader::ExtractFeatures(Mat edges, Mat enhancedImage, Mat srcOriginal)
{
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("ExtractFeatures: edges", edges);
    imageAnalizer.showImage("ExtractFeatures: enhancedImage", enhancedImage);
    imageAnalizer.showImage("ExtractFeatures: srcOriginal", srcOriginal);

    //-----------------------------------
    // Correct rotation
    //-----------------------------------
    // Derived from http://felix.abecassis.me/2011/09/opencv-detect-skew-angle/
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(edges, lines, houghDistanceResolution, houghAngleResolutionDegrees * CV_PI/180.0, houghVotesThreshold, houghMinLineLength, houghMaxLineGap);

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

    Mat reEnhancedAfterWarp(IMG_SIZE, CV_8UC1);

    //----------------------------
    // Rotation correction
    //----------------------------

    //TODO: configurable
    if(abs(medianAngleDegr) > 0.5)
    {
        // Derived from https://stackoverflow.com/questions/2289690/opencv-how-to-rotate-iplimage
        Point2f centerPoint(edges.cols/2.0F, edges.rows/2.0F);
        Mat rotationMatrix = getRotationMatrix2D(centerPoint, medianAngleDegr, 1.0);

        Mat rotationCorrected(IMG_SIZE, CV_8UC1);

        //TODO?: rotate earlier version of the image and repeat segmentation (and enhancement) for better contours search
        // White border values
        warpAffine(enhancedImage, rotationCorrected, rotationMatrix, enhancedImage.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(255));

        Mat thresAfterRotate;
        threshold(rotationCorrected, thresAfterRotate, 127, 255, THRESH_BINARY);

        Mat kernel = getStructuringElement(MORPH_RECT, Size(dilateKernelSize, dilateKernelSize));
//        Mat element = getStructuringElement(MORPH_RECT, Size(5, 5), Point(2, 2));
//        morphologyEx(srcCanny, srcCanny, MORPH_CLOSE, kernel);
        morphologyEx(thresAfterRotate, reEnhancedAfterWarp, MORPH_OPEN, kernel);

        // Only for test
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("Feature extract: disp_lines", disp_lines);
        imageAnalizer.showImage("Feature extract: rotationCorrected", rotationCorrected);
        imageAnalizer.showImage("Feature extract: thresAfterRotate", thresAfterRotate);
    }
    else
    {
        enhancedImage.copyTo(reEnhancedAfterWarp);
    }

    //TODO: choose the best option and cleanup
    //-----------------------------------
    // Create some variants of enhanced/ reEnhanced image for experiments.
    //-----------------------------------
    Mat reEnhancedAfterWarpInverted;
    bitwise_not(reEnhancedAfterWarp, reEnhancedAfterWarpInverted);
    Mat dilatedEdges;
    Mat dilateKernel = getStructuringElement(MORPH_RECT, Size(dilateKernelSize, dilateKernelSize));
    dilate(edges, dilatedEdges, dilateKernel);

    //-----------------------------------
    // Label segments
    //-----------------------------------

    imageAnalizer.showImage("Feature extract: reEnhancedAfterWarp", reEnhancedAfterWarpInverted);
    imageAnalizer.showImage("ExtractFeatures: edges dilated", dilatedEdges);

    //Size digitSize = calculateDigitSize(reEnhancedAfterWarp);

    //Mat labeledContours = Mat::zeros(IMG_SIZE, CV_8UC3);

    // Derived from https://docs.opencv.org/trunk/d6/d6e/group__imgproc__draw.html#ga746c0625f1781f1ffc9056259103edbc
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // Contours must be white in input, background must be black
    //TODO: minimize search by roi in image, based on requirements. (offset parameter must be used).
    // findContours on canny edges splits roi's of a digit if there are gaps between it's segments.
    findContours(reEnhancedAfterWarpInverted, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    // findContours(dilatedEdges, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE );

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

    //TODO: filter contours on all hierarchy levels by found digit size?
	
    // Derived from http://answers.opencv.org/question/19374/number-plate-segmentation-c/
    for( uint i = 0; i < contours.size(); i++ )
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
    }

    string number;

    // Option 1
    Mat markedDigits;// = berekenDigitAlgorithm(dilatedEdges);
//   Mat markedDigits = berekenDigitAlgorithm(reEnhancedAfterWarpInverted);

    // Alternative 2: Multi-scale template matching
    Size digitSize = calculateDigitSizeByMultiScaleTemplateMatch(reEnhancedAfterWarp);
    qDebug() << "digitSize by calculateDigitSizeByMultiScaleTemplateMatch" << digitSize.width << "*" << digitSize.height;

    // Option 3: K-nearest

    //TODO: srcOriginal can be rotated and was already scaled before. Use rotationCorrected instead? Then we also need grayscale samples.
   resize(srcOriginal, markedDigits, IMG_SIZE, 0, 0, INTER_LINEAR);

    vector<string> values { ".", "0", "1", "2", "3", "4", "5", "6", "7", "8","9" };

    Mat drawing(markedDigits);

    for( uint i = 0; i< contours.size(); i++ )
    {
        Rect rect = boundRect[i];

        Scalar color = Scalar(0,255,0);
        int width = rect.width;
        int height = rect.height;


        //TODO: check bound size using the found digit size

        if(height>2 && height < 150 && width>1 && width < 150)
            //if(height>2 && width>1)
        {
            Mat imageroi = markedDigits(rect);
            Mat roi, sample;
            resize(imageroi, roi, Size(30, 40));
            roi.reshape(1, 1).convertTo(sample, CV_32F);

            // Classification
            Mat results;
            float result = kNearest->findNearest(sample, kNearest->getDefaultK(), results);
            int val = (int)result;

            qDebug() << "val: " << val;

            string charValue = to_string(val);
            if (val == 10) {
                charValue = ".";
            }

            bool found = find(values.begin(), values.end(), charValue) != values.end();
            if (found == true)
            {
                rectangle( drawing, rect.tl(), rect.br(), color, 2, 8, 0 );
                color = Scalar(255,255,255);
                //TODO
//                putText(drawing, charValue, rect.br(), FONT_HERSHEY_DUPLEX, 1, color, 3);
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

    //TODO
    ImageObject * result = new ImageObject();

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("disp_lines", disp_lines);
    imageAnalizer.showImage("Feature extract: reEnhancedAfterWarp", reEnhancedAfterWarp);
    imageAnalizer.showImage("Feature extract: markedDigits", markedDigits);

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
//    imageAnalizer.resetNextWindowPosition();
//    imageAnalizer.showImage("samples", samples);

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
    Mat enhanced, segmented;
    EnhanceImage(src, enhanced);
    SegmentImage(enhanced, segmented);
    return Classify(ExtractFeatures(segmented, enhanced, src));
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

Mat SevenSegmentGaugeReader::berekenDigitAlgorithm(Mat dilated){

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(dilated, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

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
    Mat markedDigits;
    dilated.copyTo(markedDigits);

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

    for (int i = 0; i< contours.size(); i++)
    {
         Rect rect = boundRect[i];
         int width = rect.width;
         int height = rect.height;

         //TODO: the correct size depends on display size in source image/ distance to display
        if (height>20 && height < 80 && width>20 && width < 80)
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
            vector<Point2d> segment0 = getPoint(Point2d(10, 0), Point2d(w - 10, dH)); // top										0
            vector<Point2d> segment1 = getPoint(Point2d(10, 0), Point2d(dW + 10, h / 2)); // top left								1
            vector<Point2d> segment2 = getPoint(Point2d(w - dW, 0), Point2d(w, h / 2)); // top right								2
            vector<Point2d> segment3 = getPoint(Point2d(5, (h / 2) - (dHC + 2)), Point2d(w - 5, (h / 2) + (dHC + 2))); // center	3
            vector<Point2d> segment4 = getPoint(Point2d(0, h / 2), Point2d(dW, h)); // bottom left									4
            vector<Point2d> segment5 = getPoint(Point2d(w - dW - 5, h / 2), Point2d(w - 5, h)); // bottom right						5
            vector<Point2d> segment6 = getPoint(Point2d(0, h - dH), Point2d(w - 5, h)); // bottom									6
            segments.push_back(segment0);
            segments.push_back(segment1);
            segments.push_back(segment2);
            segments.push_back(segment3);
            segments.push_back(segment4);
            segments.push_back(segment5);
            segments.push_back(segment6);

            Mat element = getStructuringElement(MORPH_RECT, Size(2, 2));
            dilate(roiTest, roiTest, element);

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
            if (digit > 0) {

                digit = digit - 1;
                Scalar color = Scalar(255, 255, 255);

                rectangle(drawing, rect.tl(), rect.br(), color, 2, 8, 0);
                putText(drawing, to_string(digit), rect.br(), FONT_HERSHEY_DUPLEX, 1, color, 3);

                digits += digit;
            }
        }
    }
    if(digits != ""){
        QMessageBox msgBox;
        msgBox.setText(QString::fromStdString(digits));
        msgBox.exec();
    }
    imshow("Box", markedDigits);

    return markedDigits;
}

vector<Point2d> SevenSegmentGaugeReader::getPoint(Point2d p1 , Point2d p2) {
    vector<Point2d> segment;
    segment.push_back(p1);
    segment.push_back(p2);
    return segment;
}

Size SevenSegmentGaugeReader::calculateDigitSize(Mat src)
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
    //

    Mat maskDigit8p = loadReferenceImage("MaskDigit8SegmentP_30x40.png");
    Mat maskSegmentBc = loadReferenceImage("MaskSegmentBC_30x40.png");
    Mat maskSegmentAgd = loadReferenceImage("MaskSegmentAGD_30x40.png");

    //TEST
    bitwise_not(src, src);
    bitwise_not(maskDigit8p, maskDigit8p);
    bitwise_not(maskSegmentBc, maskSegmentBc);
    bitwise_not(maskSegmentAgd, maskSegmentAgd);

    // TEST
    imageAnalizer.showImage("calculateDigitSize: maskDigit8p", maskDigit8p);

    Mat scaledImg;

    // TODO: configurable
    int maskMoveStepSize = 1;
    // TODO: relative to actual image height per loop
    int imgHScaleStepSize = 4;
    double imgAspectRatio = src.cols / (double)src.rows;

    //TODO: values based on requirements
    int minImgH = maskDigit8p.rows * 18;
    int maxImgH = maskDigit8p.rows * 22;

    qDebug() << "imgAspectRatio " << imgAspectRatio;
    qDebug() << "minImgH " << minImgH;
    qDebug() << "maxImgH " << maxImgH;

    int imgW, imgH;

    int maxObjectPixelsOutsideMask = 80;
    int maxObjectPixelsMissingInMask = 60;

    bool isMatch = false;

    // Start with smallest image size: faster calculation
    // Iterate over mask sizes
    for (imgH = minImgH; (imgH <= maxImgH) && !isMatch; imgH += imgHScaleStepSize) {
        // Only for test
        imgSizeTestCounter++;
        //

        imgW = (int)(imgH * imgAspectRatio + 0.5);

        qDebug() << "imgW " << imgW;
        qDebug() << "imgH " << imgH;

        resize(src, scaledImg, Size(imgW, imgH), INTER_NEAREST);
        threshold(scaledImg, scaledImg, 127, 255, THRESH_BINARY);

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
        for (int imgRow = rowStart; (imgRow < rowEnd) && !isMatch; imgRow += maskMoveStepSize)
        {
            // Only for test
            imageRowTestCounter++;
            //

            for (int imgCol = colStart; (imgCol < colEnd) && !isMatch; imgCol += maskMoveStepSize)
            {
                // Only for test
                imageColTestCounter++;
                //

                int objectPixelsOutsideMask = 0;
                int objectPixelsMissingInMask = 0;
                bool canMatch = true;

                int maxMaskColTestCounter = 0;
                // iterate over mask rows and cols
                for (int maskRow = 0; (maskRow < maskDigit8p.rows) && canMatch; maskRow++)
                {
                    // Only for test
                    maskRowTestCounter++;
                    //

                    for (int maskCol = 0; (maskCol < maskDigit8p.cols) && canMatch; maskCol++)
                    {
                        // Only for test
                        maskColTestCounter++;
                        maxMaskColTestCounter++;
                        //

                        uchar imgPixel = scaledImg.at<uchar>(imgRow + maskRow, imgCol + maskCol);
                        uchar maskDigit8pPixel = maskDigit8p.at<uchar>(maskRow, maskCol);
                        uchar maskSegmentBcPixel = maskSegmentBc.at<uchar>(maskRow, maskCol);
                        uchar maskSegmentAgdPixel = maskSegmentAgd.at<uchar>(maskRow, maskCol);

//                        qDebug() << "maskDigit8pPixel" << maskDigit8pPixel;
//                        qDebug() << "imgPixel" << imgPixel;

//                        // Pixels outside mask pattern shoud be 0.
                        if (imgPixel > maskDigit8pPixel) {
                        //if ((imgPixel & maskDigit8pPixel) != imgPixel)
                            objectPixelsOutsideMask++;
//                        // Pixels within mask pattern should entirely fill segment B and C or segment A and G and D,
//                        // because 7-segment digits 0...9 match at least 1 of these masks.
                        } else {
                            if ((imgPixel < maskSegmentBcPixel) ||
                                 (imgPixel < maskSegmentAgdPixel)) {
//                            if ((imgPixel & maskSegmentBcPixel) != maskSegmentBcPixel &&
//                                 (imgPixel & maskSegmentAgdPixel) != maskSegmentAgdPixel)
                                objectPixelsMissingInMask++;
                            }
                        }

                        if ((objectPixelsOutsideMask > maxObjectPixelsOutsideMask) ||
                                (objectPixelsMissingInMask > maxObjectPixelsMissingInMask))
                            canMatch = false;


                            if (canMatch) {
                                //qDebug() << "canMatch = true";
                            }
                            else
                            {
                                qDebug() << "canMatch = false";
                            }
                    }
                }
                //TEST
                if (maxMaskColTestCounter > maxMaskColTest)
                    maxMaskColTest = maxMaskColTestCounter;

                qDebug() << "objectPixelsOutsideMask:" << objectPixelsOutsideMask;
                qDebug() << "objectPixelsMissingInMask:" << objectPixelsMissingInMask;
                qDebug() << "image col: " << imgCol;
                qDebug() << "image row: " << imgRow;


                isMatch = canMatch;
                if (isMatch)
                {
                    qDebug() << "image col: " << imgCol;
                    qDebug() << "image row: " << imgRow;
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

    if (isMatch)
    {
        int digitW = (int)(maskDigit8p.cols * src.cols / (double)scaledImg.cols + 0.5);
        int digitH = (int)(maskDigit8p.rows * src.rows / (double)scaledImg.rows + 0.5);

        qDebug() << "digit size: " << digitW << "*" << digitH;

        return Size(digitW, digitH);
    }
    else
    {
        qDebug() << "No digits recognized";

        return Size(0, 0);
        //throw Exception(0, "No digits recognized", "calculateDigitSize", "SevenSegmentGaugeReader.cpp", 0);
    }
}

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// and C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
// Contains the description of the match
typedef struct TemplateMatchInfo{
    bool init;
    double maxVal;
    Point maxLoc;
    double scale;
    TemplateMatchInfo(): init(0){}
} TemplateMatchInfo;

// Derived from https://www.pyimagesearch.com/2015/01/26/multi-scale-template-matching-using-python-opencv/
// and C++ translation on http://hassannadeem.com/assets/code/multi_scale_template_matching_cpp.zip)
Size SevenSegmentGaugeReader::calculateDigitSizeByMultiScaleTemplateMatch(Mat src)
{
    Mat templateDigit8p = loadReferenceImage("MaskDigit8SegmentP_30x40.png");
    int templateWidth = templateDigit8p.cols;
    int templateHeight = templateDigit8p.rows;

//    Mat templateCanny;
//    cvtColor(template_mat, template_mat, COLOR_BGR2GRAY);
    //TODO: configure thresholds
//    Canny(templateDigit8p, templateCanny, 50, 50*4);

    // Only for test
    imageAnalizer.resetNextWindowPosition();
    imageAnalizer.showImage("multiScaleTemplateMatch: ", templateDigit8p);
//    imageAnalizer.showImage("multiScaleTemplateMatch: ", templateCanny);

    Mat target_resized;//, scaledCanny;

    const float SCALE_START = 1;
    const float SCALE_END = 0.2;
    const int SCALE_POINTS = 20;

    TemplateMatchInfo found;
    for(float scale = SCALE_START; scale >= SCALE_END; scale -= (SCALE_START - SCALE_END)/SCALE_POINTS){
        resize(src, target_resized, Size(0,0), scale, scale);

        // Break if target image becomes smaller than template
        if(templateWidth > target_resized.cols || templateHeight > target_resized.rows) break;

//        Canny(target_resized, scaledCanny, 50, 50*4);

        // Match template
        Mat result;
        matchTemplate(target_resized, templateDigit8p, result, TM_CCOEFF);
//        matchTemplate(scaledCanny, templateCanny, result, TM_CCOEFF);

        double maxVal; Point maxLoc;
        minMaxLoc(result, NULL, &maxVal, NULL, &maxLoc);

        // If better match found
        if( found.init == false || maxVal > found.maxVal ){
            found.init = true;
            found.maxVal = maxVal;
            found.maxLoc = maxLoc;
            found.scale = scale;
        }

        // START VISUALIZATION CODE
        Mat target_clone;
        resize(src, target_clone, Size(0,0), scale, scale);// Resize
        rectangle(target_clone, Point(maxLoc.x, maxLoc.y), Point(maxLoc.x + templateWidth, maxLoc.y + templateHeight), Scalar(0, 255, 255), 3);
        imshow("DBG", target_clone);
        waitKey(200);
        // END VISUALIZATION CODE
    }

    int startX, startY, endX, endY;
    startX = found.maxLoc.x / found.scale;
    startY = found.maxLoc.y / found.scale;

    endX= (found.maxLoc.x + templateWidth) / found.scale;
    endY= (found.maxLoc.y + templateHeight) / found.scale;

    Mat markedMatch;
    src.copyTo(markedMatch);

    // draw a bounding box around the detected result and display the image
    rectangle(markedMatch, Point(startX, startY), Point(endX, endY), Scalar(0, 0, 255), 3);

    imageAnalizer.showImage("multiScaleTemplateMatch, markedMatch", markedMatch);

    return Size(endX - startX, endY - startY);
}
