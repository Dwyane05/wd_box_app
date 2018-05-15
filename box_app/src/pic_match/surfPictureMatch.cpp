//#include "stdafx.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>  
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>  
#include <opencv2/features2d/features2d.hpp>
//#include <opencv2/xfeatures2d/cuda.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include "surfPictureMatch.h"

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

int surfFeatureDotMatch(Mat src, int s);

Rect rect1[] = { Rect(Point(62, 12), Point(170, 336)),Rect(Point(164, 16), Point(271, 190)),Rect(Point(272, 25), Point(367, 193)),
Rect(Point(367, 26), Point(456, 193)),Rect(Point(465, 32), Point(547, 195)) ,Rect(Point(172, 192), Point(276, 337)) ,
Rect(Point(271, 190), Point(360, 337)) ,Rect(Point(367, 193), Point(448, 339)) ,Rect(Point(456, 193), Point(531, 339)) ,
Rect(Point(80, 335), Point(181, 467)) ,Rect(Point(181, 338), Point(353, 470)) ,Rect(Point(360, 337), Point(438, 467)),
Rect(Point(448, 339), Point(518, 466)) };
vector<Mat> roi;
Mat test1 = imread("/root/sample/caiyang1.jpg");
Mat test2 = imread("/root/sample/caiyang2.jpg");
Mat test3 = imread("/root/sample/caiyang3.jpg");
Mat test4 = imread("/root/sample/caiyang4.jpg");
Mat test5 = imread("/root/sample/caiyang5.jpg");
Mat test6 = imread("/root/sample/caiyang6.jpg");
Mat test7 = imread("/root/sample/caiyang7.jpg");
Mat test8 = imread("/root/sample/caiyang8.jpg");
Mat test9 = imread("/root/sample/caiyang9.jpg");
Mat test10 = imread("/root/sample/caiyang10.jpg");
Mat test11 = imread("/root/sample/caiyang11.jpg");
Mat test12 = imread("/root/sample/caiyang12.jpg");
Mat test13 = imread("/root/sample/caiyang13.jpg");
Mat test14 = imread("/root/sample/caiyang14.jpg");
Mat test[14] = { test1,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11,test12,test13,test14 };
Mat frame;
Mat useless_frame;
vector<Mat> train_set;
vector<Mat> testDesc;

VideoCapture cap;

string IntToString(int &i)
{
  string s;
  stringstream ss(s);
  ss<<i;
  return ss.str();
}

bool pic_match_init( int camera )
{
//	clock_t start, finish;
	for (unsigned int i = 0; i < 14; i++){
		train_set.push_back(test[i]);
	}

	Ptr<SURF> surf;
	surf = SURF::create(300, 4, 3);
	vector<vector<KeyPoint> > keyPoint2;
	surf->detect(train_set, keyPoint2);
	surf->compute(train_set, keyPoint2, testDesc);

//	start = clock();

//	VideoCapture cap;
	cap.open(camera);
	if (!cap.isOpened()){
		cout << "OpenCV: camera open failed!" << endl;
		return false;
	}
	cout << "OpenCV: camera open success!" << endl;
	return true;

}

void grab_capture( bool newest )
{
	if( newest ){
		cap.read(frame);
	}else
		cap.read(useless_frame);
}

int num = 1;
bool get_good_num( std::string &result )
{
	int a = 0;
	int count[14] = { 0 };
	int GJ[7];

//	cap >> frame;
	bool newest = true;
	grab_capture( newest );
	if (frame.empty()){
		cout << "OpenCV: frame empty" << endl;
		return false;
	}
	cvtColor( frame, frame, COLOR_RGB2GRAY );
#if 0
	char temp[10];
	int n = 4;
	sprintf( temp, "wd%0*d.png", n, num++ );
	imwrite( temp, frame );
#endif
	roi.clear();
	for (unsigned int i = 0; i < 13; i++){
		//rectangle(src, rect1[i], Scalar(255, 0, 0));
		roi.push_back(frame(rect1[i]));
	}
	//imshow("src:", src);

	for (unsigned int j = 0; j < 13; j++){
		a=surfFeatureDotMatch(roi[j], 300);
		if(a<0) continue;
		count[a] += 1;

	}

	GJ[0] = count[0] + count[7];
	GJ[1] = count[1] + count[8];
	GJ[2] = count[2] + count[9];
	GJ[3] = count[3] + count[10];
	GJ[4] = count[4] + count[11];
	GJ[5] = count[5] + count[12];
	GJ[6] = count[6] + count[13];
	for ( int i = 0; i < 7; i++){
		cout << "count2 :" << GJ[i] << endl;
//		std::string s = s.to_string(GJ[i]);
//		result.append(s);
		int &temp=GJ[i];
		result+=IntToString(temp);
	}
	cout << "result: " << result << endl;
	memset(GJ, 0, sizeof(GJ));
//	finish = clock();
//	cout << finish - start << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	return true;
}
#if 0
int main() {

	clock_t start, finish;
	
	for (unsigned int i = 0; i < 14; i++)
	{
		train_set.push_back(test[i]);
	}
	Ptr<SURF> surf;
	surf = SURF::create(300, 4, 3);
	vector<vector<KeyPoint> > keyPoint2;
	surf->detect(train_set, keyPoint2);
	surf->compute(train_set, keyPoint2, testDesc);
	start = clock();
	VideoCapture cap;
	cap.open(0);
	if (!cap.isOpened())
	{
		cout << "OpenCV: camera open failed!" << endl;
		return false;
	}

	//char key;
	//char filename[200];
	int a = 0;
	int count[14] = { 0 };
	int GJ[7];
	//while (1)
	//{
		//key = waitKey(50);
		cap >> frame;
		if (frame.empty())
			cout << "OpenCV: frame empty" << endl;
		//imshow("1", frame);
		//if (key == 27)
			//break;
		//if (key == 32)
		//{

			//sprintf_s(filename, "src1.jpg");
			//imwrite(filename, frame);
			//Mat src = imread("src1.jpg");
			for (unsigned int i = 0; i < 13; i++)
			{
				//rectangle(src, rect1[i], Scalar(255, 0, 0));
				roi.push_back(frame(rect1[i]));
			}
			//imshow("src:", src);
			
			for (unsigned int j = 0; j < 13; j++)
			{
				a=surfFeatureDotMatch(roi[j], 300);
				if(a<0) continue;
				count[a] += 1;
			
			}

			GJ[0] = count[0] + count[7];
			GJ[1] = count[1] + count[8];
			GJ[2] = count[2] + count[9];
			GJ[3] = count[3] + count[10];
			GJ[4] = count[4] + count[11];
			GJ[5] = count[5] + count[12];
			GJ[6] = count[6] + count[13];
			for (unsigned int i = 0; i < 7; i++)
			{
				cout << "count2 :" << GJ[i] << endl;
			}
			memset(GJ, 0, sizeof(GJ));
			finish = clock();
			cout << finish - start << "/" << CLOCKS_PER_SEC << "(s)" << endl;
		//}

	//}

	//waitKey();
	return 0;

}
#endif

int surfFeatureDotMatch(Mat src, int s)
{
	Mat src01;
	src01 = src.clone();
	Ptr<SURF> surf;
	surf = SURF::create(s, 4, 3);
	vector<KeyPoint> keyPoint1;
	Mat srcDesc;
	surf->detectAndCompute(src01, Mat(), keyPoint1, srcDesc);
	if (keyPoint1.size() == 0) return -1;
	FlannBasedMatcher matcher;
	vector<DMatch> matches;
	vector<vector<DMatch> > knnMatches;

	matches.clear();
	const float minRatio = 1.f / 1.5f;
	matcher.knnMatch(srcDesc, testDesc, knnMatches, 2);
	cout << "knnMatch:" << knnMatches.size() << endl;
	if (knnMatches.size() == 0) return -1;

	for (unsigned i = 0; i < knnMatches.size(); i++)
	{
		const DMatch& bestMatch = knnMatches[i][0];
		const DMatch& betterMatch = knnMatches[i][1];

		float distanceRatio = bestMatch.distance / betterMatch.distance;
		if (distanceRatio < minRatio)
		{
			matches.push_back(bestMatch);
		}
	}
	if (matches.size() == 0|| matches.size()==1 || matches.size()==2) return -1;
	int *count1=new int[matches.size()]();
	//int key = 0;
	for (unsigned int i = 0; i < matches.size(); i++)
	{
		cout << "count:" << matches[i].imgIdx << endl;
		count1[i]=matches[i].imgIdx;
	}
//	if (matches.size()==1) return count1[0];
	int b=0;
	int count = 1;
	int temp = 1;
	//����count1�г��ִ���������
	for (unsigned int i=0; i < matches.size(); i++)
	{
		if (count1[i] == count1[i + 1]) {
			temp++;
			if (temp > count) {
				count = temp;
				b = count1[i];
			}
		}
		else {
			temp = 1;
		}
	}
	delete[]count1;
	//map<int, int> keyList;
	//vector<int>::iterator iter;
	//for (; iter != count1.end(); ++iter)//����count1�г��ִ���������
	//{
	//	keyList[*iter]++;
	//}
	//map<int, int>::iterator iter1;
	//int maxValue = 0;
	//int key = -1;
	//for (; iter1 != keyList.end(); ++iter1)
	//{
	//	int temp = 0;
	//	temp = iter1->second;
	//	if (temp >= maxValue)
	//	{
	//		maxValue = temp;
	//		key = iter1->first;
	//	}
	//}
	
	return b;

}
