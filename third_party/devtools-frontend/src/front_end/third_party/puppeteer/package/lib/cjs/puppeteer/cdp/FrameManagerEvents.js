"use strict";
/**
 * Copyright 2023 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.FrameManagerEvent = void 0;
/**
 * We use symbols to prevent external parties listening to these events.
 * They are internal to Puppeteer.
 *
 * @internal
 */
// eslint-disable-next-line @typescript-eslint/no-namespace
var FrameManagerEvent;
(function (FrameManagerEvent) {
    FrameManagerEvent.FrameAttached = Symbol('FrameManager.FrameAttached');
    FrameManagerEvent.FrameNavigated = Symbol('FrameManager.FrameNavigated');
    FrameManagerEvent.FrameDetached = Symbol('FrameManager.FrameDetached');
    FrameManagerEvent.FrameSwapped = Symbol('FrameManager.FrameSwapped');
    FrameManagerEvent.LifecycleEvent = Symbol('FrameManager.LifecycleEvent');
    FrameManagerEvent.FrameNavigatedWithinDocument = Symbol('FrameManager.FrameNavigatedWithinDocument');
})(FrameManagerEvent || (exports.FrameManagerEvent = FrameManagerEvent = {}));
//# sourceMappingURL=FrameManagerEvents.js.map