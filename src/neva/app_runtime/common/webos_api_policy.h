// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_APP_RUNTIME_COMMON_WEBOS_API_POLICY_H_
#define NEVA_APP_RUNTIME_COMMON_WEBOS_API_POLICY_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace neva_app_runtime {

// Decides which documents in a web view may use the webOS APIs: the
// injections (webOSSystem, PalmSystem, WebOSServiceBridge, PalmServiceBridge
// and the others a web view requests), the service bus as the application,
// and the browser-side functions behind them.
//
// Every document of an application's web view used to get them, whatever it
// showed, so a sign-in provider's page, a remote page the application
// navigated to, or a third-party iframe called the bus as the application.
// A restricted policy allows only the application's own content:
//  - file: URLs inside one of the allowed directories (the application's
//    folder, and whatever else the embedder adds, such as its error page's
//    folder), and
//  - documents whose origin is one of the allowed origins.
//
// A document that takes its origin from another one (about:blank,
// about:srcdoc, the initial empty document) is decided by that origin. When
// the origin is a file: origin, which says nothing about the path, or is not
// known, the document is decided by the frame that created it: its parent,
// or for a main frame its opener.
//
// A default-constructed policy is unrestricted, which is what every web view
// had before. An embedder opts in with Restrict().
class WebOSApiPolicy {
 public:
  // How deep AllowsFrame() follows parents and openers before it gives up.
  static constexpr int kMaxCreatorDepth = 64;

  // What a frame shows: the URL of its document and the origin it runs with.
  struct Document {
    GURL url;
    url::Origin origin;
  };

  WebOSApiPolicy();
  WebOSApiPolicy(const WebOSApiPolicy&);
  WebOSApiPolicy(WebOSApiPolicy&&);
  WebOSApiPolicy& operator=(const WebOSApiPolicy&);
  WebOSApiPolicy& operator=(WebOSApiPolicy&&);
  ~WebOSApiPolicy();

  bool operator==(const WebOSApiPolicy& other) const;

  // From now on only the allowed directories and origins pass.
  void Restrict() { restricted_ = true; }
  bool restricted() const { return restricted_; }

  // `directory` is an absolute path; a trailing separator is optional, and so
  // is a "file://" prefix. Relative paths, paths with ".." components and
  // empty strings are ignored. Returns whether it was added.
  bool AddAllowedDirectory(const std::string& directory);
  // Opaque origins are ignored. Returns whether it was added.
  bool AddAllowedOrigin(const url::Origin& origin);

  const std::vector<base::FilePath>& allowed_directories() const {
    return allowed_directories_;
  }
  const std::vector<url::Origin>& allowed_origins() const {
    return allowed_origins_;
  }

  // Whether a document with `url` gets the APIs, for a URL that brings its
  // own origin (not about:blank or about:srcdoc, see InheritsOrigin()).
  bool AllowsUrl(const GURL& url) const;

  // Whether `origin` is one of the allowed origins. A file: origin never is:
  // file: documents are decided by their path.
  bool AllowsOrigin(const url::Origin& origin) const;

  // Whether a document at `url` runs with an origin it inherits from another
  // document instead of one of its own.
  static bool InheritsOrigin(const GURL& url);

  // Whether the document shown in `frame` (a pointer to whatever type of frame
  // the caller has; null means none) gets the APIs. `document_of(frame)`
  // returns the frame's WebOSApiPolicy::Document; `creator_of(frame)` returns
  // the frame that created it (its parent, or for a main frame its opener),
  // or null when there is none or it is not reachable (for example in
  // another process).
  template <typename FramePtr, typename DocumentOf, typename CreatorOf>
  bool AllowsFrame(FramePtr frame,
                   DocumentOf document_of,
                   CreatorOf creator_of) const {
    if (!restricted_) {
      return true;
    }
    for (int depth = 0; frame && depth < kMaxCreatorDepth; ++depth) {
      const Document document = document_of(frame);
      if (!InheritsOrigin(document.url)) {
        return AllowsUrl(document.url);
      }
      const url::SchemeHostPort& tuple =
          document.origin.GetTupleOrPrecursorTupleIfOpaque();
      if (tuple.IsValid() && tuple.scheme() != url::kFileScheme) {
        // The origin, or for an opaque one the origin it was derived from,
        // says who made the document: decide by it, not by the creator. A
        // third-party frame can navigate a frame of the application to
        // about:blank and then script it.
        return AllowsOrigin(url::Origin::Create(tuple.GetURL()));
      }
      frame = creator_of(frame);
    }
    return false;
  }

 private:
  bool restricted_ = false;
  std::vector<base::FilePath> allowed_directories_;
  std::vector<url::Origin> allowed_origins_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_COMMON_WEBOS_API_POLICY_H_
