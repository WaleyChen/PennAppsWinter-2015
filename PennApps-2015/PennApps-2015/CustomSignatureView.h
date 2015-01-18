//
//  CustomSignatureView.h
//  Scoopr
//
//  Created by Anil Mehta on 9/27/13.
//  Copyright (c) 2013 Anil Mehta. All rights reserved.
//

#import <UIKit/UIKit.h>
@class ViewController;
@interface CustomSignatureView : UIView{
    
}
@property (nonatomic, retain) ViewController *vc;
@property float maxX;
@property float minX;
@property float maxY;
@property float minY;
@property CGRect rectangle;
- (void)erase;
@end
