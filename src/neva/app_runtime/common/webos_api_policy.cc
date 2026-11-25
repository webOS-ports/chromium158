// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/app_runtime/common/webos_api_policy.h"

#include <algorithm>
#include <string_view>

#include "base/strings/string_util.h"
#include "net/base/filename_util.h"
#include "url/url_constants.h"

namespace neva_app_runtime {

WebOSApiPolicy::WebOSApiPolicy() = default;
WebOSApiPolicy::WebOSApiPolicy(const WebOSApiPolicy&) = default;
WebOSApiPolicy::WebOSApiPolicy(WebOSApiPolicy&&) = default;
WebOSApiPolicy& WebOSApiPolicy::operator=(const WebOSApiPolicy&) = default;
WebOSApiPolicy& WebOSApiPolicy::operator=(WebOSApiPolicy&&) = default;
WebOSApiPolicy::~WebOSApiPolicy() = default;

bool WebOSApiPolicy::operator==(const WebOSApiPolicy& other) const {
  return restricted_ == other.restricted_ &&
         allowed_directories_ == other.allowed_directories_ &&
         allowed_origins_ == other.allowed_origins_;
}

bool WebOSApiPolicy::AddAllowedDirectory(const std::string& directory) {
  std::string_view path(directory);
  constexpr std::string_view kFilePrefix = "file://";
  if (base::StartsWith(path, kFilePrefix)) {
    path.remove_prefix(kFilePrefix.size());
  }
  // WebOSFileAccessController accepts app paths with extra leading slashes.
  while (base::StartsWith(path, "//")) {
    path.remove_prefix(1);
  }
  const base::FilePath dir =
      base::FilePath(path).StripTrailingSeparators();
  if (dir.empty() || !dir.IsAbsolute() || dir.ReferencesParent() ||
      dir == base::FilePath(FILE_PATH_LITERAL("/"))) {
    return false;
  }
  if (std::find(allowed_directories_.begin(), allowed_directories_.end(),
                dir) != allowed_directories_.end()) {
    return false;
  }
  allowed_directories_.push_back(dir);
  return true;
}

bool WebOSApiPolicy::AddAllowedOrigin(const url::Origin& origin) {
  if (origin.opaque() || origin.scheme() == url::kFileScheme) {
    return false;
  }
  if (std::find(allowed_origins_.begin(), allowed_origins_.end(), origin) !=
      allowed_origins_.end()) {
    return false;
  }
  allowed_origins_.push_back(origin);
  return true;
}

bool WebOSApiPolicy::AllowsUrl(const GURL& url) const {
  if (!restricted_) {
    return true;
  }
  if (!url.is_valid()) {
    return false;
  }
  if (url.SchemeIsFile()) {
    // file://host/ is a network share, never part of an application.
    if (url.has_host() && url.host() != "localhost") {
      return false;
    }
    // GURL has already resolved "." and ".." segments, percent-encoded ones
    // included, and FileURLToFilePath() refuses an encoded separator.
    base::FilePath path;
    if (!net::FileURLToFilePath(url, &path) || !path.IsAbsolute() ||
        path.ReferencesParent()) {
      return false;
    }
    return std::any_of(
        allowed_directories_.begin(), allowed_directories_.end(),
        [&path](const base::FilePath& dir) { return dir.IsParent(path); });
  }
  return AllowsOrigin(url::Origin::Create(url));
}

bool WebOSApiPolicy::AllowsOrigin(const url::Origin& origin) const {
  if (!restricted_) {
    return true;
  }
  if (origin.opaque() || origin.scheme() == url::kFileScheme) {
    return false;
  }
  return std::find(allowed_origins_.begin(), allowed_origins_.end(),
                   origin) != allowed_origins_.end();
}

// static
bool WebOSApiPolicy::InheritsOrigin(const GURL& url) {
  return url.is_empty() || url.IsAboutBlank() || url.IsAboutSrcdoc();
}

}  // namespace neva_app_runtime
