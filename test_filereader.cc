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

#include "./FileReader.h"

#include "gtest/gtest.h"
#include "./test_suite.h"

using std::string;

namespace searchserver {

TEST(Test_FileReader, Basic) {
  ProjectEnvironment::OpenTestCase();

  // See if we can read a file successfully.
  FileReader f("./test_files/hextext.txt");
  string contents;
  ASSERT_TRUE(f.read_file(&contents));
  ASSERT_EQ(4800U, contents.size());
  ProjectEnvironment::AddPoints(5);

  // See if we can read a non text file
  // that contains the '\0' byte.
  f = FileReader("./test_files/transparent.gif");
  ASSERT_TRUE(f.read_file(&contents));
  ASSERT_EQ(43U, contents.size());
  ProjectEnvironment::AddPoints(10);

  // Make sure reading a non-existent file fails.
  f = FileReader("./non-existent");
  ASSERT_FALSE(f.read_file(&contents));
  ProjectEnvironment::AddPoints(5);

  // Another non-existant file, but
  // with a more complicated path
  f = FileReader("./test_files/../cpplint.py");
  ASSERT_FALSE(f.read_file(&contents));
  ProjectEnvironment::AddPoints(5);
}

}  // namespace searchserver
