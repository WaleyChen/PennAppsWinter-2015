//
//  ViewController.h
//  PennApps-2015
//
//  Created by Jordan Brown on 1/16/15.
//  Copyright (c) 2015 woosufjordaline. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
@interface ViewController : UIViewController
{
    NSMutableURLRequest *request;
}
@property (nonatomic, weak) IBOutlet UIImageView * backgroundImageView;

-(IBAction)uploadFile:(id)sender;
-(IBAction)downloadInfo:(id)sender;
-(IBAction)process:(id)sender;
-(void)initImageProcessing;
-(UIImage *)LogoProcessing:(UIImage *)image;
@end

