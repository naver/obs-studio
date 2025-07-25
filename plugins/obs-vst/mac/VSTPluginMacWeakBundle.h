//
//  VSTPluginMacWeakBundle.h
//  CoreMLSample
//
//  Created by andy on 2025-03-31.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VSTPluginMacWeakBundle : NSObject

- (id)initWithCFBundle:(CFBundleRef)bundleRef;

- (bool)bundleIsValid;

@end

NS_ASSUME_NONNULL_END
