#include "ui/canvas/ClipboardImage.h"

#include "utils/Logger.h"

#import <AppKit/AppKit.h>

#include <optional>
#include <string>

namespace mind_map::ui {

std::optional<std::string> GetClipboardImagePng() {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];

        // PNG is the cheapest path — no conversion.
        if (NSData* const data = [pb dataForType:NSPasteboardTypePNG]) {
            return std::string(static_cast<const char*>(data.bytes),
                               static_cast<size_t>(data.length));
        }

        // Screenshots and Preview copies land on the clipboard as TIFF.
        // Convert via NSBitmapImageRep.
        if (NSData* const tiff = [pb dataForType:NSPasteboardTypeTIFF]) {
            NSBitmapImageRep* const rep = [NSBitmapImageRep imageRepWithData:tiff];
            if (!rep) {
                LOG_WARNING_BUILD("GetClipboardImagePng: failed to read TIFF from clipboard");
                return std::nullopt;
            }
            NSData* const png = [rep representationUsingType:NSBitmapImageFileTypePNG
                                                  properties:@{}];
            if (!png) {
                LOG_WARNING_BUILD("GetClipboardImagePng: failed to convert clipboard TIFF to PNG");
                return std::nullopt;
            }
            return std::string(static_cast<const char*>(png.bytes),
                               static_cast<size_t>(png.length));
        }

        return std::nullopt;
    }
}

}  // namespace mind_map::ui
