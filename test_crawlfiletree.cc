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

#include "gtest/gtest.h"
#include "./test_suite.h"

#include "./CrawlFileTree.h"
#include "./WordIndex.h"

namespace searchserver {

TEST(Test_CrawlFileTree, ReadsFromDisk) {
  ProjectEnvironment::OpenTestCase();
  bool res;
  WordIndex idx;

  // Test that it detects a valid directory.
  res = crawl_filetree("./test_tree/bash-4.2/support", &idx);
  ASSERT_EQ(true, res);
  ProjectEnvironment::AddPoints(5);

  // Test that it detects a non-existant directory.
  res = crawl_filetree("./nonexistent/", &idx);
  ASSERT_EQ(false, res);

  // Test that it rejects files (instead of directories).
  res = crawl_filetree("./test_suite.c", &idx);
  ASSERT_EQ(false, res);
}

// Tests that WordIndex and CrawlFileTree
// are properly integrated to work with each other.
TEST(Test_CrawlFileTree, Integration) {
  ProjectEnvironment::OpenTestCase();
  WordIndex idx;

  bool res;

  string q1 = "equations";
  vector<string> q2 = {"report", "normal"};
  vector<string> q3 = {"report", "suggestions", "normal"};
  vector<string> q4 = {"report", "normal", "foobarbaz"};


  // Crawl the test tree.
  res = crawl_filetree(const_cast<char *>("./test_tree/bash-4.2/support"),
                      &idx);
  ASSERT_TRUE(res);
  ASSERT_EQ(3852, idx.num_words());

  // Process query 1, check results.
  auto res1_word = idx.lookup_word(q1);
  auto it = res1_word.begin();

  ASSERT_EQ(2U, res1_word.size());
  ASSERT_EQ("./test_tree/bash-4.2/support/texi2html", it->doc_name);
  it++;
  ASSERT_EQ("./test_tree/bash-4.2/support/man2html.c", it->doc_name);

  ProjectEnvironment::AddPoints(10);

  // what happens if we look up q1 again but stored in a vector of size 1?
  vector<string> q1_query {q1};
  auto res1_query = idx.lookup_query(q1_query);
  it = res1_query.begin();
  ASSERT_EQ(2U, res1_query.size());
  ASSERT_EQ("./test_tree/bash-4.2/support/texi2html", it->doc_name);
  ASSERT_EQ(2, it->rank);
  it++;
  ASSERT_EQ("./test_tree/bash-4.2/support/man2html.c", it->doc_name);
  ASSERT_EQ(1, it->rank);
  ProjectEnvironment::AddPoints(5);

  // Process query 2, check results.
  auto res2 = idx.lookup_query(q2);
  it = res2.begin();
  ASSERT_EQ(2U, res2.size());
  ASSERT_EQ("./test_tree/bash-4.2/support/texi2html", it->doc_name);
  ASSERT_EQ(12, it->rank);
  it++;
  ASSERT_EQ("./test_tree/bash-4.2/support/man2html.c", it->doc_name);
  ASSERT_EQ(3, it->rank);
  ProjectEnvironment::AddPoints(10);

  // Process query 3, check results.
  auto res3 = idx.lookup_query(q3);
  it = res3.begin();
  ASSERT_EQ(1U, res3.size());
  ASSERT_EQ("./test_tree/bash-4.2/support/texi2html", it->doc_name);
  ASSERT_EQ(13, it->rank);
  ProjectEnvironment::AddPoints(10);


  // Process query 4, check results.
  auto res4 = idx.lookup_query(q4);
  ASSERT_EQ(0U, res4.size());
  ProjectEnvironment::AddPoints(5);

}

}  // namespace searchserver

