/* BEGIN LICENSE
 * Copyright (C) 2011-2013 Percona Ireland Ltd.
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 * END LICENSE */

#ifndef PERCONA_PLAYBACK_QUERY_LOG_H
#define PERCONA_PLAYBACK_QUERY_LOG_H

#include <percona_playback/visibility.h>
#include "percona_playback/query_entry.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/utility/string_ref.hpp>
#include <tbb/atomic.h>

PERCONA_PLAYBACK_API
int run_query_log(const std::string &log_file, unsigned int read_count, struct percona_playback_run_result *r);

#ifdef __cplusplus
extern "C"
{
#endif

class DBThread;

class QueryLogEntry : public QueryEntry
{
private:
  uint64_t rows_sent;
  uint64_t rows_examined;
  boost::chrono::system_clock::time_point start_time;
  double query_time;
  boost::string_ref unprocessed_query;
public:
  QueryLogEntry(uint64_t _thread_id = 0)
    : QueryEntry(_thread_id), rows_sent(0), rows_examined(0), query_time(0) {}

  void setTime(boost::chrono::system_clock::time_point time) { start_time = time; }
  boost::chrono::system_clock::time_point getStartTime() const { return start_time - boost::chrono::microseconds((long)(query_time*(10^6))); }
  double getQueryTime() { return query_time; }

  bool parse_metadata(boost::string_ref s);

  bool hasQuery() const { return !unprocessed_query.empty(); }
  std::string getQuery(bool remove_timestamp);
  void setQuery(boost::string_ref s) { unprocessed_query = s; }

  void display()
  {
    std::cerr << "    " << getQuery(true) << std::endl;
  }

  bool is_quit()
  {
    return unprocessed_query.starts_with("# administrator command: Quit;");
  }

  void execute(DBThread *t);

  bool operator<(const QueryLogEntry& second) const;
};


#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_QUERY_LOG_H */
