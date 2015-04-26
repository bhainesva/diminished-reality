#include <iostream>
#include "opencv2/opencv.hpp"
#include <opencv2/legacy/legacy.hpp>
#include <stdio.h>
#include <vector>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "pm_minimal.cpp"

using namespace std;
using namespace cv;

const char* winName="MyVideo";
char imgName[15];
vector<Point> ptVector;
Mat image;
Mat img;
bool backprojMode = false;
bool selectObject = false;
int trackObject = 0;
bool showHist = true;
Point origin;
Rect selection;
int vmin = 10, vmax = 256, smin = 30;
RNG rng(12345);

static void onMouse( int event, int x, int y, int, void* )
{
    if( selectObject )
    {
        selection.x = MIN(x, origin.x);
        selection.y = MIN(y, origin.y);
        selection.width = std::abs(x - origin.x);
        selection.height = std::abs(y - origin.y);

        selection &= Rect(0, 0, image.cols, image.rows);
    }

    switch( event )
    {
    case CV_EVENT_LBUTTONDOWN:
        origin = Point(x,y);
        selection = Rect(x,y,0,0);
        selectObject = true;
        break;
    case CV_EVENT_LBUTTONUP:
        selectObject = false;
        if( selection.width > 0 && selection.height > 0 )
            trackObject = -1;
        break;
    }
}

Mat getNeighbors(Mat img, Point p) {
    int x = p.x;
    int y = p.y;
    Rect ROI(Point(max(0,x-1), max(0, y-1)), Point(min(x+2, img.size[0]), min(img.size[1], y+2)));
	Mat ret = img(ROI);
	return ret;
}

int getAverageInPatches(Mat img, BITMAP *ann, int x, int y) {
    Mat imgclone = img.clone();
    int ax = max(0, x - patch_w);
    int ay = max(0, y - patch_w);
    //cout << "x: " << x << " y: " << y << endl;
    //cout << "patch_w: " << patch_w << endl;
    //cout << "ax: " << ax << " ay: " << ay << endl;
    int total = 0;
    int count = 1;
    for (int i=ax; i < x; i++) {
        for (int j=ay; j < y; j++) {
            int w = (*ann)[j][i];
            int u = INT_TO_X(w), v = INT_TO_Y(w);
            //circle(imgclone, Point(i, j), 4, 255);
            //circle(imgclone, Point(u + (x-i), v + (y-j)), 4, 1);
            total += img.at<unsigned char>(v + (y - j), u + (x - i));
            count++;
        }
    }
    return (int)(total/count);
}


void fill(Mat img, Mat mask) {
    for (int i=0;i < mask.size().width; i++) {
        for (int j=0;j < mask.size().height; j++) {
            mask.at<unsigned char>(j,i) = 0;
        }
    }
}

void fillAvg(Mat img, Mat mask, Point tl) {
    for (int i=0;i < mask.size().width; i++) {
        for (int j=0;j < mask.size().height; j++) {
            Mat neighbors = getNeighbors(img, Point(i+tl.x, j+tl.y));
            mask.at<unsigned char>(j,i) = (int)(sum(neighbors)[0]/countNonZero(neighbors));
        }
    }
}

void fillarea(Mat img, Rect bound) {
    //cvtColor(img, img, COLOR_BGR2HSV);
    // Initial implementation with a bounding box for area to be filled
    Mat roi = img(bound);
    fill(img, roi);
    fillAvg(img, roi, bound.tl());
    imshow("TEST", img);
    cout << "TEST";
    waitKey(0);

    cout << "Looping bounds." << endl;
    Mat aMat = img.clone();
    bound = bound + Size(patch_w, patch_w);
    rectangle(aMat, bound, Scalar(0), CV_FILLED); 
    // Now get correspondences from patchmatch
    BITMAP *a = createFromMat(aMat);
    BITMAP *roim = createFromMat(img);
    BITMAP *ann = NULL, *annd = NULL;
    patchmatch(roim, a, ann, annd);

    Mat imgC = img.clone();
    
    // Fill ROI; Each pixel gets average value from all corresponding patches it's a part of
    for (int ax = bound.tl().x; ax <= bound.br().x; ax++) {
        for (int ay = bound.tl().y; ay <= bound.br().y; ay++) {
            cout << "A" << (int)img.at<unsigned char>(ay, ax) << endl;
            cout << "B" << getAverageInPatches(img, ann, ax, ay) << endl;
            img.at<unsigned char>(ay,ax) = getAverageInPatches(img, ann, ax, ay);
        }
    }

    imshow("Clone", imgC);
    cout << "WAIT" << endl;
    waitKey(0);
    imshow("Filled", img);

    cout << "DONE" << endl;
    waitKey(0);
    //getAverageInPatch(img, ann, bound.tl().x, bound.tl().y);
    return;
}



int main(int argc, char* argv[])
{
    VideoCapture cap(0); // open the video file for reading

    if ( !cap.isOpened() )  // if not success, exit program
    {
         cout << "Cannot open the video file" << endl;
         return -1;
    }

    Rect trackWindow;
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;

    namedWindow( "Histogram", 0 );
    namedWindow( "CamShift Demo", 0 );
    setMouseCallback( "CamShift Demo", onMouse, 0 );
    createTrackbar( "Vmin", "CamShift Demo", &vmin, 256, 0 );
    createTrackbar( "Vmax", "CamShift Demo", &vmax, 256, 0 );
    createTrackbar( "Smin", "CamShift Demo", &smin, 256, 0 );

    Mat frame, hsv, gray, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
    bool paused = false;

    double fps = cap.get(CV_CAP_PROP_FPS); //get the frames per seconds of the video
    cout << "Frame per seconds : " << fps << endl;

    for (;;)
    {
        if( !paused )
        {
            cap >> frame;
            if( frame.empty() )
                break;
        }

        frame.copyTo(image);

        if( !paused )
        {
            cvtColor(image, hsv, COLOR_BGR2HSV);

            if( trackObject )
            {
                int _vmin = vmin, _vmax = vmax;

                inRange(hsv, Scalar(0, smin, MIN(_vmin,_vmax)),
                        Scalar(180, 256, MAX(_vmin, _vmax)), mask);
                int ch[] = {0, 0};
                hue.create(hsv.size(), hsv.depth());
                mixChannels(&hsv, 1, &hue, 1, ch, 1);

                if( trackObject < 0 )
                {
                    Mat roi(hue, selection), maskroi(mask, selection);
                    calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
                    normalize(hist, hist, 0, 255, CV_MINMAX);

                    trackWindow = selection;
                    trackObject = 1;

                    histimg = Scalar::all(0);
                    int binW = histimg.cols / hsize;
                    Mat buf(1, hsize, CV_8UC3);
                    for( int i = 0; i < hsize; i++ )
                        buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./hsize), 255, 255);
                    cvtColor(buf, buf, CV_HSV2BGR);

                    for( int i = 0; i < hsize; i++ )
                    {
                        int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows/255);
                        rectangle( histimg, Point(i*binW,histimg.rows),
                                   Point((i+1)*binW,histimg.rows - val),
                                   Scalar(buf.at<Vec3b>(i)), -1, 8 );
                    }
                }

                calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
                backproj &= mask;
                RotatedRect trackBox = CamShift(backproj, trackWindow,
                                    TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
                if( trackWindow.area() <= 1 )
                {
                    int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                    trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                       trackWindow.x + r, trackWindow.y + r) &
                                  Rect(0, 0, cols, rows);
                }

                if( backprojMode )
                    cvtColor( backproj, image, COLOR_GRAY2BGR );

                Point2f rect_points[4]; trackBox.points( rect_points );
                vector<vector<Point> > vec;
                vector<Point2f> pts2f(rect_points, rect_points + sizeof rect_points / sizeof rect_points[0]);
                vector<Point> pts;
                Mat(pts2f).copyTo(pts);
                vec.push_back(pts);
                Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );

                //Fill Area
                img = image.clone();
                img = img.setTo(Scalar(0));
                //fillPoly( img, vec, color );
                //for( int j = 0; j < 4; j++ )
                //    line( img, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
                //ellipse( img, trackBox, Scalar(0,0,255), -1, CV_AA );
                Rect bound = boundingRect(vec[0]);
                rectangle(img, bound, Scalar(255), CV_FILLED); 
                imshow( "Histogram", img );
                cvtColor( image, gray, COLOR_BGR2GRAY );
                fillarea(gray, bound);
                exit(0);
            }
        }
        else if( trackObject < 0 )
            paused = false;

        if( selectObject && selection.width > 0 && selection.height > 0 )
        {
            Mat roi(image, selection);
            bitwise_not(roi, roi);
        }

        img = image.clone();
        img = img.setTo(Scalar(255));
        imshow( "CamShift Demo", image );

        char c = (char)waitKey(10);
        if( c == 27 )
            break;
        switch(c)
        {
        case 'b':
            backprojMode = !backprojMode;
            break;
        case 'c':
            trackObject = 0;
            histimg = Scalar::all(0);
            break;
        case 'h':
            showHist = !showHist;
            if( !showHist )
                destroyWindow( "Histogram" );
            else
                namedWindow( "Histogram", 1 );
            break;
        case 'p':
            paused = !paused;
            break;
        default:
            ;
        }
    }

    return 0;
}

