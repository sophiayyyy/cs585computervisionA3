//opencv libraries
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
//C++ standard libraries
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

const static int SENSITIVITY_VALUE = 20;
const static int BLUR_SIZE = 10;
const static int HEIGTH_SIZE = 500;
const static int WIDTH_SIZE = 500;

// the upper limit and lower limit of a hand area
const static double HAND_MAXAREA = 100000.0;
const static double HAND_MINAREA = 30000.0;

Rect objectBoundingRectangle = Rect(0, 0, 0, 0);
vector<vector<vector<Point>>> tplContours;
//max coutour number
int number[4] = { 0 };

//function
void mySkinDetect(Mat& src, Mat& dst);
vector<vector<Point>> getContour(Mat& src, Mat& output);
void morphological(Mat& src);
int myMax(int a, int b, int c);
int myMin(int a, int b, int c);
void templateMatching(vector<Point> contour, Mat& frame, int index);

int main() {
    
    // template image prepare
    vector<Mat> tpls = { imread("palm.jpg", 1), imread("three.jpg", 1), imread("eight.jpg", 1), imread("thumb_up.jpg", 1) };
    vector<Mat> tplSkin = tpls;
    vector<Mat> tplDrawings = tpls;
    vector<vector<Vec4i>> tplHierarchies;
    
    for (int i = 0; i < 4; i++) {
        resize(tpls[i], tpls[i], Size(WIDTH_SIZE, HEIGTH_SIZE), (0, 0), (0, 0));
        tplSkin[i] = Mat::zeros(tpls[i].rows, tpls[i].cols, CV_8UC1);
        mySkinDetect(tpls[i], tplSkin[i]);
        tplDrawings[i] = Mat::zeros(tpls[i].size(), CV_8UC3);
    }
    
    for (int i = 0; i < 4; i++) {
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        threshold(tplSkin[i], tplSkin[i], SENSITIVITY_VALUE, 255, THRESH_BINARY);
        morphological(tplSkin[i]);
        
        //namedWindow("threshold" + to_string(i), CV_WINDOW_AUTOSIZE);
        imshow("threshold" + to_string(i), tplSkin[i]);
        
        findContours(tplSkin[i], contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        tplContours.push_back(contours);
        tplHierarchies.push_back(hierarchy);
        
    }

    
    for (int i = 0; i < 4; i++) {
        int maxsize = 0;
        int maxind = 0;
        for (int j = 0; j < tplContours[i].size(); j++) {
            double area = contourArea(tplContours[i][j]);
            if (area > maxsize) {
                maxsize = area;
                maxind = j;
            }
        }
        number[i] = maxind;
        drawContours(tplDrawings[i], tplContours[i], maxind, Scalar(0, 0, 255), 2, 8, tplHierarchies[i]);
        //namedWindow("Contours" + to_string(i), CV_WINDOW_AUTOSIZE);
        imshow("Contours" + to_string(i), tplDrawings[i]);
    }
    
    
    VideoCapture cap(0);
    if (!cap.isOpened())
    {
        cout << "Cannot open the video cam" << endl;
        return -1;
    }
    
    namedWindow("MyVideo0", WINDOW_AUTOSIZE);
    Mat frame0;
    
    // read a new frame from video
    bool bSuccess0 = cap.read(frame0);
    resize(frame0, frame0, Size(WIDTH_SIZE, HEIGTH_SIZE), (0, 0), (0, 0));
    //if not successful, break loop
    if (!bSuccess0)
    {
        cout << "Cannot read a frame from video stream" << endl;
    }
    
    //record number of frames
    int count = 0;
    Mat background;
    Mat skin_result;
    Mat remove_result;
    
    Mat temp;
    Mat temp_result;
    
    //show the frame in "MyVideo" window
    imshow("MyVideo0", frame0);
    while (1) {
        // read a new frame from video
        Mat frame;
        bool bSuccess = cap.read(frame);
        
        //if not successful, break loop
        if (!bSuccess)
        {
            cout << "Cannot read a frame from video stream" << endl;
            break;
        }
        
        resize(frame, frame, Size(WIDTH_SIZE, HEIGTH_SIZE), (0, 0), (0, 0));
        count++;
        if (count == 1) {  //let the first one become the background
            background = frame.clone();
        }
        
        //detect the skin in current image
        skin_result = Mat::zeros(frame.rows, frame.cols, CV_8UC1);
        mySkinDetect(frame, skin_result);
        imshow("Skin", skin_result);
        
        //draw contour of  frame
        Mat contour_output;
        vector<vector<Point>> frame_contours;
        vector<vector<Point>> handCandidates;
        vector<int> candidatesMaxIndex;
        frame_contours = getContour(skin_result, contour_output);
        imshow("Contours", contour_output);
        //: Find largest contour
        
        for(int i = 0; i < frame_contours.size(); i++){
            double area = contourArea(frame_contours[i]);
            
            cout << "area = " << area << endl;
            
            if(area < HAND_MAXAREA && area > HAND_MINAREA){
                handCandidates.push_back(frame_contours[i]);
                candidatesMaxIndex.push_back(i);
            }
        }
        
        Rect boundrec;
        //template matching
        if (count > 20 && handCandidates.size() != 0) {
            for(int i = 0; i < handCandidates.size(); i++){
                templateMatching(handCandidates[i], frame, i);
                boundrec = boundingRect(handCandidates[i]);
                rectangle(frame, boundrec, Scalar(0, 255, 0), 5, 8, 0);
            }
        }
        
        
        //show the frame in "MyVideo" window
        imshow("MyVideo0", frame0);
        frame0 = frame;
        if (waitKey(30) == 27)
        {
            cout << "esc key is pressed by user" << endl;
            break;
        }
    }
    cap.release();
    return 0;
}

//Function that returns the maximum of 3 integers
int myMax(int a, int b, int c) {
    int m = a;
    (void)((m < b) && (m = b)); //short-circuit evaluation
    (void)((m < c) && (m = c));
    return m;
}

//Function that returns the minimum of 3 integers
int myMin(int a, int b, int c) {
    int m = a;
    (void)((m > b) && (m = b));
    (void)((m > c) && (m = c));
    return m;
}
void mySkinDetect(Mat& src, Mat& dst) {
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            //For each pixel, compute the average intensity of the 3 color channels
            Vec3b intensity = src.at<Vec3b>(i, j); //Vec3b is a vector of 3 uchar (unsigned character)
            int B = intensity[0]; int G = intensity[1]; int R = intensity[2];
            if ((R > 95 && G > 40 && B > 20) && (myMax(R, G, B) - myMin(R, G, B) > 15) && (abs(R - G) > 15) && (R > G) && (R > B)) {
                dst.at<uchar>(i, j) = 255;
            }
        }
    }
}

vector<vector<Point>>  getContour(Mat& src, Mat& output) {
    Mat thres_output;
    threshold(src, thres_output, SENSITIVITY_VALUE, 255, THRESH_BINARY);
    blur(thres_output, thres_output, Size(BLUR_SIZE, BLUR_SIZE));
    threshold(thres_output, thres_output, SENSITIVITY_VALUE, 255, THRESH_BINARY);
    morphological(thres_output);
   
    output = Mat::zeros(thres_output.size(), CV_8UC3);
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(thres_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    
    //: Find largest contour
    int maxsize = 0;
    int maxind = 0;
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        if (area > maxsize) {
            maxsize = area;
            maxind = i;
        }
    }
    drawContours(output, contours, maxind, Scalar(0, 0, 255), 2, 8, hierarchy);
    return contours;
}

void morphological(Mat& src) {
    Mat element = getStructuringElement(MORPH_RECT, Size(4, 4));
    Mat element2 = getStructuringElement(MORPH_RECT, Size(8, 8));
    medianBlur(src, src, 5);
    erode(src, src, element);
//    dilate(src, src, element2);
}

void templateMatching(vector<Point> contour, Mat& frame, int index){
    double matchrate1 = matchShapes(contour, tplContours[0][number[0]], CV_CONTOURS_MATCH_I1, 0.0);
    cout << "index1=" << 1 << "---" << matchrate1 << endl;
    double matchrate2 = matchShapes(contour, tplContours[1][number[1]], CV_CONTOURS_MATCH_I1, 0.0);
    cout << "index2=" << 2 << "---" << matchrate2 << endl;
    double matchrate3 = matchShapes(contour, tplContours[2][number[2]], CV_CONTOURS_MATCH_I1, 0.0);
    cout << "index3=" << 3 << "---" << matchrate3 << endl;
    double matchrate4 = matchShapes(contour, tplContours[3][number[3]], CV_CONTOURS_MATCH_I1, 0.0);
    cout << "index4=" << 4 << "---" << matchrate3 << endl;
    if (matchrate1 < matchrate2 && matchrate1 < matchrate3 && matchrate1 < matchrate4) {
        putText(frame, "palm", Point(30, 70 + index*80), CV_FONT_HERSHEY_COMPLEX, 2, Scalar(0, 0, 255), 2, 8);
    }
    if (matchrate2 < matchrate1 && matchrate2 < matchrate3 && matchrate2 < matchrate4) {
        putText(frame, "three", Point(30, 70 + index*80), CV_FONT_HERSHEY_COMPLEX, 2, Scalar(0, 0, 255), 2, 8);
    }
    if (matchrate3 < matchrate1 && matchrate3 < matchrate2 && matchrate3 < matchrate4) {
        putText(frame, "eight", Point(30, 70 + index*80), CV_FONT_HERSHEY_COMPLEX, 2, Scalar(0, 0, 255), 2, 8);
    }
    if (matchrate4 < matchrate1 && matchrate4 < matchrate2 && matchrate4 < matchrate3) {
        putText(frame, "thumb_up", Point(30, 70 + index*80), CV_FONT_HERSHEY_COMPLEX, 2, Scalar(0, 0, 255), 2, 8);
    }
}

