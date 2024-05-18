#import <UIKit/UIKit.h>

UILabel *log_buffer;

void plat_dbg(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
	va_end(args);

	strcat(buffer, "\n");

	NSString *msg = [NSString stringWithUTF8String: buffer];

	dispatch_async(dispatch_get_main_queue(), ^{
		log_buffer.text = [log_buffer.text stringByAppendingString:msg];
	});
}

void uikit_toast(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	UIAlertView *toast = [[UIAlertView alloc] initWithTitle:nil
		message:[NSString stringWithUTF8String: buffer]
		delegate:nil
		cancelButtonTitle:nil
		otherButtonTitles:nil, nil];
	[toast show];
  
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
	    [toast dismissWithClickedButtonIndex:0 animated:YES];
	});
}


@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(id)options
{
    NSLog(@"%s", __PRETTY_FUNCTION__);
    
    CGRect mainScreenBounds = [[UIScreen mainScreen] bounds];
    self.window = [[UIWindow alloc] initWithFrame:mainScreenBounds];
    
    // Create the view controller and add it to the window.
    UIViewController *viewController = [[UIViewController alloc] init];
    viewController.view.backgroundColor = [UIColor whiteColor];
    viewController.view.frame = mainScreenBounds;
    self.window.rootViewController = viewController;

	UIScrollView *scroll = [[UIScrollView alloc] init];
	scroll.frame = CGRectMake(0, 0, mainScreenBounds.size.width, mainScreenBounds.size.height);
	scroll.contentSize = CGSizeMake(mainScreenBounds.size.width, 10000);
	scroll.backgroundColor = [UIColor blackColor];

    UIButton *btn = [UIButton buttonWithType:UIButtonTypeCustom];
    btn.backgroundColor = [UIColor redColor];
    btn.frame = CGRectMake(50, 50, 200, 50);
    [btn setTitle:@"Click to do stuff" forState:UIControlStateNormal];
    btn.titleLabel.font = [UIFont systemFontOfSize:15];
    btn.translatesAutoresizingMaskIntoConstraints = NO;

	UILabel *tv = [[UILabel alloc] init];
	tv.text = @"";
	//tv.frame = CGRectMake(0, 0, 200, 50);
	tv.backgroundColor = [UIColor grayColor];
	tv.lineBreakMode = NSLineBreakByWordWrapping;
	tv.numberOfLines = 0;
	tv.textColor = [UIColor whiteColor];
	tv.translatesAutoresizingMaskIntoConstraints = NO;
	log_buffer = tv;

	plat_dbg("Fudge for iOS");
	plat_dbg("Doing stuff...");

// 	[scroll addSubview:btn];
 	[scroll addSubview:tv]; 
    [viewController.view addSubview:scroll];
    	[NSLayoutConstraint activateConstraints:@[
			[NSLayoutConstraint constraintWithItem:tv
	            attribute:NSLayoutAttributeTop
	            relatedBy:NSLayoutRelationEqual
	            toItem:tv.superview
	            attribute:NSLayoutAttributeTop
	            multiplier:1.f
	            constant:0.f]
	]];

	
    // Create constraints that center the label in the middle of the superview.
//    btn.translatesAutoresizingMaskIntoConstraints = YES;
//    NSLayoutConstraint *centerX = [NSLayoutConstraint constraintWithItem:scroll
//            attribute:NSLayoutAttributeCenterX
//            relatedBy:NSLayoutRelationEqual
//            toItem:btn.superview
//            attribute:NSLayoutAttributeCenterX
//            multiplier:1.f
//            constant:0.f];
//    NSLayoutConstraint *centerY = [NSLayoutConstraint constraintWithItem:btn
//            attribute:NSLayoutAttributeTop
//            relatedBy:NSLayoutRelationEqual
//            toItem:btn.superview
//            attribute:NSLayoutAttributeTop
//            multiplier:1.f
//            constant:0.f];
//    
//    [NSLayoutConstraint activateConstraints:@[centerX, centerY]];
    
    [self.window makeKeyAndVisible];
    
    return YES;
}

@end

int main(int argc, char *argv[])
{
    NSLog(@"%s", __PRETTY_FUNCTION__);
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
