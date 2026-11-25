// Copyright 2026 Herman van Hazendonk <github.com@herrie.org>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/app_runtime/common/webos_api_policy.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace neva_app_runtime {
namespace {

constexpr char kAppDir[] = "/usr/palm/applications/com.palm.app.accounts";
constexpr char kAppPage[] =
    "file:///usr/palm/applications/com.palm.app.accounts/index.html";
constexpr char kErrorDir[] = "/usr/share/localization/wam";

WebOSApiPolicy AppPolicy() {
  WebOSApiPolicy policy;
  policy.Restrict();
  EXPECT_TRUE(policy.AddAllowedDirectory(kAppDir));
  return policy;
}

// A frame as the policy sees it: what it shows, and who created it.
struct FakeFrame {
  GURL url;
  url::Origin origin;
  const FakeFrame* creator = nullptr;
};

FakeFrame Page(const std::string& url, const FakeFrame* creator = nullptr) {
  const GURL gurl(url);
  return {gurl, url::Origin::Create(gurl), creator};
}

// about:blank made by a document with `origin`, in a frame `creator` made.
FakeFrame Blank(const url::Origin& origin, const FakeFrame* creator) {
  return {GURL(url::kAboutBlankURL), origin, creator};
}

bool Allows(const WebOSApiPolicy& policy, const FakeFrame& frame) {
  return policy.AllowsFrame(
      &frame,
      [](const FakeFrame* f) {
        return WebOSApiPolicy::Document{f->url, f->origin};
      },
      [](const FakeFrame* f) { return f->creator; });
}

url::Origin OriginOf(const std::string& url) {
  return url::Origin::Create(GURL(url));
}

TEST(WebOSApiPolicyTest, UnrestrictedAllowsEverything) {
  const WebOSApiPolicy policy;
  EXPECT_FALSE(policy.restricted());
  EXPECT_TRUE(policy.AllowsUrl(GURL("https://accounts.google.com/")));
  EXPECT_TRUE(policy.AllowsUrl(GURL("file:///etc/passwd")));
  EXPECT_TRUE(policy.AllowsUrl(GURL("data:text/html,x")));
  const FakeFrame google = Page("https://accounts.google.com/");
  EXPECT_TRUE(Allows(policy, google));
}

TEST(WebOSApiPolicyTest, RestrictedWithNothingAllowedAllowsNothing) {
  WebOSApiPolicy policy;
  policy.Restrict();
  EXPECT_FALSE(policy.AllowsUrl(GURL(kAppPage)));
  EXPECT_FALSE(policy.AllowsUrl(GURL("https://example.com/")));
}

TEST(WebOSApiPolicyTest, AllowsTheApplicationsOwnFiles) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_TRUE(policy.AllowsUrl(GURL(kAppPage)));
  EXPECT_TRUE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/a/b/c.html")));
  EXPECT_TRUE(policy.AllowsUrl(GURL(std::string(kAppPage) + "?x=1#top")));
  EXPECT_TRUE(policy.AllowsUrl(GURL(
      "file://localhost/usr/palm/applications/com.palm.app.accounts/i.html")));
}

TEST(WebOSApiPolicyTest, RefusesRemotePages) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(GURL("https://accounts.google.com/signin")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("http://localhost/rc?code=1")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("data:text/html,<script>1</script>")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("blob:https://example.com/uuid")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("javascript:1")));
  EXPECT_FALSE(policy.AllowsUrl(GURL()));
  EXPECT_FALSE(policy.AllowsUrl(GURL("not a url")));
}

TEST(WebOSApiPolicyTest, RefusesOtherFiles) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(GURL("file:///etc/passwd")));
  EXPECT_FALSE(policy.AllowsUrl(
      GURL("file:///usr/palm/applications/com.palm.app.other/index.html")));
  // The folder itself is no document of the application.
  EXPECT_FALSE(policy.AllowsUrl(
      GURL("file:///usr/palm/applications/com.palm.app.accounts")));
  EXPECT_FALSE(policy.AllowsUrl(
      GURL("file:///usr/palm/applications/com.palm.app.accounts/")));
}

TEST(WebOSApiPolicyTest, AFolderNamedLikeTheApplicationIsNotIt) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accountsx/index.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts.evil/i.html")));
}

TEST(WebOSApiPolicyTest, DotDotCannotLeaveTheFolder) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/../other/i.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/%2e%2e/o/i.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/%2E%2E/o/i.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/.%2e/o/i.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/..%2fo/i.html")));
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts%2f..%2fo/i.html")));
  // Inside the folder, ".." that stays inside is fine.
  EXPECT_TRUE(policy.AllowsUrl(GURL(
      "file:///usr/palm/applications/com.palm.app.accounts/a/../i.html")));
}

TEST(WebOSApiPolicyTest, FileUrlsOnAnotherHostAreRefused) {
  const WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(GURL(
      "file://server/usr/palm/applications/com.palm.app.accounts/i.html")));
}

TEST(WebOSApiPolicyTest, AllowedDirectoriesAreNormalised) {
  WebOSApiPolicy policy;
  policy.Restrict();
  EXPECT_TRUE(policy.AddAllowedDirectory("/a/app/"));
  EXPECT_FALSE(policy.AddAllowedDirectory("/a/app"));  // already there
  EXPECT_TRUE(policy.AddAllowedDirectory("file:///b/err"));
  EXPECT_TRUE(policy.AddAllowedDirectory("///c/x"));
  EXPECT_FALSE(policy.AddAllowedDirectory(""));
  EXPECT_FALSE(policy.AddAllowedDirectory("relative/dir"));
  EXPECT_FALSE(policy.AddAllowedDirectory("/a/../etc"));
  EXPECT_FALSE(policy.AddAllowedDirectory("/"));
  EXPECT_FALSE(policy.AddAllowedDirectory("file:///"));
  ASSERT_EQ(3u, policy.allowed_directories().size());
  EXPECT_TRUE(policy.AllowsUrl(GURL("file:///a/app/i.html")));
  EXPECT_TRUE(policy.AllowsUrl(GURL("file:///b/err/loaderror.html")));
  EXPECT_TRUE(policy.AllowsUrl(GURL("file:///c/x/i.html")));
}

TEST(WebOSApiPolicyTest, ExtraDirectoriesSuchAsTheErrorPage) {
  WebOSApiPolicy policy = AppPolicy();
  EXPECT_FALSE(policy.AllowsUrl(
      GURL("file:///usr/share/localization/wam/loaderror.html")));
  EXPECT_TRUE(policy.AddAllowedDirectory(kErrorDir));
  EXPECT_TRUE(policy.AllowsUrl(
      GURL("file:///usr/share/localization/wam/loaderror.html?errorCode=1")));
  EXPECT_TRUE(policy.AllowsUrl(GURL(kAppPage)));
}

TEST(WebOSApiPolicyTest, AllowedOrigins) {
  WebOSApiPolicy policy = AppPolicy();
  EXPECT_TRUE(policy.AddAllowedOrigin(OriginOf("https://app.example.com/")));
  EXPECT_FALSE(policy.AddAllowedOrigin(OriginOf("https://app.example.com/x")));
  EXPECT_FALSE(policy.AddAllowedOrigin(url::Origin()));  // opaque
  EXPECT_FALSE(policy.AddAllowedOrigin(OriginOf("file:///")));
  EXPECT_TRUE(policy.AllowsUrl(GURL("https://app.example.com/a/b?c")));
  // Scheme, host and port all count.
  EXPECT_FALSE(policy.AllowsUrl(GURL("http://app.example.com/")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("https://app.example.com:8443/")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("https://evil.app.example.com/")));
  EXPECT_FALSE(policy.AllowsUrl(GURL("https://app.example.com.evil/")));
  EXPECT_FALSE(policy.AllowsOrigin(url::Origin()));
  EXPECT_FALSE(policy.AllowsOrigin(OriginOf(kAppPage)));
}

TEST(WebOSApiPolicyTest, InheritsOrigin) {
  EXPECT_TRUE(WebOSApiPolicy::InheritsOrigin(GURL()));
  EXPECT_TRUE(WebOSApiPolicy::InheritsOrigin(GURL("about:blank")));
  EXPECT_TRUE(WebOSApiPolicy::InheritsOrigin(GURL("about:blank#x")));
  EXPECT_TRUE(WebOSApiPolicy::InheritsOrigin(GURL("about:srcdoc")));
  EXPECT_FALSE(WebOSApiPolicy::InheritsOrigin(GURL(kAppPage)));
  EXPECT_FALSE(WebOSApiPolicy::InheritsOrigin(GURL("about:version")));
  EXPECT_FALSE(WebOSApiPolicy::InheritsOrigin(GURL("data:text/html,x")));
}

TEST(WebOSApiPolicyTest, FramesOfTheApplicationAndOfOthers) {
  const WebOSApiPolicy policy = AppPolicy();
  const FakeFrame app = Page(kAppPage);
  const FakeFrame app_child =
      Page("file:///usr/palm/applications/com.palm.app.accounts/f.html", &app);
  const FakeFrame google = Page("https://accounts.google.com/", &app);
  const FakeFrame google_child = Page("https://apis.google.com/x", &google);
  EXPECT_TRUE(Allows(policy, app));
  EXPECT_TRUE(Allows(policy, app_child));
  // An iframe of the application, or a window it opened, on another site.
  EXPECT_FALSE(Allows(policy, google));
  EXPECT_FALSE(Allows(policy, google_child));
}

TEST(WebOSApiPolicyTest, BlankDocumentsOfTheApplication) {
  const WebOSApiPolicy policy = AppPolicy();
  const FakeFrame app = Page(kAppPage);
  // An iframe or a window.open() window the application writes into; its
  // origin is the application's file: origin, which says nothing about the
  // path, so the frame that made it decides.
  const FakeFrame blank = Blank(app.origin, &app);
  EXPECT_TRUE(Allows(policy, blank));
  const FakeFrame srcdoc{GURL("about:srcdoc"), app.origin, &app};
  EXPECT_TRUE(Allows(policy, srcdoc));
  const FakeFrame nested = Blank(app.origin, &blank);
  EXPECT_TRUE(Allows(policy, nested));
  // The initial empty document of a new frame.
  const FakeFrame initial{GURL(), url::Origin(), &app};
  EXPECT_TRUE(Allows(policy, initial));
}

TEST(WebOSApiPolicyTest, BlankDocumentsOfOthers) {
  const WebOSApiPolicy policy = AppPolicy();
  const FakeFrame app = Page(kAppPage);
  const FakeFrame google = Page("https://accounts.google.com/", &app);
  // An about:blank iframe of Google's page.
  const FakeFrame blank = Blank(google.origin, &google);
  EXPECT_FALSE(Allows(policy, blank));
  // A frame of the application that Google's page navigated to about:blank:
  // it runs with Google's origin, and Google's page can script it.
  const FakeFrame hijacked = Blank(google.origin, &app);
  EXPECT_FALSE(Allows(policy, hijacked));
  // Sandboxed: opaque, made from Google's origin.
  const FakeFrame sandboxed{GURL(url::kAboutBlankURL),
                            google.origin.DeriveNewOpaqueOrigin(), &app};
  EXPECT_FALSE(Allows(policy, sandboxed));
  // A file: origin made by a file outside the application.
  const FakeFrame other_file = Page("file:///tmp/x.html", &app);
  const FakeFrame other_blank = Blank(other_file.origin, &other_file);
  EXPECT_FALSE(Allows(policy, other_blank));
}

TEST(WebOSApiPolicyTest, BlankDocumentsOfAnAllowedOrigin) {
  WebOSApiPolicy policy = AppPolicy();
  ASSERT_TRUE(policy.AddAllowedOrigin(OriginOf("https://app.example.com/")));
  const FakeFrame app = Page("https://app.example.com/index.html");
  const FakeFrame blank = Blank(app.origin, &app);
  EXPECT_TRUE(Allows(policy, blank));
  // Its own origin decides, not whoever holds the frame.
  const FakeFrame google = Page("https://accounts.google.com/");
  const FakeFrame inside_google = Blank(app.origin, &google);
  EXPECT_TRUE(Allows(policy, inside_google));
}

TEST(WebOSApiPolicyTest, ADocumentWithNoCreatorIsRefused) {
  const WebOSApiPolicy policy = AppPolicy();
  // A blank window whose opener is gone or in another process.
  const FakeFrame orphan{GURL(url::kAboutBlankURL), url::Origin(), nullptr};
  EXPECT_FALSE(Allows(policy, orphan));
  const FakeFrame* none = nullptr;
  EXPECT_FALSE(policy.AllowsFrame(
      none,
      [](const FakeFrame* f) {
        return WebOSApiPolicy::Document{f->url, f->origin};
      },
      [](const FakeFrame* f) { return f->creator; }));
}

TEST(WebOSApiPolicyTest, ACycleOfBlankFramesEnds) {
  const WebOSApiPolicy policy = AppPolicy();
  FakeFrame a{GURL(url::kAboutBlankURL), url::Origin(), nullptr};
  const FakeFrame b{GURL(url::kAboutBlankURL), url::Origin(), &a};
  a.creator = &b;  // two windows that opened each other
  EXPECT_FALSE(Allows(policy, a));
}

TEST(WebOSApiPolicyTest, Equality) {
  EXPECT_EQ(AppPolicy(), AppPolicy());
  EXPECT_NE(WebOSApiPolicy(), AppPolicy());
  WebOSApiPolicy more = AppPolicy();
  more.AddAllowedDirectory(kErrorDir);
  EXPECT_NE(AppPolicy(), more);
}

}  // namespace
}  // namespace neva_app_runtime
