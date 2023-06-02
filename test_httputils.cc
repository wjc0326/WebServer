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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

#include "./HttpUtils.h"
#include "./FileReader.h"

#include "gtest/gtest.h"
#include "./test_suite.h"

using std::string;

namespace searchserver {

TEST(Test_HttpUtils, is_path_safe) {
  ProjectEnvironment::OpenTestCase();

  string basedir = "test_files/ok";
  string file1_ok = "test_files/ok/./bar";
  string file2_ok = "test_files/ok/baz/../bar";
  string file3_ok = "test_files/ok/../ok/baz";
  string file4_bad = "test_files/ok/../bad";
  string file5_bad = "test_files/ok/./..";
  string file6_bad = "//etc/passwd";
  string file7_bad = "test_files/ok_not_really/private.txt";

  ASSERT_TRUE(is_path_safe(basedir, file1_ok));
  ASSERT_TRUE(is_path_safe(basedir, file2_ok));
  ASSERT_TRUE(is_path_safe(basedir, file3_ok));
  ASSERT_FALSE(is_path_safe(basedir, file4_bad));
  ASSERT_FALSE(is_path_safe(basedir, file5_bad));
  ASSERT_FALSE(is_path_safe(basedir, file6_bad));
  ASSERT_FALSE(is_path_safe(basedir, file7_bad));
  ProjectEnvironment::AddPoints(20);
}

TEST(Test_HttpUtils, escape_html) {
  ProjectEnvironment::OpenTestCase();

  string noReplace = "Strom Static Sleep Antennas";
  ASSERT_EQ(noReplace, escape_html(noReplace));

  string ampersand = "Triumph & Disaster";
  ASSERT_EQ("Triumph &amp; Disaster", escape_html(ampersand));

  string quotes = "\"HKE 2048\"";
  ASSERT_EQ("&quot;HKE 2048&quot;", escape_html(quotes));

  string apos = "\'Animals\'";
  ASSERT_EQ("&apos;Animals&apos;", escape_html(apos));

  string angleBrackets = "vectroid<int>";
  ASSERT_EQ("vectroid&lt;int&gt;", escape_html(angleBrackets));

  string all = "<\"Clouds\" & \'Nevermind The Name\'>";
  string expected;
  expected = "&lt;&quot;Clouds&quot; &amp; &apos;Nevermind The Name&apos;&gt;";
  ASSERT_EQ(expected, escape_html(all));

  ProjectEnvironment::AddPoints(15);
}

TEST(Test_HttpUtils, wrapped_read_write) {
  string filedata = "This is a test; this is only a test.\n";

  // Make sure the file we'll write/read is deleted.
  unlink("test_files/test.txt");

  // Open the file and write to it.
  int file_fd = open("test_files/test.txt",
                     O_RDWR | O_CREAT,
                     S_IRUSR | S_IWUSR);
  ASSERT_NE(-1, file_fd);
  ASSERT_EQ(static_cast<int>(filedata.size()),
            wrapped_write(file_fd, filedata));
  close(file_fd);

  // Reopen the file and read it in.
  string readstr;
  file_fd = open("test_files/test.txt", O_RDONLY);
  ASSERT_NE(-1, file_fd);
  ASSERT_EQ(static_cast<int>(filedata.size()),
            wrapped_read(file_fd, &readstr));
  close(file_fd);
  ASSERT_TRUE(readstr == filedata);

  // Delete the file.
  unlink("test_files/test.txt");
}

TEST(Test_HttpUtils, decode_URI) {
  // Test out URIDecoding.
  string empty("");
  string plain("foo");
  string two("%74%77%6f");
  string twoupper("%74%77%6F");
  string nope("%16nope");
  string broken("%broken%1");
  string spacey("%20+blah blah");
  ASSERT_EQ(string(""), decode_URI(empty));
  ASSERT_EQ(string("foo"), decode_URI(plain));
  ASSERT_EQ(string("two"), decode_URI(two));
  ASSERT_EQ(string("two"), decode_URI(twoupper));
  ASSERT_EQ(string("%16nope"), decode_URI(nope));
  ASSERT_EQ(string("%broken%1"), decode_URI(broken));
  ASSERT_EQ(string("  blah blah"), decode_URI(spacey));
}

TEST(Test_HttpUtils, URLParser) {
  // Test out URL parsing.
  string easy("/foo/bar");
  string tricky("/foo/bar?");
  string query("/foo/bar?foo=blah+blah");
  string many("/foo/bar?foo=bar&bam=baz");
  string manyshort("/foo/bar?foo=%22bar%22&bam=baz");

  URLParser p;
  p.parse(easy);
  ASSERT_EQ("/foo/bar", p.path());
  ASSERT_EQ((unsigned) 0, p.args().size());

  p.parse(tricky);
  ASSERT_EQ("/foo/bar", p.path());
  ASSERT_EQ((unsigned) 0, p.args().size());

  p.parse(query);
  ASSERT_EQ("/foo/bar", p.path());
  ASSERT_EQ((unsigned) 1, p.args().size());
  ASSERT_EQ("blah blah", p.args()["foo"]);

  p.parse(many);
  ASSERT_EQ("/foo/bar", p.path());
  ASSERT_EQ((unsigned) 2, p.args().size());
  ASSERT_EQ("bar", p.args()["foo"]);
  ASSERT_EQ("baz", p.args()["bam"]);

  p.parse(manyshort);
  ASSERT_EQ("/foo/bar", p.path());
  ASSERT_EQ((unsigned) 2, p.args().size());
  ASSERT_EQ("\"bar\"", p.args()["foo"]);
  ASSERT_EQ("baz", p.args()["bam"]);
}

}  // namespace searchserver
