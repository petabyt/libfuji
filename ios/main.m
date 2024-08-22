#import <UIKit/UIKit.h>
#import <ifaddrs.h>
#import <net/if.h>
#import <netdb.h>
#include <arpa/inet.h>

#include <app.h>
#include <camlib.h>
#include <fujiptp.h>
#include <fuji.h>

UILabel *log_buffer;

void ui_send_text(char *key, char *fmt, ...) {
	
}

void app_print(char *fmt, ...) {
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

void plat_dbg(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
	va_end(args);

	app_print(buffer);
}

void tester_fail(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
	va_end(args);

	app_print(buffer);
}

void tester_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
	va_end(args);

	app_print(buffer);
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

int app_bind_socket_wifi(int sockfd) {
	return 0;
}

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
	if (sa == NULL) {
		strcpy(s, "NULL");
		return s;
	}
	switch(sa->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
		s, maxlen);
	break;
	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
		break;
	default:
		strncpy(s, "Unknown AF", maxlen);
		return NULL;
	}

	return s;
}

int test_net() {
	struct ifaddrs *list;
	getifaddrs(&list);
	while (list->ifa_next != NULL) {
		char buffer[128];
		get_ip_str(list->ifa_dstaddr, buffer, sizeof(buffer));
		plat_dbg("%s %s", list->ifa_name, buffer);
		list = list->ifa_next;
	}

	return 0;
}

int test_ptp() {
	struct PtpRuntime *r = ptp_get();
	ptp_init(r);
	r->io_kill_switch = 0;
	r->connection_type = PTP_IP_USB;

	const char *ip = "192.168.1.39";

	int rc = ptpip_connect(r, ip, FUJI_CMD_IP_PORT);
	if (rc) {
		plat_dbg("Failed to connect");
	} else {
		plat_dbg("Connected");
	}

	rc = fuji_test_suite(r, ip);
	plat_dbg("Return code: %d", rc);

	return 0;
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
	//btn.frame = CGRectMake(50, 50, 200, 50);
	btn.translatesAutoresizingMaskIntoConstraints = NO;
	[btn setTitle:@"Click to do stuff" forState:UIControlStateNormal];
	btn.titleLabel.font = [UIFont systemFontOfSize:15];

	UILabel *tv = [[UILabel alloc] init];
	tv.text = @"";
	//tv.frame = CGRectMake(0, 0, 200, 50);
	tv.backgroundColor = [UIColor grayColor];
	tv.lineBreakMode = NSLineBreakByWordWrapping;
	tv.numberOfLines = 0;
	tv.textColor = [UIColor whiteColor];
	tv.translatesAutoresizingMaskIntoConstraints = NO;
	log_buffer = tv;

	plat_dbg("iOS Fudge");

 	[scroll addSubview:btn];
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

	[NSLayoutConstraint activateConstraints:@[
		[NSLayoutConstraint constraintWithItem:btn
			attribute:NSLayoutAttributeBottom
			relatedBy:NSLayoutRelationLessThanOrEqual
			toItem:btn.superview
			attribute:NSLayoutAttributeBottom
			multiplier:1.f
			constant:0.f]
	]];
	
// Create constraints that center the label in the middle of the superview.
//btn.translatesAutoresizingMaskIntoConstraints = YES;
//NSLayoutConstraint *centerX = [NSLayoutConstraint constraintWithItem:scroll
//attribute:NSLayoutAttributeCenterX
//relatedBy:NSLayoutRelationEqual
//toItem:btn.superview
//attribute:NSLayoutAttributeCenterX
//multiplier:1.f
//constant:0.f];
//NSLayoutConstraint *centerY = [NSLayoutConstraint constraintWithItem:btn
//attribute:NSLayoutAttributeTop
//relatedBy:NSLayoutRelationEqual
//toItem:btn.superview
//attribute:NSLayoutAttributeTop
//multiplier:1.f
//constant:0.f];
//
//[NSLayoutConstraint activateConstraints:@[centerX, centerY]];

	[self.window makeKeyAndVisible];

	test_ptp();
//test_net();

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
