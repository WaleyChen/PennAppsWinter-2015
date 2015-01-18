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
#import <opencv2/opencv.hpp>
#include "cvneon.h"
#import "ObjectTrackingClass.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#include <math.h>
#define WIDTH	640
#define HEIGHT	480
using namespace cv;
@interface ViewController () <VideoSourceDelegate>
@property (nonatomic, strong) VideoSource * videoSource;
@end

@implementation ViewController
@synthesize backgroundImageView;
@synthesize fromImage;
@synthesize realTime;
@synthesize overlay;
@synthesize topText;
@synthesize back;
@synthesize cmt;
@synthesize scrollView;
@synthesize infoDisplay;
- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    // Configure Video Source
    self.videoSource = [[VideoSource alloc] init];
    self.videoSource.delegate = self;
    [self.videoSource startWithDevicePosition:AVCaptureDevicePositionBack];
    self.navigationController.title = @"The World";
    blur = true;
    [topText setAlpha:0];
    [back setAlpha:0];
    sigView = [[CustomSignatureView alloc]initWithFrame:CGRectMake(0, 0, 1242, 2208)];
    sigView.backgroundColor = [UIColor clearColor];
    trackerView = [[Tracker alloc]initWithFrame:CGRectMake(0, 0, 1, 1)];
    trackerView.backgroundColor = [UIColor clearColor];
    sigView.vc = self;
    [self.backgroundImageView addSubview:sigView];
    [self.view addSubview:trackerView];
    shouldBlurTransition = false;
    shouldProcess = false;
    scrollView = [[UIScrollView alloc]initWithFrame:CGRectMake(0, 400, 350, 200)];
    scrollView.layer.cornerRadius = 5.0;
    scrollView.layer.masksToBounds = YES;
    [scrollView setAlpha:0];
    sigView.userInteractionEnabled = NO;
    shouldDownload = false;
    timer = [[NSTimer alloc]init];
    
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
-(cv::Mat)generateReferenceFrame{
    return [UIImage toCVMat:self.backgroundImageView.image];
}
-(IBAction)uploadFile:(UIImageView *)imageView sizeOfImage:(CGRect)rectangle {
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
    CGRect rect = [self.view bounds];
    UIGraphicsBeginImageContext(rect.size);
    CGContextRef context = UIGraphicsGetCurrentContext();
    [self.view.layer renderInContext:context];
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    img = [self imageByCropping:img toRect:rectangle];
    if (img.size.width < 40 || img.size.height < 40) {
        NSLog(@"Too small");
        return;
    }
    [postbody appendData:[@"Content-Type: application/octet-stream\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
    [postbody appendData:UIImagePNGRepresentation(img)];
    [postbody appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
    [request setHTTPBody:postbody];
    NSData *returnData = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
    NSString *returnString = [[NSString alloc] initWithData:returnData encoding:NSUTF8StringEncoding];
    NSLog(@"%@", returnString);
    timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(downloadInfo:) userInfo:nil repeats:YES];
    shouldDownload = true;
}
- (UIImage *)imageByCropping:(UIImage *)imageToCrop toRect:(CGRect)rect {
    CGImageRef cropped = CGImageCreateWithImageInRect(imageToCrop.CGImage, rect);
    UIImage *retImage = [UIImage imageWithCGImage: cropped];
    CGImageRelease(cropped);
    return retImage;
}
-(IBAction)downloadInfo:(id)sender{
    NSLog(@"Checking");
    if (!shouldDownload) {
        return;
    }
    UIImage *image = [UIImage imageWithData:[NSData dataWithContentsOfURL:[NSURL URLWithString:@"http://158.130.175.0/uploads/out.jpg"]]];
    NSLog(@"%@",image);
    if (image != NULL) {
        shouldDownload = false;
        [timer invalidate];
        NSLog(@"Done");
    }
    image = [self imageByCropping:image toRect:CGRectMake(0, 35, 350, image.size.height)];
    
    infoDisplay = [[UIImageView alloc]initWithImage:image];
    infoDisplay.layer.cornerRadius = 5.0;
    infoDisplay.layer.masksToBounds = YES;
    [scrollView addSubview:infoDisplay];
    [scrollView setContentSize:infoDisplay.bounds.size];
    scrollView.userInteractionEnabled = true;
    scrollView.scrollEnabled = true;
    [self.view addSubview:scrollView];
    scrollView.center = CGPointMake(208, 600);
    [UIView animateWithDuration:0.75f animations:^{
        [scrollView setAlpha:0.70f];
    } completion:^(BOOL finished) {}];
    /*NSString *xml = [NSString stringWithContentsOfURL:[NSURL URLWithString:@"http://en.wikipedia.org/w/api.php?action=query&prop=revisions&rvprop=content&format=xml&titles=SurveyMonkey&rvsection=0%22"] encoding:NSUTF8StringEncoding error:nil];
    if ([xml containsString:@"company_name"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"company_name"].location];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
    }else if ([xml containsString:@"name"]){
            xml = [xml substringFromIndex:[xml rangeOfString:@"| name"].location + 2];
            NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
            NSLog(@"%@", name);
    }
    if ([xml containsString:@"company_type"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| company_type"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
        NSLog(@"%@", name);
    }else if ([xml containsString:@"type"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| type"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 4, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 4)];
        NSLog(@"%@", name);
    }
    if ([xml containsString:@"locations"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| locations"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
        NSLog(@"%@", name);
    }
    if ([xml containsString:@"company_slogan"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| company_slogan"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
        NSLog(@"%@", name);
    }else if ([xml containsString:@"slogan"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| slogan"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
        NSLog(@"%@", name);
    }
    if ([xml containsString:@"area_served"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| area_served"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"= "].location + 2, [xml rangeOfString:@"|"].location - [xml rangeOfString:@"= "].location - 2)];
        NSLog(@"%@", name);
    }
    if ([xml containsString:@"homepage"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| homepage"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"URL|"].location + 4, [xml rangeOfString:@"}"].location - [xml rangeOfString:@"URL|"].location - 4)];
        NSLog(@"%@", name);
    }else if ([xml containsString:@"url"]){
        xml = [xml substringFromIndex:[xml rangeOfString:@"| url"].location + 2];
        NSString *name = [xml substringWithRange:NSMakeRange([xml rangeOfString:@"URL|"].location + 4, [xml rangeOfString:@"}"].location - [xml rangeOfString:@"URL|"].location - 4)];
        NSLog(@"%@", name);
    }*/
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
         image = [UIImage imageWithCGImage:image.CGImage scale:image.scale orientation:UIImageOrientationRight];
        [[_weakSelf backgroundImageView] setImage:image];
        cv::Mat originalImage = [UIImage toCVMat:[[_weakSelf backgroundImageView] image]];
        cv::Mat img = originalImage;
        cv::Mat im_gray;
        cv::cvtColor(img, im_gray, CV_RGB2GRAY);
        if (shouldProcess) {
            
            cmt.processFrame(im_gray);
            /*for(int i = 0; i<cmt.trackedKeypoints.size(); i++)
                cv::circle(img, cmt.trackedKeypoints[i].first.pt, 3, cv::Scalar(255,255,255));
            cv::line(img, cmt.topLeft, cmt.topRight, cv::Scalar(255,255,255));
            cv::line(img, cmt.topRight, cmt.bottomRight, cv::Scalar(255,255,255));
            cv::line(img, cmt.bottomRight, cmt.bottomLeft, cv::Scalar(255,255,255));
            cv::line(img, cmt.bottomLeft, cmt.topLeft, cv::Scalar(255,255,255));*/
            if (!(isnan(cmt.bottomLeft.x) || isnan(cmt.topLeft.y))) {
                NSLog(@"PROCESS");
                [trackerView setNeedsDisplay];
                [trackerView setFrame:CGRectMake(245 -(cmt.bottomLeft.y)/3.2, 1.5*(cmt.bottomLeft.x+220), cmt.bottomRight.x - cmt.bottomLeft.x, cmt.topLeft.y - cmt.bottomLeft.y)];
                //[trackerView setFrame:CGRectMake(cmt.bottomLeft.y,cmt.bottomRight.x, cmt.bottomRight.x - cmt.bottomLeft.x, cmt.bottomLeft.y - cmt.topLeft.y)];
                //[trackerView drawRect:CGRectMake(100, 100, 100, 100)];

            }
        }
        image = [UIImage imageWithCGImage:image.CGImage scale:image.scale orientation:UIImageOrientationRight];
        [[_weakSelf backgroundImageView] setImage:image];
        
    });
}

-(IBAction)realTimeTapped:(id)sender{
    sigView.userInteractionEnabled = true;
    [self downloadInfo:nil];
    [UIView animateWithDuration:0.75f animations:^{
        [fromImage setAlpha:0.0f];
        [realTime setAlpha:0.0f];
        [overlay setAlpha:0.0f];
    } completion:^(BOOL finished) {
        blur = false;
        [UIView animateWithDuration:0.75f animations:^{
            [topText setAlpha:1.0f];
            [back setAlpha:1.0f];
        } completion:nil];
    }];
}
-(IBAction)backButtonTapped:(id)sender{
    [sigView erase];
    [trackerView setNeedsDisplay];
    [trackerView removeFromSuperview];
    sigView.userInteractionEnabled = NO;
    [UIView animateWithDuration:0.5f animations:^{
        [topText setAlpha:0.0f];
        [back setAlpha:0.0f];
        [scrollView setAlpha:0.0f];
        if (shouldBlurTransition) {
            [backgroundImageView setAlpha:0.0f];
            shouldBlurTransition = false;
        }
    } completion:^(BOOL finished) {
        [UIView animateWithDuration:0.5f animations:^{
            blur = true;
            [fromImage setAlpha:1.0f];
            [realTime setAlpha:1.0f];
            [overlay setAlpha:1.0f];
            if (!self.videoSource.captureSession.isRunning) {
                [self.videoSource.captureSession startRunning];
            }
        } completion:^(BOOL finished){
            [UIView animateWithDuration:0.25f animations:^{
               [backgroundImageView setAlpha:1.0f];
            }];
        }];
    }];

    
}
-(IBAction)process:(id)sender{
    cv::Mat mat = [backgroundImageView.image toCVMat];
    
}
- (void) imagePickerControllerDidCancel: (UIImagePickerController *) picker {
    
    [self dismissViewControllerAnimated:YES completion:nil];
    [self backButtonTapped:nil];
}

// For responding to the user accepting a newly-captured picture or movie
- (void) imagePickerController: (UIImagePickerController *) picker didFinishPickingMediaWithInfo: (NSDictionary *) info {
    
    [self.backgroundImageView setBounds:[[UIScreen mainScreen] bounds]];
    
    self.backgroundImageView.image = (UIImage *) [info objectForKey:
                                   UIImagePickerControllerOriginalImage];
    self.backgroundImageView.contentMode = UIViewContentModeScaleAspectFit;

    [self dismissViewControllerAnimated:YES completion:nil];
}
-(IBAction)wiggle:(id)sender{
    sigView.userInteractionEnabled = true;
    shouldBlurTransition = true;
    [UIView animateWithDuration:0.75f animations:^{
        [fromImage setAlpha:0.0f];
        [realTime setAlpha:0.0f];
        [overlay setAlpha:0.0f];
    } completion:^(BOOL finished) {
        blur = false;
        [UIView animateWithDuration:0.75f animations:^{
            [topText setAlpha:1.0f];
            [back setAlpha:1.0f];
            [self.videoSource.captureSession stopRunning];
            if([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]){
                
                //we have a camera, therefore we take a picture
                
                UIImagePickerController *cameraUI = [[UIImagePickerController alloc] init];
                cameraUI.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
                
                // Hides the controls for moving & scaling pictures, or for
                // trimming movies. To instead show the controls, use YES.
                cameraUI.allowsEditing = NO;
                
                cameraUI.delegate = self;
                
                [self presentViewController:cameraUI animated:YES completion:nil];
                
                
            } else {
                // alert user that camera is unavailable and to try again if device does actually have a camera
            }

        } completion:nil];
    }];
    
    
    
}
-(void)getGray:(cv::Mat&) input andOut:(cv::Mat&) gray
{
    const int numChannes = input.channels();
    
    if (numChannes == 4)
    {
#if TARGET_IPHONE_SIMULATOR
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
#else
        cv::neon_cvtColorBGRA2GRAY(input, gray);
#endif
        
    }
    else if (numChannes == 3)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else if (numChannes == 1)
    {
        gray = input;
    }
}
/*-(void) trackingInitWithOutput:(cv::Mat&) image // output image
           withInput:(cv::Mat&) image1 // input image
               withArrayOutput:(std::vector<cv::Point2f>&) points1 // output points array
{
    // initialise tracker
    std::cout << "test \n";
    cv::TermCriteria termcrit = (cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,20.0,0.03);

    cv::goodFeaturesToTrack(image1,
                            points1,
                            200,
                            0.01,
                            10,
                            cv::Mat(),
                            3,
                            false,
                            k);
    
    // refine corner locations
    cv::cornerSubPix(image1, points1, cv::Size(10,10), cv::Size(-1,-1), termcrit);
    
    size_t i;
    for( i = 0; i < points1.size(); i++ )
    {
        // draw points
        cv::circle( image, points1[i], 3, cv::Scalar(0,255,0), -1, 8);
    }
}*/

-(BOOL)processFrame:(cv::Mat&) inputFrame andOut:(cv::Mat&) outputFrame{
    // display the frame
    inputFrame.copyTo(outputFrame);
    
    // convert input frame to gray scale
    [self getGray:inputFrame andOut:imageNext];
    
    // prepare the tracking class
    ObjectTrackingClass ot;
    ot.setMaxCorners(200);
    
    // begin tracking object
    if ( trackObject ) {
        ot.track(outputFrame,
                 imagePrev,
                 imageNext,
                 pointsPrev,
                 pointsNext,
                 status,
                 err);
        
        // check if the next points array isn't empty
        if ( pointsNext.empty() )
            trackObject = false;
    }
    
    // store the reference frame as the object to track
    if ( computeObject ) {
        ot.init(outputFrame, imagePrev, pointsNext);
        trackObject = true;
        computeObject = false;
    }
    
    // backup previous frame
    imageNext.copyTo(imagePrev);
    
    // backup points array
    std::swap(pointsNext, pointsPrev);
    
    return true;
}
-(void) setReferenceFrame:(cv::Mat&) reference
{
    [self getGray:reference andOut:imagePrev];
    computeObject = true;
}

// Reset object keypoints and descriptors
-(void) resetReferenceFrame
{
    trackObject = false;
    computeObject = false;
}
-(void)initImageProcessingXMax:(float) xMax XMin:(float) xMin YMax:(float) yMax YMin:(float) yMin{
    [trackerView setNeedsDisplay];
    [trackerView setFrame:CGRectMake(xMin, yMin, xMax - xMin, yMax - yMin)];
    [backgroundImageView addSubview:trackerView];
    [self uploadFile:backgroundImageView sizeOfImage:CGRectMake(xMin, yMin, xMax - xMin, yMax - yMin)];
    [trackerView removeFromSuperview];
    [sigView erase];
    //[self.view addSubview:trackerView];
    /*cv::Point2f initTopLeft(xMin,yMin);
    cv::Point2f initBottomDown(xMax,yMax);
    UIImage *bg = backgroundImageView.image;
    cv::Mat img = [UIImage toCVMat:bg];
    cv::Mat im_gray;
    cv::cvtColor(img, im_gray, CV_RGB2GRAY);
    cmt.initialise(im_gray, initTopLeft, initBottomDown);
    NSLog(@"im_gray: %f %f", [[UIImage fromCVMat:im_gray] size].width,[[UIImage fromCVMat:im_gray] size].height);
    shouldProcess = false;
    [sigView erase];
    NSLog(@"INITIALIZED");*/

}
@end
