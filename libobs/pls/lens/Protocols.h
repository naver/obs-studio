#ifdef __APPLE__
/*

  Written by Linda Zhong

  The NSXPCConnection API requires that the server object in an XPC communication conform to
  some protocol. However no such restriction applies for the client object, as the client 
  can pass a completion block to the server in the original message. This is a marked improvement 
  over the old Distributed Objects API.

  Methods intended to be called over XPC must return void

  The protocol is is declared in a dedicated file as both the client and daemon need to know about it.

*/

static NSString *kLaunchServiceLabel = @"com.naver.camstudio.daemon";
static NSString *kLaunchServiceVersionTag = @"LaunchDaemonVersion_";
static NSString *kLaunchServiceVersionSeparator = @"_";

static NSString *kHostAppName = @"PRISMLens.app";
static NSString *kHostBundleID = @"com.prismlive.camstudio";
static NSString *kPrismBundleID = @"com.prismlive.prismlivestudio";
static NSString *kVCamExtensionPrefix = @"PRISM Lens ";
static NSInteger kMaxVCamCount = 3;

#define kDaemonNotificationName "com.naver.camstudio.daemon.connectCounterDidChange"
#define kPrismLens1Name "PRISM Lens 1"
#define kPrismLens2Name "PRISM Lens 2"
#define kPrismLens3Name "PRISM Lens 3"


@protocol ExampleDaemonProtocol

- (void)getAllClientIDs:(void(^)(NSArray<NSString *> *clientIDs, NSString *currentVCamClientID))completion;

// MARK: -- vcam use or not event

- (void)startStreamWithClientID:(NSString *)clientID;
- (void)stopStreamWithClientID:(NSString *)clientID;

// MARK: -- vcam user info
/// vcam extension cannot get bundle id from process id. So, have to transfer process id.
- (void)pushVCamSubscribeProcessID:(NSString *)subPID
					   forClientID:(NSString *)clientID
						completion:(void(^)(void))completion;

- (void)popVCamSubIDs:(void(^)(NSArray<NSString *> *subIDs))completion;

@end

#endif
