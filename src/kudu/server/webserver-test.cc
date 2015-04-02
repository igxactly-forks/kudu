// Copyright (c) 2013, Cloudera, inc.
// Confidential Cloudera Information: Covered by NDA.

#include <gtest/gtest.h>

#include "kudu/gutil/strings/substitute.h"
#include "kudu/server/default-path-handlers.h"
#include "kudu/server/webserver.h"
#include "kudu/util/curl_util.h"
#include "kudu/util/net/sockaddr.h"
#include "kudu/util/test_util.h"

namespace kudu {

class WebserverTest : public KuduTest {
 public:
  WebserverTest() {
    WebserverOptions opts;
    opts.port = 0;
    server_.reset(new Webserver(opts));
  }

  virtual void SetUp() OVERRIDE {
    KuduTest::SetUp();

    AddDefaultPathHandlers(server_.get());
    ASSERT_OK(server_->Start());

    vector<Sockaddr> addrs;
    ASSERT_OK(server_->GetBoundAddresses(&addrs));
    ASSERT_EQ(addrs.size(), 1);
    addr_ = addrs[0];
  }

 protected:
  gscoped_ptr<Webserver> server_;
  Sockaddr addr_;
};

TEST_F(WebserverTest, TestIndexPage) {
  EasyCurl c;
  faststring buf;
  ASSERT_OK(c.FetchURL(strings::Substitute("http://$0/", addr_.ToString()),
                              &buf));
  // Should have expected title.
  ASSERT_STR_CONTAINS(buf.ToString(), "Cloudera Kudu");

  // Should have link to default path handlers (e.g memz)
  ASSERT_STR_CONTAINS(buf.ToString(), "memz");
}

TEST_F(WebserverTest, TestDefaultPaths) {
  EasyCurl c;
  faststring buf;

  // Test memz
  ASSERT_OK(c.FetchURL(strings::Substitute("http://$0/memz?raw=1", addr_.ToString()),
                              &buf));
#ifdef TCMALLOC_ENABLED
  ASSERT_STR_CONTAINS(buf.ToString(), "Bytes in use by application");
#else
  ASSERT_STR_CONTAINS(buf.ToString(), "not available unless tcmalloc is enabled");
#endif

  // Test varz -- check for one of the built-in gflags flags.
  ASSERT_OK(c.FetchURL(strings::Substitute("http://$0/varz?raw=1", addr_.ToString()),
                              &buf));
  ASSERT_STR_CONTAINS(buf.ToString(), "--v=");
}

} // namespace kudu
