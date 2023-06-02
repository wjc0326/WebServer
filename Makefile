# Copyright ©2023 Travis McGaha.  All rights reserved.  Permission is
# hereby granted to students registered for University of Pennsylvania
# CIT 5950 for use solely during Spring Semester 2023 for purposes of
# the course.  No other use, copying, distribution, or modification
# is permitted without prior written consent. Copyrights for
# third-party components of this work must be honored.  Instructors
# interested in reusing these course materials should contact the
# author.

# define the commands we will use for compilation and library building
AR = ar
ARFLAGS = rcs
CC = gcc
CXX = g++

# define useful flags to cc/ld/etc.
CFLAGS = -g -Wall -Wpedantic -std=c11 -I. -O0
CXXFLAGS = -g -Wall -Wpedantic -std=c++17 -I. -O0
LDFLAGS = -L. -lpthread
CPPUNITFLAGS = -L../gtest -lgtest

# define common dependencies
OBJS_COMMON = ThreadPool.o ServerSocket.o HttpServer.o HttpConnection.o FileReader.o CrawlFileTree.o WordIndex.o
OBJS_GOOD = $(OBJS_COMMON) HttpUtils.o

HEADERS = HttpConnection.h \
	  HttpServer.h \
	  ServerSocket.h \
	  ThreadPool.h \
	  HttpUtils.h \
	  HttpRequest.h HttpResponse.h \
          CrawlFileTree.h \
          WordIndex.h \
          Result.h \
	  FileReader.h

TESTOBJS = test_filereader.o test_wordindex.o \
           test_crawlfiletree.o test_serversocket.o \
	   test_httpconnection.o test_httputils.o \
           test_threadpool.o test_suite.o

# compile everything except our release-only "with flaws" binary; this
# is the default rule that fires if a user just types "make" in the
# same directory as this Makefile
all: httpd test_suite

httpd: httpd.o projectlib.a $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ httpd.o projectlib.a $(LDFLAGS)

projectlib.a: $(OBJS_GOOD) $(HEADERS)
	$(AR) $(ARFLAGS) $@ $(OBJS_GOOD)

test_suite: $(TESTOBJS) projectlib.a $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(TESTOBJS) \
	$(CPPUNITFLAGS) $(LDFLAGS) projectlib.a -lpthread

%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

clean:
	/bin/rm -f *.o *~ test_suite httpd httpd_withflaws projectlib.a
