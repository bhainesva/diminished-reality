#include <iostream>
#include "opencv2/opencv.hpp"
#include <opencv2/legacy/legacy.hpp>
#include <stdio.h>
#include <vector>
#include "opencv2/highgui/highgui.hpp"
#include "pm_minimal.cpp"

using namespace std;
using namespace cv;

Mat src,img,ROI;
int ialpha = 20;
int ibeta=20; 
int igamma=20;

const char* winName="MyVideo";
int i=0;
char imgName[15];
vector<Point> ptVector;

void onClick(int event, int x, int y, int flags, void* userdata) {
    if  ( event == EVENT_LBUTTONDOWN ) {
        cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        vector<Point> *p = static_cast<vector<Point> *>(userdata);
        p->push_back(Point(x, y));
    }
}

int main(int argc, char* argv[])
{
    check_im();
    VideoCapture cap("starburst.mp4"); // open the video file for reading

    if ( !cap.isOpened() )  // if not success, exit program
    {
         cout << "Cannot open the video file" << endl;
         return -1;
    }

    double fps = cap.get(CV_CAP_PROP_FPS); //get the frames per seconds of the video

     cout << "Frame per seconds : " << fps << endl;

    namedWindow(winName, CV_WINDOW_AUTOSIZE); //create a window called "MyVideo"
    Mat src;
    bool bSuccess = cap.read(src); // read a new frame from video
    namedWindow(winName,WINDOW_NORMAL);
    setMouseCallback(winName, onClick,(void*)&ptVector);
    imshow(winName,src);

    while(1) {
        char c=waitKey();
        if(c==27) break;

    }
    //
    // Draw the contour
    vector<vector<Point> > conts;
    conts.push_back(ptVector);
    drawContours( src, conts, -1, Scalar(128));
    imshow(winName, src);
    waitKey();

    // Snakes parameters
    //float alpha=ialpha/100.0f; 
    //float beta=ibeta/100.0f; 
    //float gamma=igamma/100.0f;
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
    //criteria.type=CV_TERMCRIT_ITER|CV_TERMCRIT_k; 
    //criteria.maxCount = 1000;
    //criteria.epsilon = .01;
    cvSnakeImage( img, cvPtArr, length, &alpha,&beta,&gamma,CV_VALUE,size,criteria,0 ); 
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
    waitKey();

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
        TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 200, 0.001); 
        cvSnakeImage( img, cvPtArr, length, &alpha,&beta,&gamma,CV_VALUE,size,criteria,0 ); 
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


    return 0;

}
