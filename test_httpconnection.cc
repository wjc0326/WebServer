/*
 * Copyright ©2023 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

extern "C" {
  #include <pthread.h>  // for the pthread threading/mutex functions
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>

#include "./HttpConnection.h"

#include "gtest/gtest.h"
#include "./HttpRequest.h"
#include "./HttpResponse.h"
#include "./HttpUtils.h"
#include "./test_suite.h"

using std::string;

namespace searchserver {

static pthread_mutex_t rw_lock;
static int num_read = 0;
static int num_write = 0;

static void WritePartialRequests(void* args);

static void *WriteWrapper(void* args) {
  WritePartialRequests(args);
  return nullptr;
}

TEST(Test_HttpConnection, Basic) {
  ProjectEnvironment::OpenTestCase();

  // Create a socketpair; we'll hand one end of the socket to the
  // HttpConnection object, and use the other end of the socket
  // ourselves for testing.
  int spair[2] = {-1, -1};
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, spair));

  // Create the HttpConnection object we'll test.
  HttpConnection hc(spair[0]);

  // Write three requests on the socket.
  string req1 = "GET /foo HTTP/1.1\r\n";
  req1 += "Host: somehost.foo.bar\r\n";
  req1 += "Connection: close\r\n";
  req1 += "\r\n";
  // req2: check header order doesn't matter
  string req2 = "GET /bar HTTP/1.1\r\n";
  req2 += "Connection: close\r\n";
  req2 += "Host: somehost.foo.bar\r\n";
  req2 +=  "\r\n";
  // req3: check different values
  string req3 = "GET /baz HTTP/1.1\r\n";
  req3 += "connection: keep-alive\r\n";
  req3 += "host: somehost.foo.bar\r\n";
  req3 += "OTHER: some_value\r\n";
  req3 +=  "\r\n";
  ASSERT_EQ(static_cast<int>(req1.size()),
            wrapped_write(spair[1], req1));
  ASSERT_EQ(static_cast<int>(req2.size()),
            wrapped_write(spair[1], req2));
  ASSERT_EQ(static_cast<int>(req3.size()),
            wrapped_write(spair[1], req3));

  // Do the next_requests.
  HttpRequest htreq1, htreq2, htreq3;
  ASSERT_TRUE(hc.next_request(&htreq1));
  ASSERT_TRUE(hc.next_request(&htreq2));
  ASSERT_TRUE(hc.next_request(&htreq3));
  ProjectEnvironment::AddPoints(10);

  // Make sure the request parsing worked.
  ASSERT_EQ("/foo", htreq1.uri());
  ASSERT_EQ("somehost.foo.bar", htreq1.GetHeaderValue("host"));
  ASSERT_EQ("close", htreq1.GetHeaderValue("connection"));
  ASSERT_EQ(2, htreq1.GetHeaderCount());
  ProjectEnvironment::AddPoints(5);

  ASSERT_EQ("/bar", htreq2.uri());
  ASSERT_EQ("somehost.foo.bar", htreq2.GetHeaderValue("host"));
  ASSERT_EQ("close", htreq2.GetHeaderValue("connection"));
  ASSERT_EQ(2, htreq2.GetHeaderCount());
  ProjectEnvironment::AddPoints(5);

  ASSERT_EQ("/baz", htreq3.uri());
  ASSERT_EQ("somehost.foo.bar", htreq3.GetHeaderValue("host"));
  ASSERT_EQ("keep-alive", htreq3.GetHeaderValue("connection"));
  ASSERT_EQ("some_value", htreq3.GetHeaderValue("other"));
  ASSERT_EQ(3, htreq3.GetHeaderCount());
  ProjectEnvironment::AddPoints(5);

  // Test a "split" response being written
  string req4 = "GET /foo HTTP/1.1\r\n";
  req1 += "Host: somehost.foo.bar\r\n";
  req1 += "Connection: close\r\n";
  req1 += "\r\n";
  // req2: check header order doesn't matter
  string req5 = "GET /bar HTTP/1.1\r\n";
  req2 += "Connection: close\r\n";
  req2 += "Host: somehost.foo.bar\r\n";
  req2 +=  "\r\n";

  // Prepare the responses.
  HttpResponse rep1;
  rep1.set_protocol("HTTP/1.1");
  rep1.set_response_code(200);
  rep1.set_message("OK");
  rep1.AppendToBody("This is the body of the response.");
  string expectedrep1 = "HTTP/1.1 200 OK\r\n";
  expectedrep1 += "Content-length: 33\r\n\r\n";
  expectedrep1 += "This is the body of the response.";

  HttpResponse rep2;
  rep2.set_protocol("HTTP/1.1");
  rep2.set_response_code(200);
  rep2.set_message("OK");
  rep2.set_content_type("text/html");
  rep2.AppendToBody("This is the second response.");
  string expectedrep2 = "HTTP/1.1 200 OK\r\n";
  expectedrep2 += "Content-type: text/html\r\n";
  expectedrep2 += "Content-length: 28\r\n\r\n";
  expectedrep2 += "This is the second response.";

  // Generate the responses, test them.
  string buf1;
  ASSERT_TRUE(hc.write_response(rep1));
  ASSERT_EQ(72, wrapped_read(spair[1], &buf1));
  ASSERT_EQ(expectedrep1, buf1);

  string buf2;
  ASSERT_TRUE(hc.write_response(rep2));
  ASSERT_EQ(92, wrapped_read(spair[1], &buf2));
  ASSERT_EQ(expectedrep2, buf2);

  // Clean up.
  close(spair[0]);
  close(spair[1]);
  ProjectEnvironment::AddPoints(5);
}

TEST(Test_HttpConnection, PartialRead) {
  ProjectEnvironment::OpenTestCase();

  // Create a socketpair; we'll hand one end of the socket to the
  // HttpConnection object, and use the other end of the socket
  // ourselves for testing.
  int spair[2] = {-1, -1};
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, spair));

  // Note that we use sleep() a lot to try and "motivate"
  // the threads to swap. This is beacuse valgrind isn't
  // the happiest when we have multiple threads so
  // sleep is needed to make it cooperate.

  // Create the HttpConnection object we'll test.
  HttpConnection hc(spair[0]);

  // create a thread to write on the other end of the socket
  // so that we can test our http connection class
  ASSERT_EQ(0, pthread_mutex_init(&rw_lock, nullptr));
  pthread_t thread_id;
  ASSERT_EQ(0, pthread_create(&thread_id, nullptr, WriteWrapper, &(spair[1])));
  // sleep so that the write thread can start writing to us
  sleep(1);

  // Do the next_requests.
  HttpRequest htreq1, htreq2, htreq3;

  // loop until we know that the write socket has started to
  // write the first request.
  while (1) {
    ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
    if (num_write == 1) {
      num_read += 1;
      ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
      ASSERT_TRUE(hc.next_request(&htreq1));
      break;
    }
    ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
    sleep(1);
  }

  // Make sure the request parsing worked.
  ASSERT_EQ("/foo", htreq1.uri());
  ASSERT_EQ("somehost.foo.bar", htreq1.GetHeaderValue("host"));
  ASSERT_EQ("close", htreq1.GetHeaderValue("connection"));
  ASSERT_EQ(2, htreq1.GetHeaderCount());
  ProjectEnvironment::AddPoints(5);
  sleep(1);  // sleep so that req2 can be sent

  // start parsing req2
  ASSERT_TRUE(hc.next_request(&htreq2));
  ASSERT_EQ("/bar", htreq2.uri());
  ASSERT_EQ("somehost.foo.bar", htreq2.GetHeaderValue("host"));
  ASSERT_EQ("close", htreq2.GetHeaderValue("connection"));
  ASSERT_EQ(2, htreq2.GetHeaderCount());
  ProjectEnvironment::AddPoints(5);

  // loop until we know that the write socket has started to
  // write the third request.
  while (1) {
    ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
    if (num_write == 2) {
      num_read += 1;
      ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
      break;
    }
    ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
    sleep(1);
  }

  // verify that req3 is correct
  ASSERT_TRUE(hc.next_request(&htreq3));
  ASSERT_EQ("/baz", htreq3.uri());
  ASSERT_EQ("somehost.foo.bar", htreq3.GetHeaderValue("host"));
  ASSERT_EQ("keep-alive", htreq3.GetHeaderValue("connection"));
  ASSERT_EQ("some_value", htreq3.GetHeaderValue("other"));
  ASSERT_EQ(3, htreq3.GetHeaderCount());

  // make sure that we read/wrote the correct
  // number of times and clean up
  ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
  num_read += 1;
  ASSERT_EQ(3, num_write);
  ASSERT_EQ(3, num_read);
  ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
  ASSERT_EQ(0, pthread_mutex_destroy(&rw_lock));
  ASSERT_EQ(0, pthread_join(thread_id, nullptr));

  ProjectEnvironment::AddPoints(10);

  // Clean up.
  close(spair[0]);
  close(spair[1]);
}

static void WritePartialRequests(void* args) {
  int socket = *static_cast<int*>(args);
  // Write three requests on the socket.
  // Note that the writes for these requests
  // are split up in such a way to test that
  // your http connection works properly if
  // some of the request that are read are not
  // read completely.
  string req1 = "GET /foo HTTP/1.1\r\n";
  req1 += "Host: somehost.foo.bar\r\n";
  req1 += "Connection: close\r\n";
  req1 += "\r\nGET /bar ";
  // req2: check that it is ok if the request
  // is split in the first line
  // across calls to wrapped_read().
  string req2 = "HTTP/1.1\r\n";
  req2 += "Connection: close\r\n";
  req2 += "Host: somehost.foo.bar\r\n";
  req2 += "\r\nGET /baz HTTP/1.1\r\n";
  req2 += "connection:";
  // req3: check that it is ok if the request
  // is split in the "headers" and during the
  // end \r\n\r\n sequence
  string req3 = " keep-alive\r\n";
  req3 += "host: somehost.foo.bar\r\n";
  req3 += "OTHER: some_value\r\n";
  string req3tail = "\r\n";

  // acquire the rw lock to make sure that
  // the socket doesn't read until we have
  // written req1
  ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
  ASSERT_EQ(0, num_read);
  ASSERT_EQ(0, num_write);

  // write req1
  ASSERT_EQ(static_cast<int>(req1.size()),
            wrapped_write(socket, req1));

  // Release lock and update num_write so that
  // the read socket thread knows it can proceed
  // to read req1
  num_write += 1;
  ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
  sleep(3);

  // Loop trying to acquire the lock but checking
  // to make sure that the client has had the chance
  // to start reading req1
  while (1) {
    ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
    if (num_read == 1) {
      num_write += 1;
      break;
    }
    ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
    sleep(1);
  }

  // now that we know req1 has started to have been read
  // (probably), we write req2.
  ASSERT_EQ(static_cast<int>(req2.size()),
            wrapped_write(socket, req2));
  ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
  sleep(3);

  // Loop trying to acquire the lock but checking
  // to make sure that the reading thread has had
  // the chance to start reading req2
  while (1) {
    ASSERT_EQ(0, pthread_mutex_lock(&rw_lock));
    if (num_read == 2) {
      num_write += 1;
      break;
    }
    ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));
    sleep(1);
  }

  // Write the first part of req3
  ASSERT_EQ(static_cast<int>(req3.size()),
            wrapped_write(socket, req3));
  ASSERT_EQ(0, pthread_mutex_unlock(&rw_lock));

  // sleep so that the read thread has a chance to
  // start reading req3.
  sleep(3);

  // write the last \r\n necessary to mark the
  // end of req3. If you are going infinite at
  // this point, you are not handling the reading
  // of http requests properly
  ASSERT_EQ(static_cast<int>(req3tail.size()),
            wrapped_write(socket, req3tail));
}

TEST(Test_HttpConnection, write_response) {
  ProjectEnvironment::OpenTestCase();
  string expected = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n";
  expected += "Content-length: 3\r\n\r\nhi!";

  HttpResponse response;
  response.set_protocol("HTTP/1.1");
  response.set_response_code(200);
  response.set_message("OK");
  response.set_content_type("text/plain");
  response.AppendToBody("hi!");

  int pipefds[2];
  pipe(pipefds);
  
  HttpConnection connection(pipefds[1]);
  connection.write_response(response);

  string actual;
  wrapped_read(pipefds[0], &actual);
  ASSERT_EQ(expected, actual);
  close(pipefds[0]);

  ProjectEnvironment::AddPoints(10);
}

}  // namespace searchserver
