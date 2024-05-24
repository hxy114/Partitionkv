//
// Created by hxy on 24-5-23.
//
#include <sys/types.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

#include "leveldb/cache.h"
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"
#include "port/port.h"
#include "util/crc32c.h"
#include "util/histogram.h"
#include "util/mutexlock.h"
#include "util/random.h"
#include "util/testutil.h"

#include<string>

leveldb::Env* g_env = nullptr;

int main(int argc, char** argv) {
  leveldb::DB* db_= nullptr;
  g_env = leveldb::Env::Default();
  assert(db_ == nullptr);
  leveldb::Options options;
  options.env = g_env;
  std::string path = "/tmp/leveldbtest-1000/dbbench/";
  const uint64_t start_micros = g_env->NowMicros();
  leveldb::Status s = leveldb::DB::Open(options, path, &db_);
  std::cout << g_env->NowMicros() - start_micros;

  delete db_;


}