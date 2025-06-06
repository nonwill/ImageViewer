/*
   Copyright (C) 2017-2025 Peter S. Zhigalov <peter.zhigalov@gmail.com>

   This file is part of the `ImageViewer' program.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MacToolBar.h"

#include "Workarounds/BeginExcludeOpenTransport.h"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include "Workarounds/EndExcludeOpenTransport.h"

#include <QApplication>
#include <QWidget>

#include "Utils/InfoUtils.h"
#include "Utils/LocalizationManager.h"
#include "Utils/Logging.h"
#include "Utils/ObjectiveCUtils.h"
#include "Utils/ThemeUtils.h"
#include "Utils/ThemeUtils_mac.h"
#include "Utils/WindowUtils.h"

#include "Utils/MacNSInteger.h"

#if defined (QT_MAC_USE_CARBON) || ((QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && !defined (QT_MAC_USE_COCOA))
#error "MacToolBar is designed for Cocoa Qt version and incompatible with Carbon Qt version"
#endif

// ====================================================================================================

namespace {

const NSInteger BUTTON_HEIGHT = 32;
const NSInteger ALONE_BUTTON_WIDTH = 40;
const NSInteger GROUPED_BUTTON_WIDTH = 32; // InfoUtils::MacVersionGreatOrEqual(10, 13) ? 36 : 32;
const NSInteger SEGMENTED_OFFSET = 8; // InfoUtils::MacVersionGreatOrEqual(10, 13) ? 0 : 8;
const QColor BUTTON_BASE_COLOR = InfoUtils::MacVersionGreatOrEqual(10, 10) ? QColor(qRgb(85, 85, 85)) : QColor(Qt::black);
const QColor BUTTON_ALTERNATE_COLOR = Qt::white;

} // namespace

// ====================================================================================================

@interface CheckableNSButton : NSButton
{
    @private
    BOOL m_isChecked;
    BOOL m_isCheckable;
}

- (id)initWithFrame:(NSRect)frameRect;

- (BOOL)isCheckable;
- (void)setCheckable:(BOOL)isCheckable;

- (BOOL)isChecked;
- (void)setChecked:(BOOL)isChecked;

@end


@implementation CheckableNSButton

- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    m_isChecked = NO;
    m_isCheckable = NO;
    return self;
}

- (BOOL)isCheckable
{
    return m_isCheckable;
}

- (void)setCheckable:(BOOL)isCheckable
{
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_14_AND_LATER)
    NSButtonType newType = (isCheckable ? NSButtonTypePushOnPushOff : NSButtonTypeMomentaryLight);
#else
    NSButtonType newType = (isCheckable ? NSPushOnPushOffButton : NSMomentaryLightButton);
#endif
    [self setButtonType:newType];
    m_isCheckable = isCheckable;
}

- (BOOL)isChecked
{
    return m_isChecked;
}

- (void)setChecked:(BOOL)isChecked
{
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_14_AND_LATER)
    NSControlStateValue newState = isChecked ? NSControlStateValueOn : NSControlStateValueOff;
#else
    NSCellStateValue newState = isChecked ? NSOnState : NSOffState;
#endif
    [self setState:newState];
    if((![self isChecked] && isChecked) || ([self isChecked] && !isChecked))
    {
        NSImage *tempImage = [self image];
        [self setImage:[self alternateImage]];
        [self setAlternateImage:tempImage];
    }
    m_isChecked = isChecked;
}

@end

// ====================================================================================================

@interface CustomNSToolbar : NSToolbar
{
    @private
    BOOL m_manualVisibleState;
    QObject *m_emitterObject;
}

- (id)initWithIdentifier:(NSString *)identifier
        andEmitterObject:(QObject *)emitterObject;

- (void)setVisible:(BOOL)isVisible;

- (void)setManualVisible:(BOOL)isVisible;

@end


@implementation CustomNSToolbar

- (id)initWithIdentifier:(NSString *)identifier
        andEmitterObject:(QObject *)emitterObject
{
    self = [super initWithIdentifier:identifier];
    m_manualVisibleState = [super isVisible];
    m_emitterObject = emitterObject;
    return self;
}

- (void)setVisible:(BOOL)isVisible
{
    if((!m_manualVisibleState && !isVisible) || (m_manualVisibleState && isVisible))
        return [super setVisible:isVisible];
    QMetaObject::invokeMethod(m_emitterObject, "showToolBarRequested");
}

- (void)setManualVisible:(BOOL)isVisible
{
    m_manualVisibleState = isVisible;
    [super setVisible:isVisible];
}

@end

// ====================================================================================================

namespace {

struct SimpleToolBarItem
{
    NSToolbarItem *item;

    SimpleToolBarItem()
        : item(nil)
    {}

    ~SimpleToolBarItem()
    {
        AUTORELEASE_POOL;
        if(item)
            [item release];
    }

    NSString *identifier() const
    {
        if(!item)
            return nil;
        return [item itemIdentifier];
    }

    bool actionSenderMatch(id sender) const
    {
        if(item && item == sender)
            return true;
        if(sender && item && [item menuFormRepresentation] == sender)
            return true;
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
        if(sender && item && [sender conformsToProtocol:@protocol(NSUserInterfaceItemIdentification)] && [sender respondsToSelector:@selector(identifier)] && [[item itemIdentifier] isEqualToString:[sender identifier]])
            return true;
#endif
        return false;
    }

    void setLabel(const QString &label)
    {
        if(!item)
            return;
        AUTORELEASE_POOL;
        [item setLabel:ObjCUtils::QStringToNSString(label)];
    }

    void setToolTip(const QString &toolTip)
    {
        if(!item)
            return;
        AUTORELEASE_POOL;
        [item setToolTip:ObjCUtils::QStringToNSString(toolTip)];
    }

    void setPaletteLabel(const QString &paletteLabel)
    {
        if(!item)
            return;
        AUTORELEASE_POOL;
        [item setPaletteLabel:ObjCUtils::QStringToNSString(paletteLabel)];
    }

    void setMenuTitle(const QString &menuTitle)
    {
        if(!item)
            return;
        AUTORELEASE_POOL;
        [[item menuFormRepresentation] setTitle:ObjCUtils::QStringToNSString(menuTitle)];
    }
};

struct SegmentedToolBarItem : SimpleToolBarItem
{
    NSSegmentedControl *segmentedControl;

    SegmentedToolBarItem()
        : segmentedControl(nil)
    {}

    ~SegmentedToolBarItem()
    {
        AUTORELEASE_POOL;
        if(segmentedControl)
            [segmentedControl release];
    }

    void setLabel(const QString &label)
    {
        SimpleToolBarItem::setLabel(label);
        SimpleToolBarItem::setMenuTitle(label);
    }
};

struct GroupedToolBarItem : SimpleToolBarItem
{
    NSToolbarItemGroup *group;
    NSSegmentedControl *segmentedControl;

    GroupedToolBarItem()
        : group(nil)
        , segmentedControl(nil)
    {}

    void setEnabled(bool isEnabled)
    {
        if(!item || !segmentedControl || !group)
            return;
        AUTORELEASE_POOL;
        BOOL value = isEnabled ? YES : NO;
        [item setEnabled:value];
        [[item menuFormRepresentation] setEnabled:value];
        BOOL groupEnabled = isEnabled;
        if(!isEnabled)
        {
            for(NSToolbarItem *subitem in [group subitems])
            {
                if([subitem isEnabled])
                {
                    groupEnabled = YES;
                    break;
                }
            }
        }
        [group setEnabled:groupEnabled];
        [[group menuFormRepresentation] setEnabled:groupEnabled];
        NSInteger segmentNum = segmentNumber();
        if(segmentNum < 0)
            return;
        [segmentedControl setEnabled:value forSegment:segmentNum];
    }

    bool actionSenderMatch(id sender) const
    {
        if(SimpleToolBarItem::actionSenderMatch(sender))
            return true;
        if(!sender || sender != segmentedControl)
            return false;
        NSInteger segmentNum = segmentNumber();
        if(segmentNum < 0 || [segmentedControl selectedSegment] != segmentNum)
            return false;
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER)
        if([segmentedControl respondsToSelector:@selector(trackingMode)])
            if([segmentedControl trackingMode] == NSSegmentSwitchTrackingMomentary)
                return true;
#endif
        [segmentedControl setSelected:NO forSegment:segmentNum];
        return true;
    }

    void setToolTip(const QString &toolTip)
    {
        SimpleToolBarItem::setToolTip(toolTip);
        if(!item || !segmentedControl)
            return;
        AUTORELEASE_POOL;
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_13_AND_LATER)
        if(@available(macOS 10.13, *))
        {
            NSInteger segmentNum = segmentNumber();
            if(segmentNum < 0)
                return;
            [segmentedControl setToolTip:ObjCUtils::QStringToNSString(toolTip) forSegment:segmentNum];
        }
#endif
    }

    NSInteger segmentNumber() const
    {
        if(!item || !group)
            return -1;
        AUTORELEASE_POOL;
        NSArray *groupItems = [group subitems];
        if(![groupItems containsObject:item])
            return -1;
        return static_cast<NSInteger>([[group subitems] indexOfObject:item]);
    }
};

struct ButtonedToolBarItem : SimpleToolBarItem
{
    CheckableNSButton *button;

    ButtonedToolBarItem()
        : button(nil)
    {}

    ~ButtonedToolBarItem()
    {
        AUTORELEASE_POOL;
        if(button)
            [button release];
    }

    void setEnabled(bool isEnabled)
    {
        if(!item || !button)
            return;
        AUTORELEASE_POOL;
        BOOL value = isEnabled ? YES : NO;
        [button setEnabled:value];
        [item setEnabled:value];
        [[item menuFormRepresentation] setEnabled:value];
    }

    void setChecked(bool isChecked)
    {
        if(!item || !button)
            return;
        AUTORELEASE_POOL;
        BOOL value = isChecked ? YES : NO;
        [button setChecked:value];
    }

    bool actionSenderMatch(id sender) const
    {
        if(SimpleToolBarItem::actionSenderMatch(sender))
            return true;
        if(button && button == sender)
            return true;
        if(sender && item && [item menuFormRepresentation] == sender)
            return true;
        return false;
    }

    void setLabel(const QString &label)
    {
        SimpleToolBarItem::setLabel(label);
        SimpleToolBarItem::setMenuTitle(label);
    }

    void setToolTip(const QString &toolTip)
    {
        if(!button)
            return;
        AUTORELEASE_POOL;
        [button setToolTip:ObjCUtils::QStringToNSString(toolTip)];
    }
};

} // namespace

// ====================================================================================================

namespace {
struct ToolBarData;
} // namespace

@interface MacToolbarDelegate : NSObject
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER) && defined (MAC_OS_X_VERSION_MAX_ALLOWED) && (MAC_OS_X_VERSION_MAX_ALLOWED >= 1060)
<NSToolbarDelegate>
#endif
{
    ToolBarData *m_toolBarData;
    QObject *m_emitterObject;
}

- (id)initWithToolBarData:(ToolBarData *)toolBarData
         andEmitterObject:(QObject*)emitterObject;

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
     itemForItemIdentifier:(NSString *)itemIdentifier
 willBeInsertedIntoToolbar:(BOOL)flag;

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar;

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar;

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar;

- (BOOL)validateMenuItem:(NSMenuItem *)item;

- (BOOL)validateToolbarItem:(NSToolbarItem *)item;

- (IBAction)itemClicked:(id)sender;

- (void)makeSegmentedPairItem:(SegmentedToolBarItem &)groupItem
          withGroupIdentifier:(NSString *)groupIdentifier
       withVisibilityPriority:(NSInteger)visibilityPriority
                withFirstItem:(GroupedToolBarItem &)firstItem
          withFirstIdentifier:(NSString *)firstIdentifier
               withFirstImage:(NSImage *)firstImage
               withSecondItem:(GroupedToolBarItem &)secondItem
         withSecondIdentifier:(NSString *)secondIdentifier
              withSecondImage:(NSImage *)secondImage;

- (void)makeButtonedItem:(ButtonedToolBarItem &)buttonedItem
          withIdentifier:(NSString *)identifier
  withVisibilityPriority:(NSInteger)visibilityPriority
               withImage:(NSImage *)image
      withAlternateImage:(NSImage *)alternateImage;

@end

// ====================================================================================================

namespace {

struct ToolBarData
{
    SimpleToolBarItem space;
    SimpleToolBarItem flexibleSpace;

    GroupedToolBarItem navigatePrevious;
    GroupedToolBarItem navigateNext;
    SegmentedToolBarItem navigateGroup;

    ButtonedToolBarItem startSlideShow;

    GroupedToolBarItem zoomOut;
    GroupedToolBarItem zoomIn;
    SegmentedToolBarItem zoomGroup;

    ButtonedToolBarItem zoomFitToWindow;
    ButtonedToolBarItem zoomOriginalSize;
    ButtonedToolBarItem zoomFullScreen;

    GroupedToolBarItem rotateCounterclockwise;
    GroupedToolBarItem rotateClockwise;
    SegmentedToolBarItem rotateGroup;

    GroupedToolBarItem flipHorizontal;
    GroupedToolBarItem flipVertical;
    SegmentedToolBarItem flipGroup;

    ButtonedToolBarItem openFile;
    ButtonedToolBarItem saveFileAs;
    ButtonedToolBarItem deleteFile;
    ButtonedToolBarItem print;
    ButtonedToolBarItem preferences;
    ButtonedToolBarItem exit;
};

} // namespace

// ====================================================================================================

namespace {

NSImage *NSImageForIconType(ThemeUtils::IconTypes iconType, bool darkBackground = false)
{
    if(NSImage *systemImage = ThemeUtils::GetMacSystemImage(iconType))
        return systemImage;

    const QIcon themeIcon = ThemeUtils::GetIcon(iconType);
    const QSize iconSize(16, 16);
    const QPixmap iconPixmap = themeIcon.pixmap(iconSize);
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER)
    NSImage *image = ObjCUtils::QPixmapToNSImage(iconPixmap, iconSize);
    if([image respondsToSelector:@selector(setTemplate:)])
    {
        [image setTemplate:YES];
        return image;
    }
#endif
    QImage iconImage(iconPixmap.size(), QImage::Format_ARGB32_Premultiplied);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    iconImage.fill(darkBackground ? BUTTON_ALTERNATE_COLOR : BUTTON_BASE_COLOR);
#else
    iconImage.fill((darkBackground ? BUTTON_ALTERNATE_COLOR : BUTTON_BASE_COLOR).rgb());
#endif
    iconImage.setAlphaChannel(iconPixmap.toImage()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
                              .convertToFormat(QImage::Format_Alpha8));
#else
                              .alphaChannel());
#endif
    return ObjCUtils::QImageToNSImage(iconImage, iconSize);
}

} // namespace

// ====================================================================================================

@implementation MacToolbarDelegate

- (id)initWithToolBarData:(ToolBarData *)toolBarData
         andEmitterObject:(QObject*)emitterObject
{
    self = [super init];
    m_toolBarData = toolBarData;
    m_emitterObject = emitterObject;

    m_toolBarData->space.item = [[NSToolbarItem alloc] initWithItemIdentifier:NSToolbarSpaceItemIdentifier];
    m_toolBarData->flexibleSpace.item = [[NSToolbarItem alloc] initWithItemIdentifier:NSToolbarFlexibleSpaceItemIdentifier];

    const NSInteger low = NSToolbarItemVisibilityPriorityLow;
    const NSInteger aboveLow = low + 1;
    const NSInteger belowLow = low - 1;
    const NSInteger std = NSToolbarItemVisibilityPriorityStandard;
    const NSInteger aboveStd = std + 1;
    const NSInteger belowStd = std - 1;
    const NSInteger high = NSToolbarItemVisibilityPriorityHigh;
    const NSInteger aboveHigh = high + 1;
    const NSInteger belowHigh = high - 1;

    const bool isRTL = ThemeUtils::IsRightToLeft();

#define MAKE_SEGMENTED_PAIR_ITEM_RTL_AWARE(GROUP, PRIORITY, ITEM1, ICON1_LTR, ICON1_RTL, ITEM2, ICON2_LTR, ICON2_RTL) \
    [self makeSegmentedPairItem:m_toolBarData->GROUP \
            withGroupIdentifier:@#GROUP \
         withVisibilityPriority:PRIORITY \
                  withFirstItem:m_toolBarData->ITEM1 \
            withFirstIdentifier:@#ITEM1 \
                 withFirstImage:NSImageForIconType(isRTL ? ThemeUtils::ICON1_RTL : ThemeUtils::ICON1_LTR) \
                 withSecondItem:m_toolBarData->ITEM2 \
           withSecondIdentifier:@#ITEM2 \
                withSecondImage:NSImageForIconType(isRTL ? ThemeUtils::ICON2_RTL : ThemeUtils::ICON2_LTR) \
    ]
#define MAKE_SEGMENTED_PAIR_ITEM(GROUP, PRIORITY, ITEM1, ICON1, ITEM2, ICON2) \
    MAKE_SEGMENTED_PAIR_ITEM_RTL_AWARE(GROUP, PRIORITY, ITEM1, ICON1, ICON1, ITEM2, ICON2, ICON2)
#define MAKE_BUTTONED_ITEM(ITEM, PRIORITY, ICON) \
    [self makeButtonedItem:m_toolBarData->ITEM \
            withIdentifier:@#ITEM \
    withVisibilityPriority:PRIORITY \
                 withImage:NSImageForIconType(ThemeUtils::ICON, false) \
        withAlternateImage:NSImageForIconType(ThemeUtils::ICON, true) \
    ]

    MAKE_SEGMENTED_PAIR_ITEM_RTL_AWARE(navigateGroup, aboveHigh, navigatePrevious, ICON_GO_PREVIOUS, ICON_GO_NEXT, navigateNext, ICON_GO_NEXT, ICON_GO_PREVIOUS);
    MAKE_BUTTONED_ITEM(startSlideShow, std, ICON_MEDIA_PLAYBACK_START);
    MAKE_SEGMENTED_PAIR_ITEM(zoomGroup, high, zoomOut, ICON_ZOOM_OUT, zoomIn, ICON_ZOOM_IN);
    MAKE_BUTTONED_ITEM(zoomFitToWindow, belowHigh, ICON_ZOOM_FIT_BEST);
    MAKE_BUTTONED_ITEM(zoomOriginalSize, belowHigh, ICON_ZOOM_ORIGINAL);
    MAKE_BUTTONED_ITEM(zoomFullScreen, aboveLow, ICON_VIEW_FULLSCREEN);
    MAKE_SEGMENTED_PAIR_ITEM(rotateGroup, high, rotateCounterclockwise, ICON_OBJECT_ROTATE_LEFT, rotateClockwise, ICON_OBJECT_ROTATE_RIGHT);
    MAKE_SEGMENTED_PAIR_ITEM(flipGroup, belowStd, flipHorizontal, ICON_OBJECT_FLIP_HORIZONTAL, flipVertical, ICON_OBJECT_FLIP_VERTICAL);
    MAKE_BUTTONED_ITEM(openFile, aboveHigh, ICON_DOCUMENT_OPEN);
    MAKE_BUTTONED_ITEM(saveFileAs, aboveStd, ICON_DOCUMENT_SAVE_AS);
    MAKE_BUTTONED_ITEM(deleteFile, aboveStd, ICON_EDIT_DELETE);
    MAKE_BUTTONED_ITEM(print, aboveLow, ICON_DOCUMENT_PRINT);
    MAKE_BUTTONED_ITEM(preferences, low, ICON_EDIT_PREFERENCES);
    MAKE_BUTTONED_ITEM(exit, belowLow, ICON_APPLICATION_EXIT);

#undef MAKE_SEGMENTED_PAIR_ITEM
#undef MAKE_SEGMENTED_PAIR_ITEM_RTL_AWARE
#undef MAKE_BUTTONED_ITEM

    [m_toolBarData->zoomFitToWindow.button setCheckable:YES];
    [m_toolBarData->zoomOriginalSize.button setCheckable:YES];
    [m_toolBarData->zoomFullScreen.button setCheckable:YES];

    return self;
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
     itemForItemIdentifier:(NSString *)itemIdentifier
 willBeInsertedIntoToolbar:(BOOL)flag
{
    (void)(toolbar);
    (void)(flag);
#define CHECK(MEMBER) \
    if(m_toolBarData->MEMBER.item && [itemIdentifier compare:[m_toolBarData->MEMBER.item itemIdentifier]] == NSOrderedSame) \
        return m_toolBarData->MEMBER.item
    CHECK(space);
    CHECK(flexibleSpace);
    CHECK(navigateGroup);
    CHECK(startSlideShow);
    CHECK(zoomGroup);
    CHECK(zoomFitToWindow);
    CHECK(zoomOriginalSize);
    CHECK(zoomFullScreen);
    CHECK(rotateGroup);
    CHECK(flipGroup);
    CHECK(openFile);
    CHECK(saveFileAs);
    CHECK(deleteFile);
    CHECK(print);
    CHECK(preferences);
    CHECK(exit);
#undef CHECK
    return nil;
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
    (void)(toolbar);
    return [NSArray arrayWithObjects:
            m_toolBarData->navigateGroup.identifier(),
            m_toolBarData->startSlideShow.identifier(),
            m_toolBarData->zoomGroup.identifier(),
            m_toolBarData->zoomFitToWindow.identifier(),
            m_toolBarData->zoomOriginalSize.identifier(),
            m_toolBarData->zoomFullScreen.identifier(),
            m_toolBarData->rotateGroup.identifier(),
            m_toolBarData->flipGroup.identifier(),
            m_toolBarData->openFile.identifier(),
            m_toolBarData->saveFileAs.identifier(),
            m_toolBarData->deleteFile.identifier(),
#if defined (ENABLE_PRINT_SUPPORT)
            m_toolBarData->print.identifier(),
#endif
            m_toolBarData->preferences.identifier(),
            m_toolBarData->exit.identifier(),
            m_toolBarData->space.identifier(),
            m_toolBarData->flexibleSpace.identifier(),
            nil];
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
    (void)(toolbar);
    return [NSArray arrayWithObjects:
            m_toolBarData->navigateGroup.identifier(),
            m_toolBarData->startSlideShow.identifier(),
            m_toolBarData->flexibleSpace.identifier(),
            m_toolBarData->zoomGroup.identifier(),
            m_toolBarData->zoomFitToWindow.identifier(),
            m_toolBarData->zoomOriginalSize.identifier(),
            m_toolBarData->zoomFullScreen.identifier(),
            m_toolBarData->flexibleSpace.identifier(),
            m_toolBarData->rotateGroup.identifier(),
            m_toolBarData->flipGroup.identifier(),
            m_toolBarData->flexibleSpace.identifier(),
            m_toolBarData->openFile.identifier(),
            m_toolBarData->saveFileAs.identifier(),
            m_toolBarData->deleteFile.identifier(),
            m_toolBarData->flexibleSpace.identifier(),
            m_toolBarData->preferences.identifier(),
            m_toolBarData->exit.identifier(),
            nil];
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar
{
    (void)(toolbar);
    return nil;
}

- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    return [item isEnabled];
}

- (BOOL)validateToolbarItem:(NSToolbarItem *)item
{
    return [item isEnabled];
}

- (IBAction)itemClicked:(id)sender
{
#define INVOKE_IF_MATCH(ITEM, METHOD) \
    if(ITEM.actionSenderMatch(sender)) \
    { \
        QMetaObject::invokeMethod(m_emitterObject, METHOD); \
        return; \
    }
    INVOKE_IF_MATCH(m_toolBarData->navigatePrevious         , "navigatePreviousRequested"       )
    INVOKE_IF_MATCH(m_toolBarData->navigateNext             , "navigateNextRequested"           )
    INVOKE_IF_MATCH(m_toolBarData->startSlideShow           , "startSlideShowRequested"         )
    INVOKE_IF_MATCH(m_toolBarData->zoomOut                  , "zoomOutRequested"                )
    INVOKE_IF_MATCH(m_toolBarData->zoomIn                   , "zoomInRequested"                 )
    INVOKE_IF_MATCH(m_toolBarData->zoomFitToWindow          , "zoomFitToWindowRequested"        )
    INVOKE_IF_MATCH(m_toolBarData->zoomOriginalSize         , "zoomOriginalSizeRequested"       )
    INVOKE_IF_MATCH(m_toolBarData->zoomFullScreen           , "zoomFullScreenRequested"         )
    INVOKE_IF_MATCH(m_toolBarData->rotateCounterclockwise   , "rotateCounterclockwiseRequested" )
    INVOKE_IF_MATCH(m_toolBarData->rotateClockwise          , "rotateClockwiseRequested"        )
    INVOKE_IF_MATCH(m_toolBarData->flipHorizontal           , "flipHorizontalRequested"         )
    INVOKE_IF_MATCH(m_toolBarData->flipVertical             , "flipVerticalRequested"           )
    INVOKE_IF_MATCH(m_toolBarData->openFile                 , "openFileRequested"               )
    INVOKE_IF_MATCH(m_toolBarData->saveFileAs               , "saveAsRequested"                 )
    INVOKE_IF_MATCH(m_toolBarData->deleteFile               , "deleteFileRequested"             )
    INVOKE_IF_MATCH(m_toolBarData->print                    , "printRequested"                  )
    INVOKE_IF_MATCH(m_toolBarData->preferences              , "preferencesRequested"            )
    INVOKE_IF_MATCH(m_toolBarData->exit                     , "exitRequested"                   )
#undef INVOKE_IF_MATCH
    LOG_WARNING() << LOGGING_CTX << "No match for" << sender;
}

- (void)makeSegmentedPairItem:(SegmentedToolBarItem &)groupItem
          withGroupIdentifier:(NSString *)groupIdentifier
       withVisibilityPriority:(NSInteger)visibilityPriority
                withFirstItem:(GroupedToolBarItem &)firstItem
          withFirstIdentifier:(NSString *)firstIdentifier
               withFirstImage:(NSImage *)firstImage
               withSecondItem:(GroupedToolBarItem &)secondItem
         withSecondIdentifier:(NSString *)secondIdentifier
              withSecondImage:(NSImage *)secondImage
{
    NSToolbarItem *first = [[NSToolbarItem alloc] initWithItemIdentifier:firstIdentifier];
    [first setMenuFormRepresentation:[[NSMenuItem alloc] init]];
//    [first setTarget:self];
//    [first setAction:@selector(itemClicked:)];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
    if([[first menuFormRepresentation] respondsToSelector:@selector(setIdentifier:)])
        [[first menuFormRepresentation] setIdentifier:firstIdentifier];
#endif
    [[first menuFormRepresentation] setTarget:self];
    [[first menuFormRepresentation] setAction:@selector(itemClicked:)];
    firstItem.item = first;

    NSToolbarItem *second = [[NSToolbarItem alloc] initWithItemIdentifier:secondIdentifier];
    [second setMenuFormRepresentation:[[NSMenuItem alloc] init]];
//    [second setTarget:self];
//    [second setAction:@selector(itemClicked:)];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
    if([[second menuFormRepresentation] respondsToSelector:@selector(setIdentifier:)])
        [[second menuFormRepresentation] setIdentifier:secondIdentifier];
#endif
    [[second menuFormRepresentation] setTarget:self];
    [[second menuFormRepresentation] setAction:@selector(itemClicked:)];
    secondItem.item = second;

    NSSegmentedControl *segmentedControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect(0, 0, 2 * GROUPED_BUTTON_WIDTH + SEGMENTED_OFFSET, BUTTON_HEIGHT)];
    [segmentedControl setSegmentStyle:NSSegmentStyleTexturedRounded];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER)
    if([segmentedControl respondsToSelector:@selector(setTrackingMode:)])
        [segmentedControl setTrackingMode:NSSegmentSwitchTrackingMomentary];
#endif
    [segmentedControl setSegmentCount:2];
    [segmentedControl setImage:firstImage forSegment:0];
    [segmentedControl setWidth:GROUPED_BUTTON_WIDTH forSegment:0];
    [segmentedControl setImage:secondImage forSegment:1];
    [segmentedControl setWidth:GROUPED_BUTTON_WIDTH forSegment:1];
    groupItem.segmentedControl = segmentedControl;
    firstItem.segmentedControl = segmentedControl;
    secondItem.segmentedControl = segmentedControl;
    [segmentedControl setTarget:self];
    [segmentedControl setAction:@selector(itemClicked:)];

    NSMenu *groupSubMenu = [[NSMenu alloc] init];
    [groupSubMenu addItem:[first menuFormRepresentation]];
    [groupSubMenu addItem:[second menuFormRepresentation]];

    NSMenuItem *groupSubMenuItem = [[NSMenuItem alloc] init];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
    if([groupSubMenuItem respondsToSelector:@selector(setIdentifier:)])
        [groupSubMenuItem setIdentifier:groupIdentifier];
#endif
    [groupSubMenuItem setSubmenu:groupSubMenu];

    NSToolbarItemGroup *group = [[NSToolbarItemGroup alloc] initWithItemIdentifier:groupIdentifier];
    [group setSubitems:[NSArray arrayWithObjects:first, second, nil]];
    [group setView:segmentedControl];
    [group setMenuFormRepresentation:groupSubMenuItem];
    [group setVisibilityPriority:visibilityPriority];
    groupItem.item = group;
    firstItem.group = group;
    secondItem.group = group;
}

- (void)makeButtonedItem:(ButtonedToolBarItem &)buttonedItem
          withIdentifier:(NSString *)identifier
  withVisibilityPriority:(NSInteger)visibilityPriority
               withImage:(NSImage *)image
      withAlternateImage:(NSImage *)alternateImage
{
    CheckableNSButton *button = [[CheckableNSButton alloc] initWithFrame:NSMakeRect(0, 0, ALONE_BUTTON_WIDTH, BUTTON_HEIGHT)];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_14_AND_LATER)
    [button setBezelStyle:NSBezelStyleTexturedRounded];
#else
    [button setBezelStyle:NSTexturedRoundedBezelStyle];
#endif
    [button setImage:image];
    [button setAlternateImage:alternateImage];
    [button setTitle:@""];
    [button setTarget:self];
    [button setAction:@selector(itemClicked:)];

    NSMenuItem* menuItem = [[NSMenuItem alloc] init];
    [menuItem setTitle:@""];
#if defined (AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
    if([menuItem respondsToSelector:@selector(setIdentifier:)])
        [menuItem setIdentifier:identifier];
#endif
    [menuItem setTarget:self];
    [menuItem setAction:@selector(itemClicked:)];

    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
    [item setView:button];
    [item setMenuFormRepresentation:menuItem];
    [item setVisibilityPriority:visibilityPriority];
    buttonedItem.item = item;
    buttonedItem.button = button;
}

@end

// ====================================================================================================

struct MacToolBar::Impl
{
    MacToolBar *macToolBar;
    ToolBarData toolBarData;
    CustomNSToolbar *nativeToolbar;
    MacToolbarDelegate *delegate;
    QWidget *widget;

    bool isSlideShowMode;

    explicit Impl(MacToolBar *macToolBar)
        : macToolBar(macToolBar)
        , widget(Q_NULLPTR)
        , isSlideShowMode(false)
    {
        AUTORELEASE_POOL;
        nativeToolbar = [[CustomNSToolbar alloc] initWithIdentifier:@"MacToolBar" andEmitterObject:macToolBar];
        delegate = [[MacToolbarDelegate alloc] initWithToolBarData:&toolBarData andEmitterObject:macToolBar];
        [nativeToolbar setDelegate:delegate];
        [nativeToolbar setAllowsUserCustomization:YES];
        [nativeToolbar setAutosavesConfiguration:YES];

        retranslate();
    }

    ~Impl()
    {
        AUTORELEASE_POOL;
//        macToolBar->detachFromWindow();
        [nativeToolbar setDelegate:nil];
//        [delegate release];
        [nativeToolbar release];
    }

    void retranslate()
    {
        toolBarData.navigatePrevious.setToolTip(qApp->translate("MacToolBar", "Previous"));
        toolBarData.navigatePrevious.setMenuTitle(qApp->translate("MacToolBar", "Previous", "Short form of 'Previous'"));
        toolBarData.navigateNext.setToolTip(qApp->translate("MacToolBar", "Next"));
        toolBarData.navigateNext.setMenuTitle(qApp->translate("MacToolBar", "Next", "Short form of 'Next'"));
        const QString navigateGroupText = qApp->translate("MacToolBar", "Navigate", "Short form of 'Navigate'");
        toolBarData.navigateGroup.setPaletteLabel(navigateGroupText);
        toolBarData.navigateGroup.setLabel(navigateGroupText);

        const QString slideShowShortText = qApp->translate("MacToolBar", "Slideshow", "Short form of 'Slideshow'");
        toolBarData.startSlideShow.setPaletteLabel(slideShowShortText);
        toolBarData.startSlideShow.setLabel(slideShowShortText);

        toolBarData.zoomOut.setToolTip(qApp->translate("MacToolBar", "Zoom Out"));
        toolBarData.zoomOut.setMenuTitle(qApp->translate("MacToolBar", "Zoom Out", "Short form of 'Zoom Out'"));
        toolBarData.zoomIn.setToolTip(qApp->translate("MacToolBar", "Zoom In"));
        toolBarData.zoomIn.setMenuTitle(qApp->translate("MacToolBar", "Zoom In", "Short form of 'Zoom In'"));
        const QString zoomGroupText = qApp->translate("MacToolBar", "Zoom", "Short form of 'Zoom'");
        toolBarData.zoomGroup.setPaletteLabel(zoomGroupText);
        toolBarData.zoomGroup.setLabel(zoomGroupText);

        const QString zoomFitToWindowFullText = qApp->translate("MacToolBar", "Fit Image To Window Size");
        const QString zoomFitToWindowShortText = qApp->translate("MacToolBar", "Fit", "Short form of 'Fit Image To Window Size'");
        toolBarData.zoomFitToWindow.setPaletteLabel(zoomFitToWindowShortText);
        toolBarData.zoomFitToWindow.setToolTip(zoomFitToWindowFullText);
        toolBarData.zoomFitToWindow.setLabel(zoomFitToWindowShortText);

        const QString zoomOriginalSizeFullText = qApp->translate("MacToolBar", "Original Size");
        const QString zoomOriginalSizeShortText = qApp->translate("MacToolBar", "1:1", "Short form of 'Original Size'");
        toolBarData.zoomOriginalSize.setPaletteLabel(zoomOriginalSizeShortText);
        toolBarData.zoomOriginalSize.setToolTip(zoomOriginalSizeFullText);
        toolBarData.zoomOriginalSize.setLabel(zoomOriginalSizeShortText);

        const QString zoomFullScreenFullText = qApp->translate("MacToolBar", "Full Screen");
        const QString zoomFullScreenShortText = qApp->translate("MacToolBar", "Full Screen", "Short form of 'Full Screen'");
        toolBarData.zoomFullScreen.setPaletteLabel(zoomFullScreenShortText);
        toolBarData.zoomFullScreen.setToolTip(zoomFullScreenFullText);
        toolBarData.zoomFullScreen.setLabel(zoomFullScreenShortText);

        toolBarData.rotateCounterclockwise.setToolTip(qApp->translate("MacToolBar", "Rotate Counterclockwise"));
        toolBarData.rotateCounterclockwise.setMenuTitle(qApp->translate("MacToolBar", "Rotate Counterclockwise", "Short form of 'Rotate Counterclockwise'"));
        toolBarData.rotateClockwise.setToolTip(qApp->translate("MacToolBar", "Rotate Clockwise"));
        toolBarData.rotateClockwise.setMenuTitle(qApp->translate("MacToolBar", "Rotate Clockwise", "Short form of 'Rotate Clockwise'"));
        const QString rotateGroupText = qApp->translate("MacToolBar", "Rotate", "Short form of 'Rotate'");
        toolBarData.rotateGroup.setPaletteLabel(rotateGroupText);
        toolBarData.rotateGroup.setLabel(rotateGroupText);

        toolBarData.flipHorizontal.setToolTip(qApp->translate("MacToolBar", "Flip Horizontal"));
        toolBarData.flipHorizontal.setMenuTitle(qApp->translate("MacToolBar", "Flip Horizontal", "Short form of 'Flip Horizontal'"));
        toolBarData.flipVertical.setToolTip(qApp->translate("MacToolBar", "Flip Vertical"));
        toolBarData.flipVertical.setMenuTitle(qApp->translate("MacToolBar", "Flip Vertical", "Short form of 'Flip Vertical'"));
        const QString flipGroupText = qApp->translate("MacToolBar", "Flip", "Short form of 'Flip'");
        toolBarData.flipGroup.setPaletteLabel(flipGroupText);
        toolBarData.flipGroup.setLabel(flipGroupText);

        const QString openFileFullText = qApp->translate("MacToolBar", "Open File");
        const QString openFileShortText = qApp->translate("MacToolBar", "Open", "Short form of 'Open File'");
        toolBarData.openFile.setPaletteLabel(openFileShortText);
        toolBarData.openFile.setToolTip(openFileFullText);
        toolBarData.openFile.setLabel(openFileShortText);
        const QString saveFileAsFullText = qApp->translate("MacToolBar", "Save File As");
        const QString saveFileAsShortText = qApp->translate("MacToolBar", "Save", "Short form of 'Save File As'");
        toolBarData.saveFileAs.setPaletteLabel(saveFileAsShortText);
        toolBarData.saveFileAs.setToolTip(saveFileAsFullText);
        toolBarData.saveFileAs.setLabel(saveFileAsShortText);
        const QString deleteFileFullText = qApp->translate("MacToolBar", "Delete File");
        const QString deleteFileShortText = qApp->translate("MacToolBar", "Delete", "Short form of 'Delete File'");
        toolBarData.deleteFile.setPaletteLabel(deleteFileShortText);
        toolBarData.deleteFile.setToolTip(deleteFileFullText);
        toolBarData.deleteFile.setLabel(deleteFileShortText);
        const QString printFullText = qApp->translate("MacToolBar", "Print");
        const QString printShortText = qApp->translate("MacToolBar", "Print", "Short form of 'Print'");
        toolBarData.print.setPaletteLabel(printShortText);
        toolBarData.print.setToolTip(printFullText);
        toolBarData.print.setLabel(printShortText);
        const QString preferencesFullText = qApp->translate("MacToolBar", "Preferences");
        const QString preferencesShortText = qApp->translate("MacToolBar", "Preferences", "Short form of 'Preferences'");
        toolBarData.preferences.setPaletteLabel(preferencesShortText);
        toolBarData.preferences.setToolTip(preferencesFullText);
        toolBarData.preferences.setLabel(preferencesShortText);
        const QString exitFullText = qApp->translate("MacToolBar", "Quit");
        const QString exitShortText = qApp->translate("MacToolBar", "Quit", "Short form of 'Quit'");
        toolBarData.exit.setPaletteLabel(exitShortText);
        toolBarData.exit.setToolTip(exitFullText);
        toolBarData.exit.setLabel(exitShortText);

        setSlideShowMode(isSlideShowMode);
    }

    void setSlideShowMode(bool isSlideShow)
    {
        AUTORELEASE_POOL;
        isSlideShowMode = isSlideShow;
        NSButton *button = toolBarData.startSlideShow.button;
        if(!button)
            return;
        if(!isSlideShowMode)
        {
            [button setImage:NSImageForIconType(ThemeUtils::ICON_MEDIA_SLIDESHOW)];
            toolBarData.startSlideShow.setToolTip(qApp->translate("MacToolBar", "Start Slideshow"));
        }
        else
        {
            [button setImage:NSImageForIconType(ThemeUtils::ICON_MEDIA_PLAYBACK_STOP)];
            toolBarData.startSlideShow.setToolTip(qApp->translate("MacToolBar", "Stop Slideshow"));
        }
    }
};

// ====================================================================================================

MacToolBar::MacToolBar(QObject *parent)
    : ControlsContainerEmitter(parent)
    , m_impl(new Impl(this))
{
    connect(LocalizationManager::instance(), SIGNAL(localeChanged(QString)), this, SLOT(retranslate()));
}

MacToolBar::~MacToolBar()
{}

ControlsContainerEmitter *MacToolBar::emitter()
{
    return this;
}

void MacToolBar::attachToWindow(QWidget *widget)
{
    AUTORELEASE_POOL;
    detachFromWindow();
    m_impl->widget = widget;
    if(NSWindow *window = WindowUtils::GetNativeWindow(m_impl->widget))
    {
        [window setToolbar:m_impl->nativeToolbar];
        [window setShowsToolbarButton:YES];
    }
}

void MacToolBar::detachFromWindow()
{
    AUTORELEASE_POOL;
    if(NSWindow *window = WindowUtils::GetNativeWindow(m_impl->widget))
        [window setToolbar:nil];
    m_impl->widget = Q_NULLPTR;
}

void MacToolBar::setVisible(bool isVisible)
{
    AUTORELEASE_POOL;
    if(NSWindow *window = WindowUtils::GetNativeWindow(m_impl->widget))
    {
        [window setToolbar:m_impl->nativeToolbar];
        [window setShowsToolbarButton:YES];
    }
    [m_impl->nativeToolbar setManualVisible:(isVisible ? YES : NO)];
}

void MacToolBar::retranslate()
{
    m_impl->retranslate();
}

CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setOpenFileEnabled, &m_impl->toolBarData.openFile)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setOpenFolderEnabled)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setSaveAsEnabled, &m_impl->toolBarData.saveFileAs)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setNewWindowEnabled)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setNavigatePreviousEnabled, &m_impl->toolBarData.navigatePrevious)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setNavigateNextEnabled, &m_impl->toolBarData.navigateNext)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setStartSlideShowEnabled, &m_impl->toolBarData.startSlideShow)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setImageInformationEnabled)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setPrintEnabled, &m_impl->toolBarData.print)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setPreferencesEnabled, &m_impl->toolBarData.preferences)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setExitEnabled, &m_impl->toolBarData.exit)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setRotateCounterclockwiseEnabled, &m_impl->toolBarData.rotateCounterclockwise)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setRotateClockwiseEnabled, &m_impl->toolBarData.rotateClockwise)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setFlipHorizontalEnabled, &m_impl->toolBarData.flipHorizontal)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setFlipVerticalEnabled, &m_impl->toolBarData.flipVertical)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setDeleteFileEnabled, &m_impl->toolBarData.deleteFile)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setZoomOutEnabled, &m_impl->toolBarData.zoomOut)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setZoomInEnabled, &m_impl->toolBarData.zoomIn)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setZoomResetEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setZoomCustomEnabled)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setZoomFitToWindowEnabled, &m_impl->toolBarData.zoomFitToWindow)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setZoomOriginalSizeEnabled, &m_impl->toolBarData.zoomOriginalSize)
CONTROLS_CONTAINER_SET_ENABLED_IMPL(MacToolBar, setZoomFullScreenEnabled, &m_impl->toolBarData.zoomFullScreen)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setShowMenuBarEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setShowToolBarEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setAboutEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setAboutQtEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setCheckForUpdatesEnabled)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setEditStylesheetEnabled)

CONTROLS_CONTAINER_SET_CHECKED_IMPL(MacToolBar, setZoomFitToWindowChecked, &m_impl->toolBarData.zoomFitToWindow)
CONTROLS_CONTAINER_SET_CHECKED_IMPL(MacToolBar, setZoomOriginalSizeChecked, &m_impl->toolBarData.zoomOriginalSize)
CONTROLS_CONTAINER_SET_CHECKED_IMPL(MacToolBar, setZoomFullScreenChecked, &m_impl->toolBarData.zoomFullScreen)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setShowMenuBarChecked)
CONTROLS_CONTAINER_EMPTY_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setShowToolBarChecked)

CONTROLS_CONTAINER_BOOL_ARG_FUNCTION_IMPL(MacToolBar, setSlideShowMode, m_impl, setSlideShowMode)

// ====================================================================================================
