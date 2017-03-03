//
//  XMCustomView.h
//  XMBindingLibrarySample
//
//  Created by Anuj Bhatia on 1/18/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

#import "XMCustomViewDelegate.h"

@interface XMCustomView : UIView
{
    
}

@property (nonatomic, strong) NSString* name;
@property (nonatomic, assign) id <XMCustomViewDelegate> delegate;


-(void) customizeViewWithText:(NSString *)message;



@end
