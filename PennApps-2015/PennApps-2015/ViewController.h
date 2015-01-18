//
//  ViewController.h
//  PennApps-2015
//
//  Created by Jordan Brown on 1/16/15.
//  Copyright (c) 2015 woosufjordaline. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "CustomSignatureView.h"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#import <opencv2/opencv.hpp>
#import "ArbitraryTracking.h"
#import "Tracker.h"
@interface ViewController : UIViewController <UINavigationControllerDelegate,UIImagePickerControllerDelegate>
{
    NSMutableURLRequest *request;
    BOOL blur;
    UIBezierPath *path;
    CGPoint previousPoint; 
    CGRect   rectImage;
    CustomSignatureView *sigView;
    Tracker *trackerView;
    std::vector<cv::Point2f> pointsPrev, pointsNext;
    cv::Mat imageNext, imagePrev;
    std::vector<uchar> status;
    std::vector<float> err;
    std::string m_algorithmName;
    BOOL trackObject;
    BOOL computeObject;
    BOOL shouldBlurTransition;
    BOOL shouldProcess;
    CGRect myRect;
    
}
@property (nonatomic, weak) IBOutlet UIImageView * backgroundImageView;
@property (nonatomic, weak) IBOutlet UIButton *realTime;
@property (nonatomic, weak) IBOutlet UIButton *fromImage;
@property (nonatomic, weak) IBOutlet UIImageView *overlay;
@property (nonatomic, weak) IBOutlet UIImageView *topText;
@property (nonatomic, weak) IBOutlet UIButton *back;
@property ArbitraryTracking cmt;
-(void)uploadFile:(UIImageView *)image;
-(IBAction)downloadInfo:(id)sender;
-(IBAction)process:(id)sender;
-(void)initImageProcessingXMax:(float) xMax XMin:(float) xMin YMax:(float) yMax YMin:(float) yMin;
-(UIImage *)LogoProcessing:(UIImageView *)image;
-(IBAction)realTimeTapped:(id)sender;
-(IBAction)backButtonTapped:(id)sender;
-(IBAction)wiggle:(id)sender;
-(BOOL)processFrame:(cv::Mat&) inputFrame andOut:(cv::Mat&) outputFrame;
-(cv::Mat)generateReferenceFrame;
@end

