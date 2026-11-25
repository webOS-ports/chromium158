// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_APP_RUNTIME_BROWSER_WEBOS_API_ACCESS_H_
#define NEVA_APP_RUNTIME_BROWSER_WEBOS_API_ACCESS_H_

#include <optional>
#include <string>

#include "content/public/browser/web_contents_user_data.h"
#include "neva/app_runtime/common/webos_api_policy.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace neva_app_runtime {

// The WebOSApiPolicy of a WebContents, kept for the browser-side code that
// serves the webOS APIs. That code checks each call against the document
// that makes it, so a renderer that hands an injection to a document it
// should not have reached gains nothing from it.
class WebOSApiAccess : public content::WebContentsUserData<WebOSApiAccess> {
 public:
  WebOSApiAccess(const WebOSApiAccess&) = delete;
  WebOSApiAccess& operator=(const WebOSApiAccess&) = delete;
  ~WebOSApiAccess() override;

  static void SetPolicy(content::WebContents* web_contents,
                        WebOSApiPolicy policy);
  // Unrestricted for a WebContents nobody set a policy for.
  static const WebOSApiPolicy& GetPolicy(content::WebContents* web_contents);

  // Whether the document now shown in `render_frame_host` gets the webOS
  // APIs under the policy of its WebContents.
  static bool IsAllowed(content::RenderFrameHost* render_frame_host);

  // The application the document in `render_frame_host` uses the bus as: what
  // the embedder answers to "identifier" (for WAM the application id, or the
  // error page's while it shows). Null when the document gets no APIs, or the
  // policy is restricted and the embedder has no answer. An empty string
  // means the renderer may name it, which only an unrestricted policy allows.
  static std::optional<std::string> BusIdentity(
      content::RenderFrameHost* render_frame_host);

 private:
  friend class content::WebContentsUserData<WebOSApiAccess>;

  explicit WebOSApiAccess(content::WebContents* web_contents);

  WebOSApiPolicy policy_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_WEBOS_API_ACCESS_H_
