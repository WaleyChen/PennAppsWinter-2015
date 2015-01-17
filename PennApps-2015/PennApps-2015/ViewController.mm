//
//  ViewController.m
//  PennApps-2015
//
//  Created by Jordan Brown on 1/16/15.
//  Copyright (c) 2015 woosufjordaline. All rights reserved.
//

#import "ViewController.h"
#import "VideoSource.h"
#import "UIImage+OpenCV.h"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#define WIDTH	640
#define HEIGHT	480
using namespace cv;

@interface ViewController () <VideoSourceDelegate>
@property (nonatomic, strong) VideoSource * videoSource;
@end

@implementation ViewController
@synthesize backgroundImageView;
- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    // Configure Video Source
    self.videoSource = [[VideoSource alloc] init];
    self.videoSource.delegate = self;
    [self.videoSource startWithDevicePosition:AVCaptureDevicePositionBack];
    
    
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
-(IBAction)uploadFile:(id)sender {
    NSString *urlString = @"http://158.130.175.0/get.php";
    NSString *filename = @"filename";
    request= [[NSMutableURLRequest alloc] init];
    [request setURL:[NSURL URLWithString:urlString]];
    [request setHTTPMethod:@"POST"];
    NSString *boundary = @"---------------------------14737809831466499882746641449";
    NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@",boundary];
    [request addValue:contentType forHTTPHeaderField: @"Content-Type"];
    NSMutableData *postbody = [NSMutableData data];
    [postbody appendData:[[NSString stringWithFormat:@"\r\n--%@\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
    [postbody appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"fileToUpload\"; filename=\"%@.png\"\r\n", filename] dataUsingEncoding:NSUTF8StringEncoding]];
    [postbody appendData:[@"Content-Type: application/octet-stream\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    UIImage *image = [UIImage imageNamed:@"Default.png"];
    [postbody appendData:UIImagePNGRepresentation(image)];
    [postbody appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
    [request setHTTPBody:postbody];
    NSLog(@"TEST");
    NSData *returnData = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
    NSLog(@"TEST 2");
    NSString *returnString = [[NSString alloc] initWithData:returnData encoding:NSUTF8StringEncoding];
    NSLog(@"%@", returnString);
}
-(IBAction)downloadInfo:(id)sender{
    NSString *string = [NSString stringWithContentsOfURL:[NSURL URLWithString:@"http://158.130.175.0/test.html"] encoding:NSUTF8StringEncoding error:nil];
    NSLog(@"%@",string);
    
}
#pragma mark -
#pragma mark VideoSource Delegate
-(UIImage *)LogoProcessing:(UIImage *)image{
    
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
    
    if (!cap.isOpened()) {return nil;}
    
    Mat img = [UIImage toCVMat:image];
    namedWindow("opencv", CV_WINDOW_AUTOSIZE);
    HOGDescriptor hog;
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
    
    vector<cv::Point> centroidCollections;
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
        
        vector<cv::Rect> found, foundFiltered;
        hog.detectMultiScale(img, found, 0, cv::Size(4, 4), cv::Size(32, 32), 1.05, 2);
        size_t i, j;
        for (i = 0; i < found.size(); i++) {
            cv::Rect r = found[i];
            for (j = 0; j < found.size(); j++) {
                if (j != i && (r & found[j]) == r) break;
            }	if (j == found.size())	foundFiltered.push_back(r);
        }
        
        Mat person;
        int tLX, tLY, bRX, bRY;
        if(foundFiltered.size() > 0) {
            for (i = 0; i < foundFiltered.size(); i++) {
                cv::Rect r = foundFiltered[i];
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
                
                
                person = img(cv::Rect(tLX, tLY, bRX - tLX, bRY - tLY));
                
                cv::Point rectCentroid;
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
                    
                    
                    if (goodMatches.size() > 3) {
                        rectangle(img, r.tl(), r.br(), Scalar(255, 255, 255),3);
                        centroidCollections.push_back(rectCentroid);
                    } else { rectangle(img, r.tl(), r.br(), Scalar(0, 255, 0), 3); }
                    
                } else {
                }
            }
        }
        imshow("opencv", img);
        counter++;
    }
}
- (void)frameReady:(VideoFrame)frame {
    __weak typeof(self) _weakSelf = self;
    dispatch_sync( dispatch_get_main_queue(), ^{
        // Construct CGContextRef from VideoFrame
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef newContext = CGBitmapContextCreate(frame.data,
                                                        frame.width,
                                                        frame.height,
                                                        8,
                                                        frame.stride,
                                                        colorSpace,
                                                        kCGBitmapByteOrder32Little |
                                                        kCGImageAlphaPremultipliedFirst);
        
        // Construct CGImageRef from CGContextRef
        CGImageRef newImage = CGBitmapContextCreateImage(newContext);
        CGContextRelease(newContext);
        CGColorSpaceRelease(colorSpace);
        
        // Construct UIImage from CGImageRef
        UIImage * image = [UIImage imageWithCGImage:newImage];
        CGImageRelease(newImage);
        if (image.size.width != 0) {
            cv::Mat originalImage = [UIImage toCVMat:image];
            cv::Mat image_copy;
            cvtColor(originalImage, image_copy, CV_BGRA2BGR);
            bitwise_not(image_copy, image_copy);
            cvtColor(image_copy, originalImage, CV_BGR2BGRA);
            image = [UIImage fromCVMat:image_copy];
            image = [UIImage imageWithCGImage:image.CGImage scale:image.scale orientation:UIImageOrientationRight];
            [[_weakSelf backgroundImageView] setImage:image];
        }
        
    });
}
-(IBAction)process:(id)sender{
    cv::Mat mat = [backgroundImageView.image toCVMat];
    
}
@end
