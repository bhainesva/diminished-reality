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

void onClick(int event, int x, int y, int flags, void* userdata) {
    if  ( event == EVENT_LBUTTONDOWN ) {
        cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        vector<Point> *p = static_cast<vector<Point> *>(userdata);
        p->push_back(Point(x, y));
    }
}


void fillarea(Mat img, Rect bound) {
    Mat imgClone = img.clone();


    /*
    vector<Point*> changing;
    // Figure out which points need to change, ie inside the polygon defined by the contour
    // Faster way woud be to do fillPoly, then threshold, then only change the white (/black?) pixels 
    for (int j = 0; j < img.rows; j++) {
        for (int i = 0; i < img.cols; i++) {
            if (pointPolygonTest(contour[0], Point(i,j), false) != -1) {
                Point p = Point(i,j);
                int val = img.at<unsigned char>(p);
                cout << j << ", " << i << ": " << val << endl;
                changing.push_back(&p);
            } 
        }
    }
    */

    // Initial implementation with a bounding box for area to be filled
    rectangle( img, bound.tl(), bound.br(), (0, 255, 0), 2, 8, 0 );
    Mat bounded = Mat(img, bound);
    BITMAP* boundbit = createFromMat(bounded);
    save_bitmap(boundbit, "bounded.png");
    
    //Show in a window
    //namedWindow( "Contours", CV_WINDOW_AUTOSIZE );
    //imshow( "Contours", bounded);

    // Source = Img - Target
    // But since we don't want to get patches pointing to themselves, set target area to one color
    // in a and another in b, since patchmatch finds correspondences a => b
    Mat aMat = img.clone();
    Mat bMat = img.clone();

    cout << "Entering loop." << endl;
    int pmod = patch_w / 2;
    while (bound.width > pmod && bound.height > pmod) {
        cout << "Looping bounds." << endl;
        rectangle(bMat, bound, Scalar(255), CV_FILLED); 
        bound = bound + Point(pmod, pmod);
        bound -= Size(pmod, pmod);
        // Now get correspondences from patchmatch
        BITMAP *a = createFromMat(aMat);
        BITMAP *b = createFromMat(bMat);
        BITMAP *ann = NULL, *annd = NULL;
        patchmatch(a, b, ann, annd);
        save_bitmap(ann, "ann.png");
        save_bitmap(annd, "annd.png");
        Point start = bound.tl();
        Point end = bound.br();
        
        // Fill top and bottom rows
        for (int ay = start.y; ay <= end.y; ay += end.y - start.y) {
            for (int ax = start.x; ax < end.x; ax += patch_w) {
                int w = (*ann)[ay][ax];
                int u = INT_TO_X(w), v = INT_TO_Y(w);
                for (int j = 0; j < patch_w; j++) {
                    for (int i = 0; i < patch_w; i++) {
                        imgClone.at<unsigned char>(ay+j,ax+i) = 0;
                            //imgClone.at<unsigned char>(v+j,u+i);                    
                            //Uncomment this line to fill with Patchmatch 
                    }
                }
            }
        }
        imshow("Filled", imgClone);
        waitKey(0);
            //cout << "ax: " << ax << "\tay: " << ay << endl;
            //cout << "u: " << u << "\tv: " << v << endl;
            //cout << "Patch filled." << endl;
    }

    waitKey(0);
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

    Mat frame, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
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
                //fillarea(image, bound);
                imshow( "Histogram", img );
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

