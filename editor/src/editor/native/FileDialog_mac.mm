// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileDialog.h"
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

struct FileDialog::CocoaImpl {
    FileDialog* Q = nullptr;
    id panel = nullptr;
    id delegate = nullptr;

    void onFileChosen(absl::string_view path)
    {
        if (Q->onFileChosen)
            Q->onFileChosen(path);
    }
};

@interface FileDialogDelegate : NSObject
{
    FileDialog::CocoaImpl* impl_;
}
-(id)initWithImpl:(FileDialog::CocoaImpl*)impl;
-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation FileDialogDelegate
-(id)initWithImpl:(FileDialog::CocoaImpl*)impl
{
    [super init];
    self->impl_ = impl;
    return self;
}

-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    NSSavePanel* panel = self->impl_->panel;
    self->impl_->panel = nullptr;

    if (returnCode == NSOKButton)
    {
        NSArray* urls = [panel URLs];
        NSURL* url;
        if ([urls count] == 1)
            url = [urls objectAtIndex:0];
        if (url && [url isFileURL]) {
            self->impl_->onFileChosen([url.path UTF8String]);
            return;
        }
    }

    self->impl_->onFileChosen({});
}
@end

FileDialog::FileDialog()
    : impl_(new CocoaImpl)
{
    impl_->Q = this;
}

FileDialog::~FileDialog()
{
    if (impl_->panel)
        [impl_->panel release];
    if (impl_->delegate)
        [impl_->delegate release];
}

bool FileDialog::chooseFile()
{
    if (impl_->panel) {
        [impl_->panel makeKeyAndOrderFront:nil];
        return false;
    }

    NSSavePanel* panel = impl_->panel = (mode_ == Mode::Save) ?
        [NSSavePanel savePanel] : [NSOpenPanel openPanel];

    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];

    if (!title_.empty())
        [panel setTitle:[NSString stringWithUTF8String:path_.c_str()]];

    if (!path_.empty())
        [panel setDirectory:[NSString stringWithUTF8String:path_.c_str()]];

    // TODO: file type filters

    id delegate = impl_->delegate;
    if (!delegate) {
        delegate = [[FileDialogDelegate alloc] initWithImpl:impl_.get()];
        impl_->delegate = delegate;
    }

    [panel beginSheetForDirectory:nullptr
                             file:nullptr
                   modalForWindow:nullptr
                    modalDelegate:delegate
                   didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                      contextInfo:nullptr];

    return true;
}
