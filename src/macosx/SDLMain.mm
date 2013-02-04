/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

*/
#import <AppKit/AppKit.h>
#import <SDL/SDL.h>
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#include "aa_config.h"
#include "tError.h"
#include "tConfiguration.h"
#include "uMenu.h"
// #ifdef MACOSX_XCODE
// #include "AAURLHandler.h"
// #include "AAGrowlPlugin.h"
// #endif

/* Use this flag to determine whether we use SDLMain.nib or not */
#define		SDL_USE_NIB_FILE	1

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;

#if SDL_USE_NIB_FILE
/* A helper category for NSString */
@interface NSString (ReplaceSubString)
- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString;
@end
#else
/* An internal Apple class used to setup Apple menus */
@interface NSAppleMenuController:NSObject {}
- (void)controlMenu:(NSMenu *)aMenu;
@end
#endif

SDL_Event event;
@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */

- (void)terminate:(id)sender
{
    st_SaveConfig();
    uMenu::quickexit=uMenu::QuickExit_Total;  
}

- (void)sendEvent:(NSEvent *)event
{
    NSEventType eventType = [event type];
    
    if (eventType == NSKeyDown || eventType == NSKeyUp)
    {
        // Let Mac OS X handle Hide, Minimize, etc
        if ([event modifierFlags] & NSCommandKeyMask)
            [super sendEvent:event];
    }
    else
    {
        [super sendEvent:event];
    }
}

@end

//! Change to the resource folder of the application bundle
void MacOSX_SetCWD(char **argv) {
    char buffer[300];
    int count = 2, i;
    strcpy(buffer, argv[0]);
    
    if ( !strstr(buffer, ".app") )
    {
        //it's not a .app bundle
        return;
    }

    for ( i = strlen(buffer); i > 0 && count > 0; i-- )
    {
        if (buffer[i] == '/')
            count--;
    }
    
    if ( i == 0 )
        return;
    
    i+=2;
    buffer[i] = 0;
    strcat( buffer, "Resources");
    chdir( buffer );
}

/* The main class of the application, the application's delegate */
@implementation SDLMain

- (void)quit:(id)sender
{
    st_SaveConfig();
    uMenu::quickexit=uMenu::QuickExit_Total;    
}

/* Fix menu to contain the real app name instead of "SDL App" */
- (void)fixMenu:(NSMenu *)aMenu withAppName:(NSString *)appName
{
    NSRange aRange;
    NSEnumerator *enumerator;
    NSMenuItem *menuItem;

    aRange = [[aMenu title] rangeOfString:@"SDL App"];
    if (aRange.length != 0)
        [aMenu setTitle: [[aMenu title] stringByReplacingRange:aRange with:appName]];

    enumerator = [[aMenu itemArray] objectEnumerator];
    while ((menuItem = [enumerator nextObject]))
    {
        aRange = [[menuItem title] rangeOfString:@"SDL App"];
        if (aRange.length != 0)
            [menuItem setTitle: [[menuItem title] stringByReplacingRange:aRange with:appName]];
        if ([menuItem hasSubmenu])
            [self fixMenu:[menuItem submenu] withAppName:appName];
    }
    [aMenu sizeToFit];
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    int status;

#if SDL_USE_NIB_FILE
    /* Set the main menu to contain the real app name instead of "SDL App" */
    [self fixMenu:[NSApp mainMenu] withAppName:[[NSProcessInfo processInfo] processName]];
#endif

    // Forward events to NSApp
    // https://bugs.launchpad.net/armagetronad/+bug/510174
    // setenv("SDL_ENABLEAPPEVENTS", "1", 1);

    /* Hand off to main application code */
    status = SDL_main(gArgc, gArgv);
    
    /* We're done, thank you for playing */
    exit(status);
}
@end


@implementation NSString (ReplaceSubString)

- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
    unsigned int bufferSize;
    unsigned int selfLen = [self length];
    unsigned int aStringLen = [aString length];
    unichar *buffer;
    NSRange localRange;
    NSString *result;

    bufferSize = selfLen + aStringLen - aRange.length;
    buffer = (unichar *) NSAllocateMemoryPages(bufferSize*sizeof(unichar));
    
    /* Get first part into buffer */
    localRange.location = 0;
    localRange.length = aRange.location;
    [self getCharacters:buffer range:localRange];
    
    /* Get middle part into buffer */
    localRange.location = 0;
    localRange.length = aStringLen;
    [aString getCharacters:(buffer+aRange.location) range:localRange];
     
    /* Get last part into buffer */
    localRange.location = aRange.location + aRange.length;
    localRange.length = selfLen - localRange.location;
    [self getCharacters:(buffer+aRange.location+aStringLen) range:localRange];
    
    /* Build output string */
    result = [NSString stringWithCharacters:buffer length:bufferSize];
    NSDeallocateMemoryPages(buffer, bufferSize);
    
    return result;
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
// #ifdef MACOSX_XCODE
//     AAGrowlPlugin *growl = [[AAGrowlPlugin alloc] init];
//     [growl startGrowling];
//     SetupAAURLHandler();
// #endif

    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 )
    {
        gArgc = 1;
        gFinderLaunch = YES;
    }
    else
    {
        gArgc = argc;
        gFinderLaunch = NO;
    }

    gArgv = (char**)malloc( sizeof(*gArgv) * (gArgc+1) );
    tASSERT (gArgv != NULL);
    int i = 0;
    for (i; i < gArgc; i++)
    {
        gArgv[i] = (char *) argv[i];
    }

    gArgv[i] = NULL;
	MacOSX_SetCWD(gArgv);
    
#if SDL_USE_NIB_FILE
    // [SDLApplication poseAsClass:[NSApplication class]];
    NSApplicationMain (argc, argv);
#else
    CustomApplicationMain (argc, argv);
#endif
// #ifdef MACOSX_XCODE
//     CleanupAAURLHandler();
//     [growl release];
// #endif
    [pool release];
    return 0;
}
