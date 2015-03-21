#include <iostream>
#include "opencv2/opencv.hpp"
#include <opencv2/legacy/legacy.hpp>
#include <stdio.h>
#include <vector>

using namespace std;
using namespace cv;


Mat src,img,ROI;
int ialpha = 20;
int ibeta=20; 
int igamma=20;

const char* winName="Contour Image";
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


int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "Usage: display_image ImageName" << endl;
        return -1;
    }

    cout<<"Click to select contour points."<<endl<<endl;

    cout<<"------> Press 'Esc' to quit"<<endl<<endl;


    src = imread(argv[1], CV_LOAD_IMAGE_COLOR);

    namedWindow(winName,WINDOW_NORMAL);
    setMouseCallback(winName,onClick,(void*)&ptVector);
    imshow(winName,src);

    while(1) {
        char c=waitKey();
        if(c=='s'&&ROI.data){
            sprintf(imgName,"%d.jpg",i++);
            imwrite(imgName,ROI);
            cout<<"  Saved "<<imgName<<endl;
        }

        if(c==27) break;

    }


    // Draw the contour
    vector<vector<Point> > conts;
    conts.push_back(ptVector);
    drawContours( src, conts, -1, Scalar(128));
    imshow(winName, src);
    waitKey();

    // Snakes parameters
    float alpha=ialpha/100.0f; 
    float beta=ibeta/100.0f; 
    float gamma=igamma/100.0f;

    int length = ptVector.size();
    CvPoint cvPtArr[length];
    for (int i = 0; i < length; i++) {
        cvPtArr[i] = CvPoint(ptVector.at(i));
    }

    cvtColor(src, src, CV_BGR2GRAY);
    IplImage copy = src;
    IplImage* img = &copy;

    Size size;
    size.width = 3;
    size.height = 3;
    TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 1000, 0.01); 
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
    imshow(winName, src);
    waitKey();
    return 0;
}
