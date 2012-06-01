#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CFString.h>

// note that you should #import <Cocoa/Cocoa.h> BEFORE macutils.h to work with Cocoa correctly
#import "macutils.h"

#include "log.h"

#include <QDebug>

//static NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

WindowRef windowRefFromWidget(const QWidget *w)
{
	return (WindowRef)[nsWindowFromWidget(w) windowRef];
}

NSWindow * nsWindowFromWidget(const QWidget *w)
{
	return [nsViewFromWidget(w) window];
}

NSView * nsViewFromWidget(const QWidget *w)
{
	return (NSView *)w->winId();
}

// warning! NSImage isn't released!
NSImage * nsImageFromQImage(const QImage &image)
{
	if (!image.isNull())
	{
		CGImageRef ref = QPixmap::fromImage(image).toMacCGImageRef();
		NSImage * nsimg = [[NSImage alloc] initWithCGImage: ref size: NSZeroSize];
		CGImageRelease(ref);
		return nsimg;
	}
	else
		return nil;
}

QImage qImageFromNSImage(NSImage *image)
{
	if (image)
	{
		CGImageRef ref = [image CGImageForProposedRect:NULL context:nil hints:nil];
		QImage result = QPixmap::fromMacCGImageRef(ref).toImage();
		return result;
	}
	return QImage();
}

// warning! NSString isn't released!
NSString * nsStringFromQString(const QString &s)
{
	const char * utf8String = s.toUtf8().constData();
	return [[NSString alloc] initWithUTF8String: utf8String];
}

QString qStringFromNSString(NSString *s)
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	QString res = QString::fromUtf8([s UTF8String]);
	[pool release];
	return res;
}

void setWindowShadowEnabled(QWidget *window, bool enabled)
{
	[[nsViewFromWidget(window) window] setHasShadow: (enabled ? YES : NO)];
}

bool isWindowGrowButtonEnabled(const QWidget *window)
{
    if (window)
        return [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] isEnabled] == YES;
    else
        return false;
}

void setWindowGrowButtonEnabled(QWidget *window, bool enabled)
{
    if (window)
        [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] setEnabled: (enabled ? YES : NO)];
}

void hideWindow(void */* (NSWindow*) */ window)
{
	NSWindow *nsWindow = (NSWindow*)window;
	[nsWindow orderOut:nil];
}

void setWindowFullScreenEnabled(QWidget *window, bool enabled)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	NSWindowCollectionBehavior b = [wnd collectionBehavior];
	if (enabled)
		[wnd setCollectionBehavior: b | NSWindowCollectionBehaviorFullScreenPrimary];
	else if (isWindowFullScreenEnabled(window))
		[wnd setCollectionBehavior: b ^ NSWindowCollectionBehaviorFullScreenPrimary];
}

bool isWindowFullScreenEnabled(const QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary;
}

void setWindowFullScreen(QWidget *window, bool enabled)
{
	if (isWindowFullScreenEnabled(window))
	{
		bool isFullScreen = isWindowFullScreen(window);
		if ((enabled && !isFullScreen) || (!enabled && isFullScreen))
		{
			if (enabled)
			{
				// removing Qt's ontop flag
				Qt::WindowFlags flags = window->windowFlags();
				if (flags & Qt::WindowStaysOnTopHint)
				{
					LogWarning(QString("[setWindowFullScreen]: Widget %1 has Qt ontop flag! It will be removed and window will be redisplayed.").arg(window->windowTitle()));
					window->setWindowFlags(flags ^ Qt::WindowStaysOnTopHint);
					window->show();
					setWindowFullScreenEnabled(window, true);
				}
			}

			NSWindow *wnd = nsWindowFromWidget(window->window());
			[wnd toggleFullScreen:nil];
		}
	}
}

bool isWindowFullScreen(const QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd styleMask] & NSFullScreenWindowMask;
}

void setWindowOntop(QWidget *window, bool enabled)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	[wnd setLevel:(enabled ? NSModalPanelWindowLevel : NSNormalWindowLevel)];
}

bool isWindowOntop(const QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd level] == NSModalPanelWindowLevel;
}

void setAppFullScreenEnabled(bool enabled)
{
	NSApplication *app = [NSApplication sharedApplication];
	if (enabled)
		[app setPresentationOptions: [app presentationOptions] | NSApplicationPresentationFullScreen];
	else if (isAppFullScreenEnabled())
		[app setPresentationOptions: [app presentationOptions] ^ NSApplicationPresentationFullScreen];
}

bool isAppFullScreenEnabled()
{
	return [[NSApplication sharedApplication] presentationOptions] & NSApplicationPresentationFullScreen;
}

QString convertFromMacCyrillic(const char *str)
{
	NSString *srcString = [[NSString alloc] initWithCString:str encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingMacCyrillic)];
	QString resString = qStringFromNSString(srcString);
	[srcString release];
	return resString;
}
