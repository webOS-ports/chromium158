// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/app_runtime/browser/webos_service_bridge_provider.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "content/public/browser/render_frame_host.h"
#include "neva/app_runtime/browser/webos_api_access.h"
#include "neva/pal_service/system_servicebridge.h"

namespace neva_app_runtime {

// static
void WebOSServiceBridgeProvider::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<pal::mojom::SystemServiceBridgeProvider> receiver) {
  CHECK(render_frame_host);
  WebOSServiceBridgeProvider* provider =
      GetOrCreateForCurrentDocument(render_frame_host);
  provider->providers_.Add(provider, std::move(receiver));
}

WebOSServiceBridgeProvider::WebOSServiceBridgeProvider(
    content::RenderFrameHost* render_frame_host)
    : DocumentUserData<WebOSServiceBridgeProvider>(render_frame_host) {}

WebOSServiceBridgeProvider::~WebOSServiceBridgeProvider() = default;

void WebOSServiceBridgeProvider::GetSystemServiceBridge(
    mojo::PendingReceiver<pal::mojom::SystemServiceBridge> receiver) {
  const std::optional<std::string> identity =
      WebOSApiAccess::BusIdentity(&render_frame_host());
  if (!identity) {
    // Dropping the receiver closes the pipe: the bridge never connects, and
    // its calls fail in the page as they do for a bus that is not there.
    LOG(WARNING)
        << "No service bridge for a document of "
        << render_frame_host().GetLastCommittedOrigin().GetDebugString();
    return;
  }
  std::unique_ptr<pal::SystemServiceBridgeImpl> bridge =
      identity->empty()
          ? std::make_unique<pal::SystemServiceBridgeImpl>()
          : std::make_unique<pal::SystemServiceBridgeImpl>(*identity);
  bridges_.Add(std::move(bridge), std::move(receiver));
}

DOCUMENT_USER_DATA_KEY_IMPL(WebOSServiceBridgeProvider);

}  // namespace neva_app_runtime
