//
//  VSTPluginMacWeakBundle.m
//  CoreMLSample
//
//  Created by andy on 2025-03-31.
//

#import "VSTPluginMacWeakBundle.h"
#include "util/base.h"

@interface VSTPluginMacWeakBundle ()
@property (nonatomic, weak) NSBundle *weakBundle;
@end

@implementation VSTPluginMacWeakBundle

- (id)initWithCFBundle:(CFBundleRef)bundleRef
{
    self = [super init];
    if (self) {
        _weakBundle = (__bridge NSBundle *) bundleRef;
    }
    NSString *pointer = [NSString
        stringWithFormat:@"Mac VSTPLguin Proces: VSTPluginMacWeakBundle initWithCFBundle self is %@, bundleRef is %@",
                         self, bundleRef];
    blog(LOG_INFO, "%s", [pointer UTF8String]);
    return self;
}

- (bool)bundleIsValid
{
    if (_weakBundle) {
        return true;
    }
    return false;
}

- (void)dealloc
{
    NSString *pointer = [NSString
        stringWithFormat:@"Mac VSTPLguin Proces: VSTPluginMacWeakBundle dealloc,  self is %@, bundleRef is %@", self,
                         _weakBundle];
    blog(LOG_INFO, "%s", [pointer UTF8String]);
}

@end
