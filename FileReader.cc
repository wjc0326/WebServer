/*
 * Copyright ©2023 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to the students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>

#include "./HttpUtils.h"
#include "./FileReader.h"

using std::string;

namespace searchserver {

bool FileReader::read_file(string *str) {
  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).

  // TODO: implement
  int fd = open(fname_.c_str(), O_RDONLY);
  if (fd == -1) {
    // detect file open error
    return false;
  }

  string result_string;
  char single_char;
  int count = 0;
  while (true) {
    // keep looping until read success or encounter faluire 
    ssize_t result = read(fd, &single_char, 1); // read in one single char
    if (result == 0){
      // if reach the end of the file
      break;
    }else if (result == -1){
      // if encounter error
      if (errno != EINTR) {
        // if real error happened, exit
        close(fd);
        return false;
      }
      continue; // else, try again
    }
    result_string += single_char;
    count ++;
  }

  *str = result_string;

  close(fd);
  return true;
}

}  // namespace searchserver
