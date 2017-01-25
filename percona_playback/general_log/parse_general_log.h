#ifndef PERCONA_PLAYBACK_PARSE_GENERAL_LOG
#define PERCONA_PLAYBACK_PARSE_GENERAL_LOG

#include <iostream>
#include <stdlib.h>

#include <tbb/atomic.h>

#include <percona_playback/plugin.h>

#ifdef __cplusplus
extern "C"
{
#endif

class ParseGeneralLog {
    public:
      ParseGeneralLog(FILE *input_file_,
                unsigned int run_count_,
                tbb::atomic<uint64_t> *entries_,
                tbb::atomic<uint64_t> *queries_)
        : nr_entries(entries_),
          nr_queries(queries_),
          input_file(input_file_),
          run_count(run_count_),
          next_line(NULL),
          next_len(0)
      {};

      QueryEntryPtrVec getEntries();

    private:
      tbb::atomic<uint64_t> *nr_entries;
      tbb::atomic<uint64_t> *nr_queries;
      FILE *input_file;
      unsigned int run_count;
      char *next_line;
      ssize_t next_len;
};

#ifdef __cplusplus
}
#endif

#endif // PERCONA_PLAYBACK_PARSE_GENERAL_LOG
