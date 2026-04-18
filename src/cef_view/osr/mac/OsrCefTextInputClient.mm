// Copyright 2016 The Chromium Embedded Framework Authors. Portions copyright
// 2013 The Chromium Authors. All rights reserved. Use of this source code is
// governed by a BSD-style license that can be found in the LICENSE file.

// Implementation based on
// content/browser/renderer_host/render_widget_host_view_mac.mm from Chromium.

#include "OsrCefTextInputClient.h"

#include "include/cef_client.h"

#define ColorBLACK 0xFF000000  // Same as Blink SKColor.

namespace {

cef_color_t CefColorFromNSColor(NSColor* color) {
  CGFloat r, g, b, a;
  [color getRed:&r green:&g blue:&b alpha:&a];

  return std::max(0, std::min(static_cast<int>(lroundf(255.0f * a)), 255))
             << 24 |
         std::max(0, std::min(static_cast<int>(lroundf(255.0f * r)), 255))
             << 16 |
         std::max(0, std::min(static_cast<int>(lroundf(255.0f * g)), 255))
             << 8 |
         std::max(0, std::min(static_cast<int>(lroundf(255.0f * b)), 255));
}

// Extract underline information from an attributed string. Mostly copied from
// third_party/WebKit/Source/WebKit/mac/WebView/WebHTMLView.mm
void ExtractUnderlines(NSAttributedString* string,
                       std::vector<CefCompositionUnderline>* underlines) {
  int length = static_cast<int>([[string string] length]);
  int i = 0;
  while (i < length) {
    NSRange range;
    NSDictionary* attrs = [string attributesAtIndex:i
                              longestEffectiveRange:&range
                                            inRange:NSMakeRange(i, length - i)];
    NSNumber* style = [attrs objectForKey:NSUnderlineStyleAttributeName];
    if (style) {
      cef_color_t color = ColorBLACK;
      if (NSColor* colorAttr =
              [attrs objectForKey:NSUnderlineColorAttributeName]) {
        color = CefColorFromNSColor(
            [colorAttr colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace]);
      }
      cef_composition_underline_t line = {
          sizeof(cef_composition_underline_t),
          {static_cast<uint32_t>(range.location),
           static_cast<uint32_t>(NSMaxRange(range))},
          color,
          0,
          [style intValue] > 1};
      underlines->push_back(line);
    }
    i = static_cast<int>(range.location + range.length);
  }
}

}  // namespace

extern "C" {
extern NSString* NSTextInputReplacementRangeAttributeName;
}

@implementation OsrCefTextInputClient

@synthesize selectedRange = _selectedRange;
@synthesize handlingKeyDown = _handlingKeyDown;

- (id)initWithBrowser:(CefRefPtr<CefBrowser>)browser {
  self = [super init];
  _browser = browser;
  return self;
}

- (void)detach {
  _browser = nullptr;
}

- (NSArray*)validAttributesForMarkedText {
  if (!_validAttributesForMarkedText) {
    _validAttributesForMarkedText = [[NSArray alloc]
        initWithObjects:NSUnderlineStyleAttributeName,
                        NSUnderlineColorAttributeName,
                        NSMarkedClauseSegmentAttributeName,
                        NSTextInputReplacementRangeAttributeName, nil];
  }
  return _validAttributesForMarkedText;
}

- (NSRange)selectedRange {
  if (_selectedRange.location == NSNotFound || _selectedRange.length == 0) {
    return NSMakeRange(NSNotFound, 0);
  }
  return _selectedRange;
}

- (NSRange)markedRange {
  return _hasMarkedText ? _markedRange : NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText {
  return _hasMarkedText;
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange {
  BOOL isAttributedString = [aString isKindOfClass:[NSAttributedString class]];
  NSString* im_text = isAttributedString ? [aString string] : aString;
  if (_handlingKeyDown) {
    _textToBeInserted.append([im_text UTF8String]);
  } else {
    cef_range_t range = {static_cast<uint32_t>(replacementRange.location),
                         static_cast<uint32_t>(NSMaxRange(replacementRange))};
    _browser->GetHost()->ImeCommitText([im_text UTF8String], range, 0);
  }

  // Inserting text will delete all marked text automatically.
  _hasMarkedText = NO;
}

- (void)doCommandBySelector:(SEL)aSelector {
  // An input method calls this function to dispatch an editing command to be
  // handled by this view.
}

- (void)setMarkedText:(id)aString
        selectedRange:(NSRange)newSelRange
     replacementRange:(NSRange)replacementRange {
  // An input method has updated the composition string. We send the given text
  // and range to the browser so it can update the composition node of Blink.

  BOOL isAttributedString = [aString isKindOfClass:[NSAttributedString class]];
  NSString* im_text = isAttributedString ? [aString string] : aString;
  uint32_t length = static_cast<uint32_t>([im_text length]);

  // |_markedRange| will get set in a callback from ImeSetComposition().
  _selectedRange = newSelRange;
  _markedText = [im_text UTF8String];
  _hasMarkedText = (length > 0);
  _underlines.clear();

  if (isAttributedString) {
    ExtractUnderlines(aString, &_underlines);
  } else {
    // Use a thin black underline by default.
    cef_composition_underline_t line = {
        sizeof(cef_composition_underline_t), {0, length}, ColorBLACK, 0, false};
    _underlines.push_back(line);
  }

  // If we are handling a key down event then ImeSetComposition() will be
  // called from the keyEvent: method.
  // Input methods of Mac use setMarkedText calls with empty text to cancel an
  // ongoing composition. Our input method backend will automatically cancel an
  // ongoing composition when we send empty text.
  if (_handlingKeyDown) {
    _replacementRange = {
        static_cast<uint32_t>(replacementRange.location),
        static_cast<uint32_t>(NSMaxRange(replacementRange))};
  } else if (!_handlingKeyDown) {
    CefRange replacement_range(
        static_cast<uint32_t>(replacementRange.location),
        static_cast<uint32_t>(NSMaxRange(replacementRange)));
    CefRange selection_range(static_cast<uint32_t>(newSelRange.location),
                             static_cast<uint32_t>(NSMaxRange(newSelRange)));

    _browser->GetHost()->ImeSetComposition(_markedText, _underlines,
                                           replacement_range, selection_range);
  }
}

- (void)unmarkText {
  // Delete the composition node of the browser and finish an ongoing
  // composition.
  // It seems that, instead of calling this method, an input method will call
  // the setMarkedText method with empty text to cancel ongoing composition.
  // Implement this method even though we don't expect it to be called.
  _hasMarkedText = NO;
  _markedText.clear();
  _underlines.clear();

  // If we are handling a key down event then ImeFinishComposingText() will be
  // called from the keyEvent: method.
  if (!_handlingKeyDown) {
    _browser->GetHost()->ImeFinishComposingText(false);
  } else {
    _unmarkTextCalled = YES;
  }
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                   (NSRangePointer)actualRange {
  // Modify the attributed string if required.
  // Not implemented here as we do not want to control the IME window view.
  return nil;
}

- (NSRect)firstViewRectForCharacterRange:(NSRange)theRange
                             actualRange:(NSRangePointer)actualRange {
  NSRect rect;

  NSUInteger location = theRange.location;

  // If location is not specified fall back to the composition range start.
  if (location == NSNotFound) {
    location = _markedRange.location;
  }

  // Offset location by the composition range start if required.
  if (location >= _markedRange.location) {
    location -= _markedRange.location;
  }

  if (location < _compositionBounds.size()) {
    const CefRect& rc = _compositionBounds[location];
    rect = NSMakeRect(rc.x, rc.y, rc.width, rc.height);
  }

  if (actualRange) {
    *actualRange = NSMakeRange(location, theRange.length);
  }

  return rect;
}

- (NSRect)screenRectFromViewRect:(NSRect)rect {
  NSRect screenRect;

  int screenX, screenY;
  _browser->GetHost()->GetClient()->GetRenderHandler()->GetScreenPoint(
      _browser, rect.origin.x, rect.origin.y, screenX, screenY);
  screenRect.origin = NSMakePoint(screenX, screenY);
  screenRect.size = rect.size;

  return screenRect;
}

- (NSRect)firstRectForCharacterRange:(NSRange)theRange
                         actualRange:(NSRangePointer)actualRange {
  NSRect rect = [self firstViewRectForCharacterRange:theRange
                                         actualRange:actualRange];

  // Convert into screen coordinates for return.
  rect = [self screenRectFromViewRect:rect];

  if (rect.origin.y >= rect.size.height) {
    rect.origin.y -= rect.size.height;
  } else {
    rect.origin.y = 0;
  }

  return rect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint {
  return NSNotFound;
}

- (void)HandleKeyEventBeforeTextInputClient:(NSEvent*)keyEvent {
  DCHECK([keyEvent type] == NSEventTypeKeyDown);
  // Don't call this method recursively.
  DCHECK(!_handlingKeyDown);

  _oldHasMarkedText = _hasMarkedText;
  _handlingKeyDown = YES;

  // These variables might be set when handling the keyboard event.
  // Clear them here so that we can know whether they have changed afterwards.
  _textToBeInserted.clear();
  _markedText.clear();
  _underlines.clear();
  _replacementRange = CefRange::InvalidRange();
  _unmarkTextCalled = NO;
}

- (void)HandleKeyEventAfterTextInputClient:(CefKeyEvent)keyEvent {
  _handlingKeyDown = NO;

  // Send keypress and/or composition related events.
  // Note that |_textToBeInserted| is a UTF-16 string but it's fine to only
  // handle BMP characters here as we can always insert non-BMP characters as
  // text.

  // If the text to be inserted only contains 1 character then we can just send
  // a keypress event.
  if (!_hasMarkedText && !_oldHasMarkedText &&
      _textToBeInserted.length() <= 1) {
    keyEvent.type = KEYEVENT_KEYDOWN;

    if (IsMacOsrShortcutChar(keyEvent.unmodified_character))
        keyEvent.unmodified_character = 0;

    _browser->GetHost()->SendKeyEvent(keyEvent);

    // Don't send a CHAR event for non-char keys like arrows, function keys and
    // clear.
    if (keyEvent.modifiers & (EVENTFLAG_IS_KEY_PAD)) {
      if (keyEvent.native_key_code == 71) {
        return;
      }
    }

    keyEvent.type = KEYEVENT_CHAR;
    _browser->GetHost()->SendKeyEvent(keyEvent);
  }

  // If the text to be inserted contains multiple characters then send the text
  // to the browser using ImeCommitText().
  BOOL textInserted = NO;
  if (_textToBeInserted.length() >
      ((_hasMarkedText || _oldHasMarkedText) ? 0u : 1u)) {
    _browser->GetHost()->ImeCommitText(_textToBeInserted,
                                       CefRange::InvalidRange(), 0);
    _textToBeInserted.clear();
  }

  // Update or cancel the composition. If some text has been inserted then we
  // don't need to explicitly cancel the composition.
  if (_hasMarkedText && _markedText.length()) {
    // Update the composition by sending marked text to the browser.
    // |_selectedRange| is the range being selected inside the marked text.
    _browser->GetHost()->ImeSetComposition(
        _markedText, _underlines, _replacementRange,
        CefRange(static_cast<uint32_t>(_selectedRange.location),
                 static_cast<uint32_t>(NSMaxRange(_selectedRange))));
  } else if (_oldHasMarkedText && !_hasMarkedText && !textInserted) {
    // There was no marked text or inserted text. Complete or cancel the
    // composition.
    if (_unmarkTextCalled) {
      _browser->GetHost()->ImeFinishComposingText(false);
    } else {
      _browser->GetHost()->ImeCancelComposition();
    }
  }

  _replacementRange = CefRange::InvalidRange();
}

- (void)ChangeCompositionRange:(CefRange)range
              character_bounds:(const CefRenderHandler::RectList&)bounds {
  _compositionRange = range;
  _markedRange = NSMakeRange(range.from, range.to - range.from);
  _compositionBounds = bounds;
}

- (void)cancelComposition {
  if (!_hasMarkedText) {
    return;
  }

// Cancel the ongoing composition. [NSInputManager markedTextAbandoned:]
// doesn't call any NSTextInput functions, such as setMarkedText or
// insertText.
// TODO(erikchen): NSInputManager is deprecated since OSX 10.6. Switch to
// NSTextInputContext. http://www.crbug.com/479010.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  NSInputManager* currentInputManager = [NSInputManager currentInputManager];
  [currentInputManager markedTextAbandoned:self];
#pragma clang diagnostic pop

  _hasMarkedText = NO;
  _markedText.clear();
  _underlines.clear();

  // Notify Blink to cancel the composition so it won't commit the pending
  // marked text into a newly focused input field during focus changes.
  if (_browser && _browser->GetHost()) {
    _browser->GetHost()->ImeCancelComposition();
  }
}

@end
