// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeIdleRunner.h"
#import <Cocoa/Cocoa.h>

@interface NativeIdleRunnerTimerHandler : NSObject
-(id)initWithCallback:(void (*)(void*))cbfn userData:(void*)cbdata;
-(void)onTimer:(id)timer;
@end

struct NativeIdleRunner::Impl {
    NSTimer* timer = nullptr;
    NSObject* handler = nullptr;
};

NativeIdleRunner::NativeIdleRunner()
    : impl_(new Impl)
{
}

NativeIdleRunner::~NativeIdleRunner()
{
    stop();
}

void NativeIdleRunner::start(double interval, void(*cbfn)(void*), void* cbdata)
{
    stop();

    NSObject* handler = [[NativeIdleRunnerTimerHandler alloc] initWithCallback:cbfn userData:cbdata];
    impl_->handler = [handler retain];

    NSTimer* timer = [NSTimer timerWithTimeInterval:interval
                                             target:handler
                                           selector:@selector(onTimer:)
                                           userInfo:nil
                                            repeats:YES];
    impl_->timer = [timer retain];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

void NativeIdleRunner::stop()
{
    NSTimer* timer = impl_->timer;
    [timer invalidate];
    [timer release];
    impl_->timer = nullptr;

    NSObject* handler = impl_->handler;
    [handler release];
    impl_->handler = nullptr;
}

@implementation NativeIdleRunnerTimerHandler
{
    void (*cbfn_)(void*);
    void* cbdata_;
}

-(id)initWithCallback:(void (*)(void*))cbfn userData:(void*)cbdata
{
    [super init];
    self->cbfn_ = cbfn;
    self->cbdata_ = cbdata;
    return self;
}

-(void)onTimer:(id)timer
{
    (void)timer;
    self->cbfn_(self->cbdata_);
}
@end
