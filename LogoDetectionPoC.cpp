#include <stdio.h>
#include <iostream>
#include "/usr/local/include/opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#define WIDTH	640
#define HEIGHT	480

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
	Mat	PennAppsLogo	= imread("NameCheap.png", CV_LOAD_IMAGE_COLOR);
	int	minHessian		= 500;
	
	SiftFeatureDetector	detector(minHessian);
	vector<KeyPoint>	logoKeyPoints;
	detector.detect(PennAppsLogo, logoKeyPoints);
	
	SiftDescriptorExtractor	extractor;
	Mat						logoDescriptor;
	extractor.compute(PennAppsLogo, logoKeyPoints, logoDescriptor);
	
	FlannBasedMatcher matcher;
	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

	if (!cap.isOpened()) {return -1;}
	
	Mat img;
	namedWindow("opencv", CV_WINDOW_AUTOSIZE);
	HOGDescriptor hog;
	hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	
	vector<Point> centroidCollections;
	Mat lastFrame;
	int counter = 0;
	
	while(true) {
		cap >> img;
		cap >> lastFrame;
		
		Mat personDescriptor, personMatches;
		vector<KeyPoint> personKeyPoint;
		vector<vector<DMatch> > matches;
		vector<DMatch> goodMatches;

		if (img.empty()) continue;
		
		vector<Rect> found, foundFiltered;
		hog.detectMultiScale(img, found, 0, Size(4, 4), Size(32, 32), 1.05, 2);
		size_t i, j;
		for (i = 0; i < found.size(); i++) {
			Rect r = found[i];
			for (j = 0; j < found.size(); j++) {
				if (j != i && (r & found[j]) == r) break;
			}	if (j == found.size())	foundFiltered.push_back(r);
		}
		
		Mat person; 
		int tLX, tLY, bRX, bRY;
		if(foundFiltered.size() > 0) {
			for (i = 0; i < foundFiltered.size(); i++) {
				Rect r = foundFiltered[i];
				r.x = r.x + cvRound(r.width * 0.1);
				r.width = cvRound(r.width * 0.8);
				r.y = r.y + cvRound(r.height * 0.07);
				r.height = cvRound(r.height * 0.8);

				tLX = r.tl().x;
				tLY = r.tl().y;
				bRX = r.br().x;
				bRY = r.br().y;

				if(tLX < 1) {tLX = 1;}
				if(tLY < 1) {tLY = 1;}
				if(bRX > WIDTH) {bRX = WIDTH;}
				if(bRY > HEIGHT) {bRY = HEIGHT;}

				cout << "Filter size " << foundFiltered.size() << endl;
				cout << tLX << "," << tLY << " | " << bRX - tLX << "," << bRY - tLY << endl;

				person = img(Rect(tLX, tLY, bRX - tLX, bRY - tLY));

				Point rectCentroid;
				rectCentroid.x = ((bRX - tLY) / 2);
				rectCentroid.y = ((bRX - tLY) / 2);

				Mat personGrayScale;
				cvtColor(person, personGrayScale, CV_BGR2GRAY);

				detector.detect(personGrayScale, personKeyPoint);
				extractor.compute(personGrayScale, personKeyPoint, personDescriptor);

				if(!(personDescriptor.empty())) {
					matcher.knnMatch(logoDescriptor, personDescriptor, matches, 2);
					for(int i = 0; i < min(personDescriptor.rows - 1, (int)matches.size()); i++) {
						if((matches[i][0].distance	< 0.7 * (matches[i][1].distance))
							 && ((int) matches[i].size() <= 2 && (int) matches[i].size() > 0)) { 
								 goodMatches.push_back(matches[i][0]);
							}
					}

					cout << "Good Matches: " << goodMatches.size() << endl;

					if (goodMatches.size() > 3) {
						rectangle(img, r.tl(), r.br(), Scalar(255, 255, 255),3);
						centroidCollections.push_back(rectCentroid);
					} else { rectangle(img, r.tl(), r.br(), Scalar(0, 255, 0), 3); }

				} else {
					cout << "Person Descriptor is empty !!!" << endl;
					cout << "Skipping current frame" << endl;
				}
			}
		}
		imshow("opencv", img);
		counter++;
		if(waitKey(10) >= 0) break;
	}
	while (true) {
		for (int i = 0; i < centroidCollections.size() - 1; i++) {
			Point a, b;
			a.x = centroidCollections[i].x;
			a.y = centroidCollections[i].y;
			b.x = centroidCollections[i + 1].x;
			b.y = centroidCollections[i + 1].y;
			line(lastFrame, a, b, Scalar(255, 255, 255), 3, 8, 0);
		}	imshow("Traversed Path", lastFrame);
		if(waitKey(10) >= 0) break;
	} return 0;
}
