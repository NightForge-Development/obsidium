// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/main/browser_web_state_list_delegate.h"

#import "ios/chrome/browser/tabs/model/tab_helper_util.h"

BrowserWebStateListDelegate::BrowserWebStateListDelegate() = default;

BrowserWebStateListDelegate::~BrowserWebStateListDelegate() = default;

void BrowserWebStateListDelegate::WillAddWebState(web::WebState* web_state) {
  // Unconditionally call AttachTabHelper even for pre-rendered WebState as
  // the method is idempotent and this ensure that any WebState in a
  // WebStateList has all the expected tab helpers.
  AttachTabHelpers(web_state, /*for_prerender=*/false);
}
