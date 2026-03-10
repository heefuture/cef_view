#import "OsrCefTextInputClient.h"
#include "utils/LogUtil.h"
#include "include/cef_browser.h"

extern "C" {
extern NSString* NSTextInputReplacementRangeAttributeName;
}

@implementation OsrCefTextInputClient

@synthesize selectedRange = _selectedRange;
@synthesize handlingKeyDown = _handlingKeyDown;

- (id)initWithBrowser:(CefRefPtr<CefBrowser>)browser {
    self = [super init];
    if (self) {
        _browser = browser;
        _handlingKeyDown = NO;
        _hasMarkedText = NO;
        _oldHasMarkedText = NO;
        _unmarkTextCalled = NO;
        _markedRange = NSMakeRange(NSNotFound, 0);
        _selectedRange = NSMakeRange(NSNotFound, 0);
        _replacementRange = CefRange(UINT32_MAX, UINT32_MAX);
        _validAttributesForMarkedText = @[
            NSUnderlineStyleAttributeName,
            NSUnderlineColorAttributeName,
            NSMarkedClauseSegmentAttributeName,
            NSTextInputReplacementRangeAttributeName
        ];
    }
    return self;
}

- (void)detach {
    _browser = nullptr;
}

- (void)setBrowser:(CefRefPtr<CefBrowser>)browser {
    _browser = browser;
}

#pragma mark - NSTextInputClient Protocol

- (BOOL)hasMarkedText {
    return _hasMarkedText;
}

- (NSRange)markedRange {
    return _markedRange;
}

- (NSArray*)validAttributesForMarkedText {
    return _validAttributesForMarkedText;
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return NSNotFound;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
    NSString* text = isAttributedString ? [string string] : string;

    if (_handlingKeyDown) {
        _textToBeInserted = [text UTF8String];
    } else {
        if (_browser && _browser->GetHost()) {
            CefString cefText([text UTF8String]);
            CefRange replacementRange(UINT32_MAX, UINT32_MAX);
            _browser->GetHost()->ImeCommitText(cefText, replacementRange, 0);
        }
    }

    // Clear marked text state
    _hasMarkedText = NO;
    _markedRange = NSMakeRange(NSNotFound, 0);
    _underlines.clear();
    _markedText.ClearAndFree();
    _replacementRange = CefRange(UINT32_MAX, UINT32_MAX);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
    NSString* text = isAttributedString ? [string string] : string;

    _markedText = CefString([text UTF8String]);
    _selectedRange = selectedRange;
    _hasMarkedText = [text length] > 0;

    if (_hasMarkedText) {
        _markedRange = NSMakeRange(0, [text length]);
    } else {
        _markedRange = NSMakeRange(NSNotFound, 0);
    }

    // Extract underline info from attributed string
    _underlines.clear();
    if (isAttributedString) {
        NSAttributedString* attrString = (NSAttributedString*)string;
        [attrString enumerateAttribute:NSUnderlineStyleAttributeName
                               inRange:NSMakeRange(0, [attrString length])
                               options:0
                            usingBlock:^(id value, NSRange range, BOOL* stop) {
            if (value) {
                CefCompositionUnderline underline;
                underline.range.from = static_cast<uint32_t>(range.location);
                underline.range.to = static_cast<uint32_t>(range.location + range.length);
                underline.thick = NO;
                underline.color = 0;
                underline.background_color = 0;
                _underlines.push_back(underline);
            }
        }];
    }

    // If no underlines extracted, create a default one for the whole text
    if (_underlines.empty() && _hasMarkedText) {
        CefCompositionUnderline underline;
        underline.range.from = 0;
        underline.range.to = static_cast<uint32_t>([text length]);
        underline.thick = NO;
        underline.color = 0;
        underline.background_color = 0;
        _underlines.push_back(underline);
    }

    // Handle replacement range
    if (replacementRange.location != NSNotFound) {
        _replacementRange = CefRange(
            static_cast<uint32_t>(replacementRange.location),
            static_cast<uint32_t>(replacementRange.location + replacementRange.length));
    } else {
        _replacementRange = CefRange(UINT32_MAX, UINT32_MAX);
    }

    if (!_handlingKeyDown) {
        if (_browser && _browser->GetHost()) {
            CefRange cefSelectedRange(
                static_cast<uint32_t>(selectedRange.location),
                static_cast<uint32_t>(selectedRange.location + selectedRange.length));
            _browser->GetHost()->ImeSetComposition(
                _markedText, _underlines, _replacementRange, cefSelectedRange);
        }
    }
}

- (void)unmarkText {
    _hasMarkedText = NO;
    _markedRange = NSMakeRange(NSNotFound, 0);
    _unmarkTextCalled = YES;

    if (!_handlingKeyDown) {
        if (_browser && _browser->GetHost()) {
            _browser->GetHost()->ImeFinishComposingText(false);
        }
    }
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    if (_compositionBounds.empty()) {
        return NSZeroRect;
    }

    // Use the first character bound for positioning
    size_t index = 0;
    if (range.location != NSNotFound && range.location < _compositionBounds.size()) {
        index = range.location;
    }

    CefRect cefRect = _compositionBounds[index];

    // Convert to screen coordinates
    NSRect rect = NSMakeRect(cefRect.x, cefRect.y, cefRect.width, cefRect.height);

    if (actualRange) {
        *actualRange = range;
    }

    return rect;
}

#pragma mark - Key Event Handling

- (void)HandleKeyEventBeforeTextInputClient:(NSEvent*)keyEvent {
    _handlingKeyDown = YES;
    _oldHasMarkedText = _hasMarkedText;
    _unmarkTextCalled = NO;
    _textToBeInserted.clear();
}

- (void)HandleKeyEventAfterTextInputClient:(CefKeyEvent)keyEvent {
    _handlingKeyDown = NO;

    if (!_browser || !_browser->GetHost()) {
        return;
    }

    auto host = _browser->GetHost();

    // If text was inserted via insertText:replacementRange:
    if (!_textToBeInserted.empty()) {
        // If there was marked text, we need to confirm the composition first
        if (_oldHasMarkedText) {
            CefString cefText(_textToBeInserted);
            CefRange range(UINT32_MAX, UINT32_MAX);
            host->ImeCommitText(cefText, range, 0);
        } else {
            // Send as a regular key event
            keyEvent.type = KEYEVENT_CHAR;
            host->SendKeyEvent(keyEvent);
        }
        return;
    }

    // If composition is active (marked text was set)
    if (_hasMarkedText) {
        CefRange cefSelectedRange(
            static_cast<uint32_t>(_selectedRange.location),
            static_cast<uint32_t>(_selectedRange.location + _selectedRange.length));
        host->ImeSetComposition(
            _markedText, _underlines, _replacementRange, cefSelectedRange);
        return;
    }

    // If unmarkText was called
    if (_unmarkTextCalled) {
        host->ImeFinishComposingText(false);
        return;
    }

    // No IME activity, send as regular key event
    if (!_oldHasMarkedText) {
        keyEvent.type = KEYEVENT_KEYDOWN;
        host->SendKeyEvent(keyEvent);
        keyEvent.type = KEYEVENT_CHAR;
        host->SendKeyEvent(keyEvent);
    }
}

#pragma mark - Composition Management

- (void)ChangeCompositionRange:(CefRange)range
              character_bounds:(const CefRenderHandler::RectList&)bounds {
    _compositionRange = range;
    _compositionBounds.assign(bounds.begin(), bounds.end());
}

- (void)cancelComposition {
    if (!_hasMarkedText) {
        return;
    }

    _hasMarkedText = NO;
    _markedRange = NSMakeRange(NSNotFound, 0);
    _underlines.clear();
    _markedText.ClearAndFree();

    if (_browser && _browser->GetHost()) {
        _browser->GetHost()->ImeCancelComposition();
    }
}

@end
