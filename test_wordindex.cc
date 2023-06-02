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

#include <cstdlib>
#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "./test_suite.h"

#include "./WordIndex.h"

using std::string;
using std::vector;

namespace searchserver {

TEST(Test_WordIndex, Simple) {
  ProjectEnvironment::OpenTestCase();
  string doc_name1 = "./HK.hke";
  string doc_name2 = "./Odyssey.home";

  // We need to assign these logically-constant strings into a non-const
  // pointers because the compiler won't let me cast away the const
  // qualifier on a string literal.
  string bananas = "bananas";
  string pears = "pears";
  string apples = "apples";
  string grapes = "grapes";

  WordIndex index;

  // Document 1 has bananas, pears, and apples.
  std::cout << "bananas, doc_name1"<<std::endl;
  index.record(bananas, doc_name1);  
  std::cout << "bananas, doc_name1"<<std::endl;
  index.record(bananas, doc_name1);
  std::cout << "pears, doc_name1"<<std::endl;
  index.record(pears, doc_name1);
  std::cout << "apples, doc_name1"<<std::endl;
  index.record(apples, doc_name1);
  std::cout << "apples, doc_name1"<<std::endl;
  index.record(apples, doc_name1);
  std::cout << "apples, doc_name1"<<std::endl;
  index.record(apples, doc_name1);

  // Document 2 only has apples and bananas.
  std::cout << "apples, doc_name2"<<std::endl;
  index.record(apples, doc_name2);
  std::cout << "bananas, doc_name1"<<std::endl;
  index.record(bananas, doc_name2);

  ASSERT_EQ(3U, index.num_words());

  // No results.
  vector<string> q1 {grapes};
  auto res1 = index.lookup_word(grapes);
  ASSERT_EQ(0U, res1.size());
  res1 = index.lookup_query(q1);
  ASSERT_EQ(0U, res1.size());

  ProjectEnvironment::AddPoints(5);

  // One resultant document.
  vector<string> q2{pears};
  auto res2 = index.lookup_query(q2);
  ASSERT_EQ(1U, res2.size());
  auto it = res2.begin();
  ASSERT_EQ(doc_name1, it->doc_name);
  ASSERT_EQ(1, it->rank);

  ProjectEnvironment::AddPoints(5);

  // Multiple resultant documents.
  vector<string> q3 {apples};
  auto res3 = index.lookup_query(q3);
  ASSERT_EQ(2U, res3.size());
  it = res3.begin();
  ASSERT_EQ(doc_name1, it->doc_name);
  ASSERT_EQ(3, it->rank);
  it++;
  ASSERT_EQ(doc_name2, it->doc_name);
  ASSERT_EQ(1, it->rank);

  ProjectEnvironment::AddPoints(10);

  // Multiple search terms.
  vector<string> q4 {apples, bananas};
  auto res4 = index.lookup_query(q4);
  ASSERT_EQ(2U, res4.size());
  it = res4.begin();
  ASSERT_EQ(doc_name1, it->doc_name);
  ASSERT_EQ(5, it->rank);
  it++;
  ASSERT_EQ(doc_name2, it->doc_name);
  ASSERT_EQ(2, it->rank);
  ProjectEnvironment::AddPoints(10);

  // Multiple search terms: testing different term order.
  vector<string> q5 {bananas, apples};
  auto res5 = index.lookup_query(q5);
  ASSERT_EQ(2U, res5.size());
  it = res5.begin();
  ASSERT_EQ(doc_name1, it->doc_name);
  ASSERT_EQ(5, it->rank);
  it++;
  ASSERT_EQ(doc_name2, it->doc_name);
  ASSERT_EQ(2, it->rank);
  ProjectEnvironment::AddPoints(5);

  // Multiple search terms: not all documents should be results.
  vector<string> q6 {pears};
  auto res6 = index.lookup_query(q6);
  ASSERT_EQ(1U, res6.size());
  it = res6.begin();
  ASSERT_EQ(doc_name1, it->doc_name);
  ASSERT_EQ(1, it->rank);

  ProjectEnvironment::AddPoints(10);
}

}  // namespace searchserver
