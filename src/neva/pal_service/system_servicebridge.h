// Copyright 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_
#define NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_

#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "neva/pal_service/public/mojom/system_servicebridge.mojom.h"
#include "neva/pal_service/public/system_servicebridge_delegate.h"

namespace pal {

class SystemServiceBridgeDelegate;

class COMPONENT_EXPORT(PAL_SERVICE) SystemServiceBridgeImpl
    : public mojom::SystemServiceBridge {
 public:
  SystemServiceBridgeImpl();
  // Connects on the bus as `appid`, whatever Connect() is asked for: the
  // ConnectionParams come from the renderer, and a document must not choose
  // which application, or which service name, it calls the bus as.
  explicit SystemServiceBridgeImpl(std::string appid);
  SystemServiceBridgeImpl(const SystemServiceBridgeImpl&) = delete;
  SystemServiceBridgeImpl& operator=(const SystemServiceBridgeImpl&) = delete;
  ~SystemServiceBridgeImpl() override;

  // mojom::SystemServiceBridge
  void Connect(mojom::ConnectionParamsPtr params,
               ConnectCallback callback) override;

  // What the delegate is created with for `params`: those, or, with
  // `forced_appid`, that application id and the service name that follows
  // from it, whatever `params` asks for.
  static SystemServiceBridgeDelegate::CreationParams ResolveConnectionParams(
      const mojom::ConnectionParams& params,
      const std::optional<std::string>& forced_appid);
  void Call(const std::string& uri, const std::string& payload) override;
  void Cancel() override;

 private:
  void OnResponse(mojom::ResponseStatus status,
                  const std::string& payload);

  const std::optional<std::string> forced_appid_;
  std::unique_ptr<SystemServiceBridgeDelegate> delegate_;
  mojo::AssociatedRemote<mojom::SystemServiceBridgeClient> remote_client_;
  base::WeakPtrFactory<SystemServiceBridgeImpl> weak_factory_;
};

class SystemServiceBridgeProviderImpl : public mojom::SystemServiceBridgeProvider {
 public:
  SystemServiceBridgeProviderImpl();
  SystemServiceBridgeProviderImpl(const SystemServiceBridgeProviderImpl&) =
      delete;
  SystemServiceBridgeProviderImpl& operator=(
      const SystemServiceBridgeProviderImpl&) = delete;
  ~SystemServiceBridgeProviderImpl() override;

  void AddBinding(mojo::PendingReceiver<mojom::SystemServiceBridgeProvider> receiver);

  // mojom::SystemServiceBridgeProvider
  void GetSystemServiceBridge(
      mojo::PendingReceiver<mojom::SystemServiceBridge> receiver) override;

 private:
  mojo::ReceiverSet<mojom::SystemServiceBridgeProvider> receivers_;
  mojo::UniqueReceiverSet<mojom::SystemServiceBridge> bridges_receivers_;
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_
