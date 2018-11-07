// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Application.h"
#include "iOSAnalytics.h"

void Analytics::RegisterLuaModule(LuaCpp::State *state) {
    state->RegisterModule("analytics", [](LuaCpp::Module &module) {
        module["init"].SetFunc(Analytics::Init);
        module["log"].SetFunc(Analytics::Log);
    });
};

void Analytics::Init(const char *trackingID) {
    NSString *nsTrackingID = [[NSString alloc] initWithBytes:trackingID length:strlen(trackingID) encoding:NSUTF8StringEncoding];

	GAI *gai = [GAI sharedInstance];

	// Optional: automatically report uncaught exceptions.
	gai.trackUncaughtExceptions = YES;

	// Optional: set Logger to VERBOSE for debug information.
#ifdef _DEBUG
	gai.logger.logLevel = kGAILogLevelVerbose;
	//gai.dispatchInterval = 20;
#endif

	id<GAITracker> tracker = [gai trackerWithTrackingId:nsTrackingID];
	//[tracker set:kGAIScreenName value:@"BlueshiftGame"];
	//[tracker send:[[GAIDictionaryBuilder createScreenView] build]];
}

void Analytics::Log(const char *category, const char *action, const char *label, long value) {
	NSString *nsCategory = [[NSString alloc] initWithBytes:category length:strlen(category) encoding:NSUTF8StringEncoding];
	NSString *nsAction = [[NSString alloc] initWithBytes:action length:strlen(action) encoding:NSUTF8StringEncoding];
	NSString *nsLabel = [[NSString alloc] initWithBytes:label length:strlen(label) encoding:NSUTF8StringEncoding];
	NSNumber *nsValue = [NSNumber numberWithLong:value];

	id<GAITracker> tracker = [GAI sharedInstance].defaultTracker;

	[tracker send:[[GAIDictionaryBuilder createEventWithCategory:nsCategory
                                                      	  action:nsAction
                                                           label:nsLabel
                                                           value:nsValue] build]];
}