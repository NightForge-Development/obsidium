// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_BLINK_TEST_ENVIRONMENT_H_
#define CONTENT_PUBLIC_TEST_BLINK_TEST_ENVIRONMENT_H_

#include "v8/include/v8-forward.h"

// This package provides functions used by blink_unittests.
namespace content {

// Initializes Blink test environment for unit tests.
void SetUpBlinkTestEnvironment();

// Terminates Blink test environment for unit tests.
void TearDownBlinkTestEnvironment();

// Returns the Isolate associated with the test environment.
v8::Isolate* GetMainThreadIsolateForTestEnvironment();

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_BLINK_TEST_ENVIRONMENT_H_
