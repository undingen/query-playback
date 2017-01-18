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

#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <ctime>
#include <boost/thread.hpp>
#include "query_log.h"
#include <unistd.h>
#include <time.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>

#include <tbb/atomic.h>

#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>
#include <percona_playback/db_thread.h>
#include <percona_playback/query_log/query_log.h>
#include <percona_playback/query_result.h>
#include <percona_playback/gettext.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>

namespace po= boost::program_options;

static bool g_run_set_timestamp;
static bool g_preserve_query_time;
static bool g_preserve_query_starttime;

extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

class ParseQueryLogFunc {
public:
  ParseQueryLogFunc(boost::string_ref data_,
		    unsigned int run_count_,
		    tbb::atomic<uint64_t> *entries_,
		    tbb::atomic<uint64_t> *queries_)
    : nr_entries(entries_),
      nr_queries(queries_),
      data(data_),
      run_count(run_count_),
      pos(0)
  {
  }

  boost::shared_ptr<QueryEntryPtrVec> getEntries();

private:
  bool parse_time(boost::string_ref s, boost::chrono::system_clock::time_point& start_time);
  boost::string_ref readline();

private:
  tbb::atomic<uint64_t> *nr_entries;
  tbb::atomic<uint64_t> *nr_queries;
  boost::string_ref data;
  unsigned int run_count;

  boost::string_ref::size_type pos;
};

boost::string_ref ParseQueryLogFunc::readline() {
  boost::string_ref::size_type new_pos = data.substr(pos).find('\n');
  boost::string_ref line = data.substr(pos, new_pos + 1);
  if (pos != boost::string_ref::npos)
    pos += new_pos + 1;
  return line;
}

bool QueryLogEntry::operator<(const QueryEntry& _second) const {
  const QueryLogEntry& second = static_cast<const QueryLogEntry&>(_second);

  // for same connections we make sure that the follow the order in the query log
  if (getThreadId() == second.getThreadId())
    return unprocessed_query.data() < second.unprocessed_query.data();
  return getStartTime() < second.getStartTime();
}

static boost::string_ref trim(boost::string_ref str, boost::string_ref chars) {
  boost::string_ref::size_type start_pos = str.find_first_not_of(chars);
  if (start_pos == boost::string_ref::npos)
    start_pos = 0;
  return str.substr(start_pos, str.find_last_not_of(chars) + 1 - start_pos);
}

boost::shared_ptr<QueryEntryPtrVec> ParseQueryLogFunc::getEntries()  {
  boost::shared_ptr<QueryEntryPtrVec> entries = boost::shared_ptr<QueryEntryPtrVec>(new QueryEntryPtrVec);
  boost::shared_ptr<QueryLogEntry> tmp_entry;

  boost::string_ref line, next_line;

  boost::chrono::system_clock::time_point start_time;

  for (;;) {
    if (next_line.empty()) {
      line = readline();
      if (line.empty())
        break;
    } else {
      line = next_line;
      next_line.clear();
    }

    if (line.starts_with("# Time")) {
      parse_time(line, start_time);
      continue;
    }

    if (line[0] != '#' && line.ends_with("started with:\n"))
      continue;

    if (line[0] != '#' && line.starts_with("Tcp port: "))
      continue;

    // skip lines like: "Time[ ]+Id[ ]+Command[ ]+Argument"
    if (line[0] != '#' && line.starts_with("Time "))
      continue;

    /*
      # fixme, process admin commands.
      if (strcmp(p,"# administrator command: Prepare;\n") == 0)
      goto next;
    */

    if (line.starts_with("# User@Host"))
    {
      if (tmp_entry && tmp_entry->hasQuery()) {
        entries->push(tmp_entry);
      }
      tmp_entry.reset(new QueryLogEntry());
      tmp_entry->setTime(start_time);
      (*this->nr_entries)++;
    }

    if (line[0] == '#')
      tmp_entry->parse_metadata(line);
    else
    {
      (*nr_queries)++;

      // read whole query - can be multiple lines
      do {
        next_line = readline();
      } while (!next_line.empty() && !next_line.starts_with('#'));

      boost::string_ref query(line.data(), next_line.data() - line.data());

      tmp_entry->setQuery(trim(query, " \n\r\t"));
    }
  }

  if (tmp_entry && tmp_entry->hasQuery()) {
    entries->push(tmp_entry);
  }

  if (entries->empty())
    return entries;

  return entries;
}

bool ParseQueryLogFunc::parse_time(boost::string_ref s, boost::chrono::system_clock::time_point& start_time) {
  // # Time: 090402 9:23:36
  // # Time: 090402 9:23:36.123456

#if 0
  static const boost::regex time_regex("# Time: (\\d\\d)(\\d\\d)(\\d\\d) (\\d+):(\\d+):(\\d+)\\.?(\\d+)?\\s*", boost::regex_constants::optimize);
  boost::cmatch results;
  if (!boost::regex_match(s.begin(), s.end(), results, time_regex))
      return false;

  int year = std::atol(results.str(1).c_str());
  year += year < 70 ? 2000 : 1900;
  boost::gregorian::date date(year, std::atol(results.str(2).c_str()), std::atol(results.str(3).c_str()));
  boost::posix_time::time_duration td(std::atol(results.str(4).c_str()), std::atol(results.str(5).c_str()), std::atol(results.str(6).c_str()));
  if (results[7].matched /* microsecs */) {
    td += boost::posix_time::microseconds(std::atol(results.str(7).c_str()));
  }
#endif

  static const boost::regex time_regex("# Time: (\\d\\d)(\\d\\d)(\\d\\d) (\\d+):(\\d+):(\\d+)\\.?(\\d+)?\\s*", boost::regex_constants::optimize);
  boost::cmatch results;
  if (!boost::regex_match(s.begin(), s.end(), results, time_regex))
      return false;

  int year = std::atol(results.str(1).c_str());
  std::tm td;
  memset(&td, 0, sizeof(td));
  td.tm_year = year < 70 ? 100 + year : year;
  td.tm_mon = std::atol(results.str(2).c_str());
  td.tm_mday = std::atol(results.str(3).c_str());
  td.tm_hour = std::atol(results.str(4).c_str());
  td.tm_min = std::atol(results.str(5).c_str());
  td.tm_sec = std::atol(results.str(6).c_str());

  start_time = boost::chrono::system_clock::from_time_t(std::mktime(&td));

  if (results[7].matched /* microsecs */)
    start_time += boost::chrono::microseconds(std::atol(results.str(7).c_str()));

  return true;
}

void QueryLogEntry::execute(DBThread *t)
{
  std::string query = getQuery(!g_run_set_timestamp);

  boost::this_thread::sleep_until(getStartTime() + t->getDiff());

  QueryResult expected_result;
  expected_result.setRowsSent(rows_sent);
  expected_result.setRowsExamined(rows_examined);
  expected_result.setError(0);

  boost::posix_time::time_duration expected_duration=
    boost::posix_time::microseconds(long(query_time * 1000000));
  expected_result.setDuration(expected_duration);

  boost::posix_time::ptime start_time;
  start_time= boost::posix_time::microsec_clock::universal_time();

  QueryResult r;
  t->execute_query(query, &r, expected_result);

  boost::posix_time::ptime end_time;
  end_time= boost::posix_time::microsec_clock::universal_time();

  boost::posix_time::time_period duration(start_time, end_time);
  r.setDuration(duration.length());

  if (g_preserve_query_time
      && expected_duration > duration.length())
  {
    boost::posix_time::time_duration us_sleep_time=
      expected_duration - duration.length();

    usleep(us_sleep_time.total_microseconds());
  }

  BOOST_FOREACH(const percona_playback::PluginRegistry::ReportPluginPair pp,
		percona_playback::PluginRegistry::singleton().report_plugins)
  {
    pp.second->query_execution(getThreadId(),
			       query,
			       expected_result,
			       r);
  }
}

std::string QueryLogEntry::getQuery(bool remove_timestamp) {
  static const boost::regex format_r("SET timestamp=[^\n]*\n[ \t]*", boost::regex_constants::optimize);
  static const boost::regex format("[ ]*\r?\n[ \t]*", boost::regex_constants::optimize);

  std::string ret;
  ret.reserve(unprocessed_query.size());

  if (remove_timestamp) {
    std::string tmp;
    tmp.reserve(unprocessed_query.size());
    boost::regex_replace(std::back_insert_iterator<std::string>(tmp), unprocessed_query.begin(), unprocessed_query.end(), format_r, "");
    boost::regex_replace(std::back_insert_iterator<std::string>(ret), tmp.begin(), tmp.end(), format, " ");
  } else {
    boost::regex_replace(std::back_insert_iterator<std::string>(ret), unprocessed_query.begin(), unprocessed_query.end(), format, " ");
  }
  return ret;
}

bool QueryLogEntry::parse_metadata(boost::string_ref s)
{
  bool r= false;
  {
    size_t location= s.find("Thread_id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.substr(location + strlen("Thread_Id: ")).data(), NULL, 10);
      r= true;
    }
  }
  {
    // starting from MySQL 5.6.2 (bug #53630) the thread id is included as "Id:"
    size_t location= s.find("Id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.substr(location + strlen("Id: ")).data(), NULL, 10);
      r= true;
    }
  }

  {
    size_t location= s.find("Rows_sent: ");
    if (location != std::string::npos)
    {
      rows_sent = strtoull(s.substr(location + strlen("Rows_sent: ")).data(), NULL, 10);
      r= true;
    }
  }

  {
    size_t location= s.find("Rows_Examined: ");
    if (location != std::string::npos)
    {
      rows_examined = strtoull(s.substr(location + strlen("Rows_examined: ")).data(), NULL, 10);
      r= true;
    }
  }

  {
    std::string qt_str("Query_time: ");
    size_t location= s.find(qt_str);
    if (location != std::string::npos)
    {
      query_time = strtod(s.substr(location + qt_str.length()).data(), NULL);
      r= true;
    }
  }
/*
  if (s[0] == '#' && strncmp(s.c_str(), "# administrator", strlen("# administrator")))
  {
    query.append(s);
    r= true;
  }
*/
  return r;
}

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

static void LogReaderThread(boost::string_ref data, unsigned int run_count, struct percona_playback_run_result *r)
{
  tbb::atomic<uint64_t> entries;
  tbb::atomic<uint64_t> queries;
  entries=0;
  queries=0;

  boost::shared_ptr<QueryEntryPtrVec> entry_vec = ParseQueryLogFunc(data, run_count, &entries, &queries).getEntries();

  if (!entry_vec->empty()) {
    g_dispatcher_plugin->dispatch(entry_vec);

  }
  g_dispatcher_plugin->finish_all_and_wait();


  r->n_log_entries= entries;
  r->n_queries= queries;
}

class QueryLogPlugin : public percona_playback::InputPlugin
{
private:
  po::options_description     options;
  std::string                 file_name;
  unsigned int                read_count;
  bool			      std_in;

public:
  QueryLogPlugin(const std::string &_name) :
    InputPlugin(_name),
    options(_("Query Log Options")),
    read_count(1),
    std_in(false)
  {};

  virtual boost::program_options::options_description* getProgramOptions() {
      options.add_options()
      ("query-log-file",
       po::value<std::string>(), _("Query log file"))
      ("query-log-stdin",
       po::value<bool>()->default_value(false)->zero_tokens(),
       _("Read query log from stdin"))
/* Disabled for 0.3 until we have something more universal.
      ("query-log-read-count",
       po::value<unsigned int>(&read_count)->default_value(1),
       _("Query log file read count (how many times to read query log file)"))
*/
      ("query-log-set-timestamp",
       po::value<bool>(&g_run_set_timestamp)->
          default_value(false)->
            zero_tokens(), 
       _("By default, we skip the SET TIMESTAMP=XX; query that the MySQL slow "
       "query log always includes. This may cause some subsequent queries to "
       "fail, depending on your workload. If the --run-set-timestamp option "
       "is enabled, we run these queries too."))
      ("query-log-preserve-query-time",
       po::value<bool>(&g_preserve_query_time)->
        default_value(false)->
          zero_tokens(),
       _("Ensure that each query takes at least Query_time (from slow query "
	 "log) to execute."))
      ("query-log-preserve-query-startime",
       po::value<bool>(&g_preserve_query_starttime)->
        default_value(false)->
          zero_tokens(),
       _("Ensure that each query executes at the exact time."))
      ;

    return &options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm)
  {
    if (!active &&
        (vm.count("query-log-file") ||
	 !vm["query-log-stdin"].defaulted() ||
//         !vm["query-log-read-count"].defaulted() ||
         !vm["query-log-preserve-query-time"].defaulted() ||
         !vm["query-log-set-timestamp"].defaulted()))
    {
      fprintf(stderr,_(("query-log plugin is not selected, "
			"you shouldn't use this plugin-related "
			"command line options\n")));
      return -1;
    }

    if (!active)
      return 0;

    if (vm.count("query-log-file") && vm["query-log-stdin"].as<bool>())
    {
      fprintf(stderr,  _(("The options --query-log-file and --query-log-stdin "
			  "can not be used together\n")));
      return -1;
    }

    if (vm.count("query-log-file"))
      file_name= vm["query-log-file"].as<std::string>();
    else if (vm["query-log-stdin"].as<bool>())
    {
      std_in = true;
    }
    else
    {
      fprintf(stderr, _("ERROR: --query-log-file is a required option.\n"));
      return -1;
    }

    return 0;
  }

  virtual void run(percona_playback_run_result &result)
  {
    if (std_in)
    {
      // todo maybe we want to create a temp file in to safe RAM...
      const int block_size = 1024;
      std::string data;
      while (true) {
        std::string::size_type old_size = data.size();
        data.resize(old_size + block_size);
        int num_read = fread(&data[old_size], 1, block_size, stdin);
        if (num_read < block_size) {
          data.resize(old_size + num_read);
          break;
        }
      }
      boost::thread log_reader_thread(LogReaderThread,
                                      data,
                                      read_count,
                                      &result);

      log_reader_thread.join();
    }
    else
    {
      struct stat s;
      int fd = open(file_name.c_str(), O_RDONLY);
      if (fd == -1 || fstat(fd, &s) == -1) {
        fprintf(stderr,
          _("ERROR: Error opening file '%s': %s"),
          file_name.c_str(), strerror(errno));
        return;
      }
      boost::string_ref data;
      int size = s.st_size;
      void* ptr = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
      madvise(ptr, size, MADV_WILLNEED);
      data = boost::string_ref((const char*)ptr, size);

      boost::thread log_reader_thread(LogReaderThread,
                                      data,
                                      read_count,
                                      &result);

      log_reader_thread.join();
      munmap(const_cast<char*>(data.data()), data.size());
      close(fd);
    }
  }
};

static void init(percona_playback::PluginRegistry&r)
{
  r.add("query-log", new QueryLogPlugin("query-log"));
}

PERCONA_PLAYBACK_PLUGIN(init);
