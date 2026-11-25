// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/app_runtime/browser/webos_api_access.h"

#include "base/no_destructor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "neva/app_runtime/browser/app_runtime_webview_controller_impl.h"
#include "neva/app_runtime/public/webview_controller_delegate.h"

namespace neva_app_runtime {

namespace {

// What webOSSystem.identifier, and so WebOSServiceBridge, has always asked
// the embedder for.
constexpr char kIdentifierFunction[] = "identifier";

WebOSApiPolicy::Document DocumentOf(content::RenderFrameHost* frame) {
  return {frame->GetLastCommittedURL(), frame->GetLastCommittedOrigin()};
}

// The frame whose document made the one in `frame`, for a document that
// inherits its origin: the parent, or for the main frame the opener (which
// may be in another WebContents, as for a window.open() window).
content::RenderFrameHost* CreatorOf(content::RenderFrameHost* frame) {
  if (content::RenderFrameHost* parent = frame->GetParent()) {
    return parent;
  }
  if (!frame->IsInPrimaryMainFrame()) {
    return nullptr;
  }
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(frame);
  return contents ? contents->GetOpener() : nullptr;
}

}  // namespace

WebOSApiAccess::WebOSApiAccess(content::WebContents* web_contents)
    : content::WebContentsUserData<WebOSApiAccess>(*web_contents) {}

WebOSApiAccess::~WebOSApiAccess() = default;

// static
void WebOSApiAccess::SetPolicy(content::WebContents* web_contents,
                               WebOSApiPolicy policy) {
  CreateForWebContents(web_contents);
  FromWebContents(web_contents)->policy_ = std::move(policy);
}

// static
const WebOSApiPolicy& WebOSApiAccess::GetPolicy(
    content::WebContents* web_contents) {
  static const base::NoDestructor<WebOSApiPolicy> kUnrestricted;
  const WebOSApiAccess* access =
      web_contents ? FromWebContents(web_contents) : nullptr;
  return access ? access->policy_ : *kUnrestricted;
}

// static
bool WebOSApiAccess::IsAllowed(content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host) {
    return false;
  }
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!contents) {
    return false;
  }
  return GetPolicy(contents).AllowsFrame(render_frame_host, &DocumentOf,
                                         &CreatorOf);
}

// static
std::optional<std::string> WebOSApiAccess::BusIdentity(
    content::RenderFrameHost* render_frame_host) {
  if (!IsAllowed(render_frame_host)) {
    return std::nullopt;
  }
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  const bool restricted = GetPolicy(contents).restricted();

  std::string identifier;
  if (auto* controller =
          AppRuntimeWebViewControllerImpl::FromWebContents(contents)) {
    if (WebViewControllerDelegate* delegate = controller->delegate()) {
      identifier = delegate->RunFunction(kIdentifierFunction, {});
    }
  }
  if (identifier.empty() && restricted) {
    return std::nullopt;
  }
  return identifier;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebOSApiAccess);

}  // namespace neva_app_runtime
