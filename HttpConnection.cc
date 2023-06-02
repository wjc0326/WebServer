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

#include <cstdint>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;

namespace searchserver {

static const char *kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::next_request(HttpRequest *request) {
  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  // TODO: implement
  int index = 0;
  while(1) {
    // get the index where the "\r\n\r\n" start
    index = buffer_.find(kHeaderEnd, 0);
    if (index != (int)string::npos) {
        // if find 
        break;
    }
    // keep read in requests
    wrapped_read(fd_, &buffer_);
  }
  
  // get the request header
  string header = buffer_.substr(0, index);
  // deal with the rest 
  int buffer_length = buffer_.length();
  buffer_ = buffer_.substr(index + kHeaderEndLen, buffer_length - index - 4);
  // pass the header into parse_request
  bool if_success = parse_request(header, request);
  
  return if_success;
}

bool HttpConnection::write_response(const HttpResponse &response) {
  // Implement so that the response is converted to a string
  // and written out to the socket for this connection  

  // TODO: implement
  const string response_string = response.GenerateResponseString();
  int response_len = response_string.length();
  int actual_len = wrapped_write(fd_, response_string);

  if(response_len == actual_len) {
    return true;
  }

  return false;
}

bool HttpConnection::parse_request(const string &request, HttpRequest* out) {
  HttpRequest req("/");  // by default, get "/".

  // Split the request into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
  // at HttpRequest.h for details about the HTTP header format that
  // you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  //
  // If a request is malfrormed, return false, otherwise true and 
  // the parsed request is retrned via *out
  
  // TODO: implement

  // trim, to lower case and split request by "\r\n"
  string request_copy = std::string(request);
  boost::algorithm::to_lower(request_copy);
  boost::algorithm::trim(request_copy);  // trim out the white space
  vector<string> lines;
  boost::split(lines, request_copy, boost::is_any_of("\r\n"), boost::token_compress_on);

  // get url
  string first_line = lines[0];
  vector<string> components;
  boost::split(components, first_line, boost::is_any_of(" "), boost::token_compress_on);

  // check whether the request is malfrormed
  if(components[0] != "get"){
    return false;
  }

  // set the uri
  req.set_uri(components[1]);

  // deal with remaining lines
  for (int i = 1; i < (int)lines.size(); i++) {
    boost::split(components, lines[i], boost::is_any_of(":"), boost::token_compress_on);
    boost::algorithm::trim(components[0]);
    boost::algorithm::trim(components[1]);
    req.AddHeader(components[0], components[1]);
  }

  *out = req;

  return true;
}

}  // namespace searchserver
