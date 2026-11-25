// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/pal_service/system_servicebridge.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace pal {
namespace {

mojom::ConnectionParams Params(std::optional<std::string> name,
                               std::optional<std::string> appid,
                               int suffix) {
  mojom::ConnectionParams params;
  params.name = std::move(name);
  params.appid = std::move(appid);
  params.suffix = suffix;
  return params;
}

TEST(SystemServiceBridgeTest, WithoutAForcedIdTheParamsAreUsed) {
  const auto resolved = SystemServiceBridgeImpl::ResolveConnectionParams(
      Params("com.webos.chromium.service_worker", std::nullopt, 3),
      std::nullopt);
  EXPECT_EQ("com.webos.chromium.service_worker", resolved.name);
  EXPECT_EQ("", resolved.appid);
  EXPECT_EQ(3, resolved.suffix);

  const auto from_page = SystemServiceBridgeImpl::ResolveConnectionParams(
      Params(std::nullopt, "com.palm.app.accounts", -1), std::nullopt);
  EXPECT_EQ("", from_page.name);
  EXPECT_EQ("com.palm.app.accounts", from_page.appid);
  EXPECT_EQ(-1, from_page.suffix);
}

TEST(SystemServiceBridgeTest, AForcedIdOverridesWhatThePageAsksFor) {
  // A page that claims another application, or a service name of its own.
  const auto resolved = SystemServiceBridgeImpl::ResolveConnectionParams(
      Params("com.palm.service.accounts", "com.palm.app.settings", 7),
      std::string("com.palm.app.accounts"));
  EXPECT_EQ("", resolved.name);
  EXPECT_EQ("com.palm.app.accounts", resolved.appid);
  EXPECT_EQ(-1, resolved.suffix);
}

TEST(SystemServiceBridgeTest, AForcedIdAppliesWhenThePageSendsNothing) {
  const auto resolved = SystemServiceBridgeImpl::ResolveConnectionParams(
      Params(std::nullopt, std::nullopt, -1),
      std::string("com.webos.settingsservice.client"));
  EXPECT_EQ("", resolved.name);
  EXPECT_EQ("com.webos.settingsservice.client", resolved.appid);
  EXPECT_EQ(-1, resolved.suffix);
}

}  // namespace
}  // namespace pal
