//
//  Tracker.m
//  PennApps-2015
//
//  Created by Jordan Brown on 1/17/15.
//  Copyright (c) 2015 woosufjordaline. All rights reserved.
//

#import "Tracker.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
@implementation Tracker

- (void)drawRect:(CGRect)rect {
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetLineWidth(context, 4.0);
    CGContextSetStrokeColorWithColor(context,
                                     [UIColor redColor].CGColor);
    CGRect rectangle = rect;
    CGContextAddRect(context, rectangle);
    CGContextStrokePath(context);
}

@end
