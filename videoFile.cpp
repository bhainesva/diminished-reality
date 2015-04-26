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

void onClick(int event, int x, int y, int flags, void* userdata) {
    if  ( event == EVENT_LBUTTONDOWN ) {
        cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        vector<Point> *p = static_cast<vector<Point> *>(userdata);
        p->push_back(Point(x, y));
    }
}

Mat getNeighbors(Mat img, Point p) {
    int x = p.x;
    int y = p.y;
    Rect ROI(Point(max(0,x-1), max(0, y-1)), Point(min(x+2, img.size[0]), min(img.size[1], y+2)));
	Mat ret = img(ROI);
	return ret;
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

void fillarea(Mat img, vector<vector<Point> > contour) {
    double area = contourArea(contour[0]);
    cout << "Area: " << area << endl;
    cout << "Depth: " << img.depth() << endl;


    // Initial implementation with a bounding box for area to be filled
    Rect bound = boundingRect( Mat(contour[0]) );
    Mat roi = img(bound);
    fill(img, roi);
    fillAvg(img, roi, bound.tl());
    //imshow("TEST", img);
    //cout << "TEST";
    //waitKey(0);

    cout << "Looping bounds." << endl;
    Mat aMat = img.clone();
    bound = bound + Size(patch_w, patch_w);
    rectangle(aMat, bound, Scalar(0), CV_FILLED); 
    // Now get correspondences from patchmatch
    BITMAP *a = createFromMat(aMat);
    BITMAP *roim = createFromMat(img);
    BITMAP *ann = NULL, *annd = NULL;
    patchmatch(roim, a, ann, annd);
    
    // Fill top and bottom rows
    for (int ax = 0; ax <= roi.size().width-patch_w; ax += patch_w) {
        for (int ay = 0; ay <= roi.size().height-patch_w; ay += patch_w) {
            int w = (*ann)[ay+bound.tl().y][ax+bound.tl().x];
            int u = INT_TO_X(w), v = INT_TO_Y(w);
            for (int j = 0; j <= patch_w; j++) {
                for (int i = 0; i <= patch_w; i++) {
                    roi.at<unsigned char>(ay+j,ax+i) = aMat.at<unsigned char>(v+j,u+i);                    
                }
            }
        }
    }

    imshow("Filled", img);

    cout << "DONE" << endl;
    waitKey(0);
    return;
}


int main(int argc, char* argv[])
{
    VideoCapture cap("starburst.mp4"); // open the video file for reading

    if ( !cap.isOpened() )  // if not success, exit program
    {
         cout << "Cannot open the video file" << endl;
         return -1;
    }

    double fps = cap.get(CV_CAP_PROP_FPS); //get the frames per seconds of the video
    cout << "Frame per seconds : " << fps << endl;

    namedWindow(winName, CV_WINDOW_AUTOSIZE); //create a window 
    Mat src;
    bool bSuccess = cap.read(src); // read a new frame from video
    namedWindow(winName,WINDOW_NORMAL);
    
    // Make clicks add points to vector
    setMouseCallback(winName, onClick,(void*)&ptVector);
    imshow(winName,src);
    // Add points to vector until ESC
    while(1) {
        char c=waitKey();
        if(c==27) break;
    }

    // Draw the contour
    vector<vector<Point> > conts;
    conts.push_back(ptVector);
    Mat src_contourlines = src.clone();
    drawContours( src_contourlines, conts, -1, Scalar(128));
    imshow(winName, src_contourlines);

    // Snakes parameters
    float alpha=0.1f;
    float beta=0.4f;
    float gamma=0.5f;

    int length = ptVector.size();
    CvPoint cvPtArr[length];
    for (int i = 0; i < length; i++) {
        cvPtArr[i] = CvPoint(ptVector.at(i));
    }

    cvtColor(src, src, CV_BGR2GRAY);
    IplImage copy = src;
    IplImage* img = &copy;

    Size size;
    size.width = 19;
    size.height = 19;
    TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 1000, 0.001); 
    cvSnakeImage( img, cvPtArr, length, &alpha,&beta,&gamma,CV_VALUE,size,criteria, true ); 
    src = Mat(img);

    // Just Draw the contour
    vector<Point> cvPtVector;
    for (int i=0;i < length; i++) {
        cvPtVector.push_back(Point(cvPtArr[i]));
    }

    vector<vector<Point> > conts2;
    conts2.push_back(cvPtVector);
    Mat copysrc = src.clone();
    fillarea(copysrc, conts2);

    drawContours( src, conts2, -1, Scalar(0));
    fillPoly( src, conts2, Scalar(128));
    imshow(winName, src);

    // Temporary comment while working on fillarea
    /*
    while(1) {
        Mat src;

        bool bSuccess = cap.read(src); // read a new frame from video

        if (!bSuccess) {//if not success, break loop
            cout << "Cannot read the frame from video file" << endl;
            break;
        }

       // for (int i = 0; i < length; i++) {
       //     cvPtArr[i] = CvPoint(ptVector.at(i));
       // }

        cvtColor(src, src, CV_BGR2GRAY);
        IplImage copy = src;
        IplImage* img = &copy;

        Size size;
        size.width = 19;
        size.height = 19;
        TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 20, 0.001); 
        cvSnakeImage( img, cvPtArr, length, &alpha,&beta,&gamma,CV_VALUE,size,criteria, false); 
        src = Mat(img);

        // Just Draw the contour
        vector<vector<Point> > conts2;
        vector<Point> cvPtVector;
        for (int i=0;i < length; i++) {
            cvPtVector.push_back(Point(cvPtArr[i]));
        }
        conts2.push_back(cvPtVector);
        drawContours( src, conts2, -1, Scalar(0));
        fillPoly( src, conts2, Scalar(128));
        imshow(winName, src);

        if(waitKey(30) == 27){ //wait for 'esc' key press for 30 ms. If 'esc' key is pressed, break loop
            cout << "esc key is pressed by user" << endl;
            break; 
        }
    }

    */

    cout << endl;
    return 0;

}
