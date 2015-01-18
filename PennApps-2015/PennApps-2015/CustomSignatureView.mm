//
//  CustomSignatureView.m
//  Scoopr
//
//  Created by Anil Mehta on 9/27/13.
//  Copyright (c) 2013 Anil Mehta. All rights reserved.
//

#import "CustomSignatureView.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#import "ArbitraryTracking.h"
#import "ViewController.h"
static CGPoint midpoint(CGPoint p0, CGPoint p1) {
    return (CGPoint) {
        static_cast<CGFloat>((p0.x + p1.x) / 2.0),
        static_cast<CGFloat>((p0.y + p1.y) / 2.0)
    };
}

@interface CustomSignatureView () {
    UIBezierPath *path;
    CGPoint previousPoint;
    
    CGRect   rectImage;
}
@end

@implementation CustomSignatureView
@synthesize vc;
@synthesize rectangle;
- (void)commonInit {
    path = [UIBezierPath bezierPath];
    self.maxX = -1;
    self.maxY = -1;
    self.minX = -1;
    self.minY = -1;
   
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder]) [self commonInit];
    return self;
}
- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) [self commonInit];
    return self;
}

- (void)erase {
    path = [UIBezierPath bezierPath];
    self.maxX = -1;
    self.maxY = -1;
    self.minX = -1;
    self.minY = -1;
    [self setNeedsDisplay];
}
-(void)getBoxedImage{
    path = [UIBezierPath bezierPath];
    [self setNeedsDisplay];
    
}
- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    [self erase];
    UITouch *touch = [touches anyObject];
    CGPoint currentPoint = [touch locationInView:self];
    [path moveToPoint:currentPoint];
    previousPoint = currentPoint;
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    CGPoint currentPoint = [touch locationInView:self];
    if (self.minX == -1 || currentPoint.x < self.minX) {
        self.minX = currentPoint.x;
    }
    if (self.minY == -1 || currentPoint.y < self.minY) {
        self.minY = currentPoint.y;
    }
    if (self.maxX == -1 || currentPoint.x > self.maxX) {
        self.maxX = currentPoint.x;
    }
    if (self.maxY == -1 || currentPoint.y > self.maxY) {
        self.maxY = currentPoint.y;
    }
    if (sqrt(pow(currentPoint.x - previousPoint.x, 2)+pow(currentPoint.y - previousPoint.y, 2)) >= 15.0) {
        CGPoint midPoint = midpoint(previousPoint, currentPoint);
        
        [path addLineToPoint:midPoint];
        
        previousPoint = currentPoint;
        
        [self setNeedsDisplay];
    }
   
    
    /*UIGraphicsBeginImageContext(rectImage.size);
    
    // Put everything in the current view into the screenshot
    [[self layer] renderInContext:UIGraphicsGetCurrentContext()];
    // Save the current image context info into a UIImage
    UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    [AppDelegate sharedInstance].img_screenshot = newImage;*/
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    [self getBoxedImage];
    [vc initImageProcessingXMax:self.maxX XMin:self.minX YMax:self.maxY YMin:self.minY];
    
/*    // Create the screenshot
//    UIGraphicsBeginImageContext(self.bounds.size);

    UIGraphicsBeginImageContext(rectImage.size);
    
    // Put everything in the current view into the screenshot
    [[self layer] renderInContext:UIGraphicsGetCurrentContext()];
    // Save the current image context info into a UIImage
    UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    [AppDelegate sharedInstance].img_screenshot = newImage;
*/
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
   
}
- (void)drawRect:(CGRect)rect
{
    //[[UIColor colorWithRed:99/255.0 green:143/255.0 blue:196/255.0 alpha:1.0] setStroke];
    rectImage = rect;
  /*  [[UIColor redColor] setStroke];
    [path stroke];
    [path  setLineWidth:1.0];
    */
    [[UIColor colorWithRed:0x2D/255.0 green:0x49/255.0 blue:0x3E/255.0 alpha:1.0] setStroke];
    [path stroke];
    //[path  setLineWidth:5.0];
    [path  setLineWidth:5.0];
    //Get the CGContext from this view
    rectangle = CGRectMake(self.minX, self.minY,(self.maxX - self.minX),(self.maxY - self.minY));
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 0.0);   //this is the transparent color
    CGContextSetRGBStrokeColor(context, 1.0, 0.0, 0.0, 0.5);
    CGContextFillRect(context, rectangle);
    CGContextStrokeRect(context, rectangle);
}
-(void)makeRectangleWithX:(float)x andY:(float)y andWidth:(float)width andHeight:(float)height{
    [self setNeedsDisplay];
    
}

@end
