// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host_lacros.h"

#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/desktop_browser_frame_lacros.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chromeos/constants/chromeos_features.h"
#include "chromeos/ui/base/window_properties.h"
#include "chromeos/ui/frame/frame_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-shared.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/rrect_f.h"
#include "ui/platform_window/extensions/wayland_extension.h"
#include "ui/platform_window/platform_window.h"

namespace {

// Returns the event source for the active tab drag session.
absl::optional<ui::mojom::DragEventSource> GetCurrentTabDragEventSource() {
  if (auto* source_context = TabDragController::GetSourceContext()) {
    if (auto* drag_controller = source_context->GetDragController()) {
      return drag_controller->event_source();
    }
  }
  return absl::nullopt;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostLacros, public:

BrowserDesktopWindowTreeHostLacros::BrowserDesktopWindowTreeHostLacros(
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura,
    BrowserView* browser_view,
    BrowserFrame* browser_frame)
    : DesktopWindowTreeHostLacros(native_widget_delegate,
                                  desktop_native_widget_aura),
      browser_view_(browser_view),
      desktop_native_widget_aura_(desktop_native_widget_aura) {
  native_frame_ = static_cast<DesktopBrowserFrameLacros*>(
      browser_frame->native_browser_frame());
  native_frame_->set_host(this);

  // Lacros receives occlusion information from exo via aura-shell.
  SetNativeWindowOcclusionEnabled(true);
}

BrowserDesktopWindowTreeHostLacros::~BrowserDesktopWindowTreeHostLacros() {
  native_frame_->set_host(nullptr);
}

void BrowserDesktopWindowTreeHostLacros::UpdateFrameHints() {
  auto* const view = browser_view_->frame()->GetFrameView();
  const bool showing_frame =
      browser_view_->frame()->UseCustomFrame() && !view->IsFrameCondensed();
  const float scale = device_scale_factor();
  const gfx::Size widget_size_px =
      platform_window()->GetBoundsInPixels().size();

  const float corner_radius =
      chromeos::GetFrameCornerRadius(browser_view_->GetNativeWindow());

  std::vector<gfx::Rect> opaque_region;
  const bool should_have_rounded_corners = corner_radius > 0;
  if (showing_frame && should_have_rounded_corners) {
    gfx::RoundedCornersF frame_radii{corner_radius, corner_radius, 0, 0};
    if (chromeos::features::IsRoundedWindowsEnabled()) {
      frame_radii.set_lower_left(corner_radius);
      frame_radii.set_lower_right(corner_radius);
    }

    GetContentWindow()->layer()->SetRoundedCornerRadius(frame_radii);
    GetContentWindow()->layer()->SetIsFastRoundedCorner(true);

    // The opaque region is a list of rectangles that contain only fully
    // opaque pixels of the window.  We need to convert the clipping
    // rounded-rect into this format.
    gfx::Rect local_bounds = view->GetLocalBounds();
    gfx::RRectF rounded_corners_rect(gfx::RectF(local_bounds), frame_radii);
    gfx::RectF rect_f = rounded_corners_rect.rect();
    rect_f.Scale(scale);

    // It is acceptable to omit some pixels that are opaque, but the region
    // must not include any translucent pixels.  Therefore, we must
    // conservatively scale to the enclosed rectangle.
    gfx::Rect rect = gfx::ToEnclosedRect(rect_f);

    // Create the initial region from the clipping rectangle without rounded
    // corners.
    cc::Region region(rect);

    // Now subtract out the small rectangles that cover the corners.
    struct {
      gfx::RRectF::Corner corner;
      bool left;
      bool upper;
    } kCorners[] = {
        {gfx::RRectF::Corner::kUpperLeft, true, true},
        {gfx::RRectF::Corner::kUpperRight, false, true},
        {gfx::RRectF::Corner::kLowerLeft, true, false},
        {gfx::RRectF::Corner::kLowerRight, false, false},
    };
    for (const auto& corner : kCorners) {
      auto corner_radii = rounded_corners_rect.GetCornerRadii(corner.corner);
      auto rx = std::ceil(scale * corner_radii.x());
      auto ry = std::ceil(scale * corner_radii.y());
      auto corner_rect =
          gfx::Rect(corner.left ? rect.x() : rect.right() - rx,
                    corner.upper ? rect.y() : rect.bottom() - ry, rx, ry);
      region.Subtract(corner_rect);
    }

    // Convert the region to a list of rectangles.
    for (gfx::Rect i : region) {
      opaque_region.push_back(i);
    }
  } else {
    GetContentWindow()->layer()->SetRoundedCornerRadius({});
    GetContentWindow()->layer()->SetIsFastRoundedCorner(false);
    opaque_region.push_back({{}, widget_size_px});
  }
  // TODO(crbug.com/1306688): Instead of setting OpaqueRegion, set the rounded
  // corners in dp.
  platform_window()->SetOpaqueRegion(opaque_region);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostLacros,
//     DesktopWindowTreeHost implementation:

void BrowserDesktopWindowTreeHostLacros::OnWidgetInitDone() {
  DesktopWindowTreeHostLacros::OnWidgetInitDone();

  UpdateFrameHints();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostLacros,
//     BrowserDesktopWindowTreeHost implementation:

views::DesktopWindowTreeHost*
BrowserDesktopWindowTreeHostLacros::AsDesktopWindowTreeHost() {
  return this;
}

int BrowserDesktopWindowTreeHostLacros::GetMinimizeButtonOffset() const {
  return 0;
}

bool BrowserDesktopWindowTreeHostLacros::UsesNativeSystemMenu() const {
  return false;
}

void BrowserDesktopWindowTreeHostLacros::TabDraggingKindChanged(
    TabDragKind tab_drag_kind) {
  // If there's no tabs left, the browser window is about to close, so don't
  // call SetOverrideRedirect() to prevent the window from flashing.
  if (!browser_view_->tabstrip()->GetModelCount() ||
      tab_drag_kind == TabDragKind::kNone)
    return;

  auto* wayland_extension = ui::GetWaylandExtension(*platform_window());
  if (auto event_source = GetCurrentTabDragEventSource()) {
    const auto allow_system_drag = base::FeatureList::IsEnabled(
        features::kAllowWindowDragUsingSystemDragDrop);
    wayland_extension->StartWindowDraggingSessionIfNeeded(*event_source,
                                                          allow_system_drag);
  }
}

bool BrowserDesktopWindowTreeHostLacros::SupportsMouseLock() {
  auto* wayland_extension = ui::GetWaylandExtension(*platform_window());
  return wayland_extension->SupportsPointerLock();
}

void BrowserDesktopWindowTreeHostLacros::LockMouse(aura::Window* window) {
  DesktopWindowTreeHostLacros::LockMouse(window);

  if (SupportsMouseLock()) {
    auto* wayland_extension = ui::GetWaylandExtension(*platform_window());
    wayland_extension->LockPointer(true /*enabled*/);
  }
}

void BrowserDesktopWindowTreeHostLacros::UnlockMouse(aura::Window* window) {
  DesktopWindowTreeHostLacros::UnlockMouse(window);

  if (SupportsMouseLock()) {
    auto* wayland_extension = ui::GetWaylandExtension(*platform_window());
    wayland_extension->LockPointer(false /*enabled*/);
  }
}

void BrowserDesktopWindowTreeHostLacros::OnBoundsChanged(
    const BoundsChange& change) {
  DesktopWindowTreeHostLacros::OnBoundsChanged(change);

  UpdateFrameHints();
}

void BrowserDesktopWindowTreeHostLacros::OnWindowStateChanged(
    ui::PlatformWindowState old_window_show_state,
    ui::PlatformWindowState new_window_show_state) {
  DesktopWindowTreeHostLacros::OnWindowStateChanged(old_window_show_state,
                                                    new_window_show_state);

  bool fullscreen_changed =
      new_window_show_state == ui::PlatformWindowState::kFullScreen ||
      old_window_show_state == ui::PlatformWindowState::kFullScreen;
  if (old_window_show_state != new_window_show_state && fullscreen_changed) {
    // If the browser view initiated this state change,
    // BrowserView::ProcessFullscreen will no-op, so this call is harmless.
    browser_view_->FullscreenStateChanging();
  }

  UpdateFrameHints();
}

void BrowserDesktopWindowTreeHostLacros::OnImmersiveModeChanged(bool enabled) {
  DesktopWindowTreeHostLacros::OnImmersiveModeChanged(enabled);
  // Update the browser UI, because some fullscreen mode UI updates depend on
  // immersive mode state. Unlike ash-chrome, Lacros's immersive mode is set
  // to the system asynchronously.
  browser_view_->browser()->FullscreenTopUIStateChanged();
}

void BrowserDesktopWindowTreeHostLacros::OnFullscreenModeChanged() {
  // Finalizing full screen mode transition after Ash has also asynchronously
  // entered the full screen mode state for this window.
  browser_view_->FullscreenStateChanged();
}

void BrowserDesktopWindowTreeHostLacros::OnOverviewModeChanged(
    bool in_overview) {
  DesktopWindowTreeHostLacros::OnOverviewModeChanged(in_overview);

  // Window corner radius depends on weather the window is in overview mode or
  // not. Once the overview property has been updated, the browser window
  // corners needs to be updated.
  // See `chromeos::GetFrameCornerRadius()` for more details.
  // TODO(b/301501363): Rename to UpdateWindowHints.
  UpdateFrameHints();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostLacros,
//     DesktopWindowTreeHostPlatform implementation:

SkPath BrowserDesktopWindowTreeHostLacros::GetWindowMaskForClipping() const {
  // Lacros doesn't need to request clipping since it is already
  // done in views, so returns empty window mask.
  return SkPath();
}

void BrowserDesktopWindowTreeHostLacros::OnSurfaceFrameLockingChanged(
    bool lock) {
  aura::Window* window = desktop_native_widget_aura_->GetNativeWindow();
  DCHECK(window);
  window->SetProperty(chromeos::kFrameRestoreLookKey, lock);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHost, public:

// static
BrowserDesktopWindowTreeHost*
BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura,
    BrowserView* browser_view,
    BrowserFrame* browser_frame) {
  return new BrowserDesktopWindowTreeHostLacros(native_widget_delegate,
                                                desktop_native_widget_aura,
                                                browser_view, browser_frame);
}
