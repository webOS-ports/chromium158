// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_APP_RUNTIME_BROWSER_WEBOS_SERVICE_BRIDGE_PROVIDER_H_
#define NEVA_APP_RUNTIME_BROWSER_WEBOS_SERVICE_BRIDGE_PROVIDER_H_

#include "content/public/browser/document_user_data.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "neva/pal_service/public/mojom/system_servicebridge.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace neva_app_runtime {

// WebOSServiceBridge and PalmServiceBridge for one document. It was bound for
// the whole renderer process, and the renderer said which application to
// call the bus as. Now:
//  - a bridge is handed out only to a document that gets the webOS APIs
//    (WebOSApiAccess::IsAllowed), checked when the bridge is asked for;
//  - it calls the bus as the application the embedder says the web view is
//    (WebOSApiAccess::BusIdentity), whatever the renderer asks for;
//  - the provider and every bridge it handed out belong to the document and
//    go with it, so a page the frame shows next cannot use the ones the
//    previous page opened.
// The renderer drops its provider pipe as soon as it has a bridge, so the
// bridges must not be owned by that pipe: both live here, per document.
class WebOSServiceBridgeProvider
    : public content::DocumentUserData<WebOSServiceBridgeProvider>,
      public pal::mojom::SystemServiceBridgeProvider {
 public:
  // For the frame binder map (RegisterBrowserInterfaceBindersForFrame).
  static void Create(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<pal::mojom::SystemServiceBridgeProvider> receiver);

  WebOSServiceBridgeProvider(const WebOSServiceBridgeProvider&) = delete;
  WebOSServiceBridgeProvider& operator=(const WebOSServiceBridgeProvider&) =
      delete;
  ~WebOSServiceBridgeProvider() override;

  // pal::mojom::SystemServiceBridgeProvider
  void GetSystemServiceBridge(
      mojo::PendingReceiver<pal::mojom::SystemServiceBridge> receiver) override;

 private:
  friend content::DocumentUserData<WebOSServiceBridgeProvider>;
  explicit WebOSServiceBridgeProvider(
      content::RenderFrameHost* render_frame_host);

  mojo::ReceiverSet<pal::mojom::SystemServiceBridgeProvider> providers_;
  mojo::UniqueReceiverSet<pal::mojom::SystemServiceBridge> bridges_;

  DOCUMENT_USER_DATA_KEY_DECL();
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_WEBOS_SERVICE_BRIDGE_PROVIDER_H_
