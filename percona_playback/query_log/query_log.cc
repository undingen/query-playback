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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>

#include <tbb/pipeline.h>
#include <tbb/tick_count.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tbb_allocator.h>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>

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
#include <boost/utility/string_ref.hpp>

namespace po= boost::program_options;

static bool g_run_set_timestamp;
static bool g_preserve_query_time;

extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

class ParseQueryLogFunc: public tbb::filter {
public:
  ParseQueryLogFunc(boost::string_ref data_,
		    unsigned int run_count_,
		    tbb::atomic<uint64_t> *entries_,
		    tbb::atomic<uint64_t> *queries_)
    : tbb::filter(true),
      nr_entries(entries_),
      nr_queries(queries_),
      data(data_),
      run_count(run_count_),
      pos(0)
  {
      std::cout << "ffoooo " << sizeof(QueryEntry) << std::endl;
  };

  void* operator() (void*);

private:
  bool parse_time(const std::string& s);
  boost::string_ref readline();

private:
  tbb::atomic<uint64_t> *nr_entries;
  tbb::atomic<uint64_t> *nr_queries;
  boost::string_ref data;
  unsigned int run_count;
  boost::string_ref next_line;
  boost::posix_time::ptime first_query_time;

  boost::posix_time::ptime start_time;
  boost::string_ref::size_type pos;
};

void* dispatch(void *input_);

boost::string_ref ParseQueryLogFunc::readline() {
  boost::string_ref::size_type new_pos = data.substr(pos).find('\n');
  boost::string_ref line = data.substr(pos, new_pos + 1);
  if (pos != boost::string_ref::npos)
    pos += new_pos+1;
  return line;
}

void* ParseQueryLogFunc::operator() (void*)  {
  std::vector<boost::shared_ptr<QueryLogEntry> > *entries=
    new std::vector<boost::shared_ptr<QueryLogEntry> >();

  boost::shared_ptr<QueryLogEntry> tmp_entry(new QueryLogEntry());
 // entries->push_back(tmp_entry);

  boost::string_ref line;

  if (!next_line.empty())
  {
    line= next_line;
    next_line.clear();
  }
  else
  {
    line = readline();
    if (line.empty()) {
      delete entries;
      return NULL;
    }
  }


  int count= 0;

  for (;;) {
    if (line.starts_with("# Time")) {
      parse_time(line.to_string());
      goto next;
    }

    if (line[0] != '#' && line.ends_with("started with:\n"))
      goto next;

    if (line[0] != '#' && line.starts_with("Tcp port: "))
      goto next;

    if (line[0] != '#' && line.starts_with("Time Id Command Argument"))
      goto next;

    /*
      # fixme, process admin commands.
      if (strcmp(p,"# administrator command: Prepare;\n") == 0)
      goto next;
    */

    if (line.starts_with("# User@Host"))
    {
      if (!tmp_entry->getQuery().empty())
        entries->push_back(tmp_entry);
      count++;
      tmp_entry.reset(new QueryLogEntry());
      tmp_entry->setTime(start_time);
      (*this->nr_entries)++;
    }

    if (line[0] == '#')
      tmp_entry->parse_metadata(line.to_string());
    else
    {
      (*nr_queries)++;
      tmp_entry->add_query_line(line.to_string());
      do {
        line = readline();
        if (line.empty())
        {
          break;
        }

        if (line[0] == '#')
        {
          next_line= line;
          break;
        }
        tmp_entry->add_query_line(line.to_string());
      } while(true);
    }
  next:
    if (count > 100)
    {
      count= 0;
      //      fseek(input_file,-len, SEEK_CUR);
      break;
    }
    if (next_line.empty())
    {
      line = readline();
      if (line.empty())
        break;
    }
    next_line.clear();
  }

  if (!tmp_entry->getQuery().empty())
    entries->push_back(tmp_entry);

  return entries;
}

bool ParseQueryLogFunc::parse_time(const std::string& s) {
  // # Time: 090402 9:23:36
  // # Time: 090402 9:23:36.123456
  static const boost::regex time_regex("# Time: (\\d\\d)(\\d\\d)(\\d\\d) (\\d+):(\\d+):(\\d+)\\.?(\\d+)?\\s*");
  boost::smatch results;
  if (!boost::regex_match(s, results, time_regex))
      return false;

  int year = std::atol(results.str(1).c_str());
  year += year < 70 ? 2000 : 1900;
  boost::gregorian::date date(year, std::atol(results.str(2).c_str()), std::atol(results.str(3).c_str()));
  boost::posix_time::time_duration td(std::atol(results.str(4).c_str()), std::atol(results.str(5).c_str()), std::atol(results.str(6).c_str()));
  if (results[7].matched /* microsecs */) {
    td += boost::posix_time::microseconds(std::atol(results.str(7).c_str()));
  }
  start_time = boost::posix_time::ptime(date, td);

  // retrieve the time of the first query
  if (first_query_time.is_not_a_date_time() || start_time < first_query_time)
      first_query_time = start_time;

  std::cout << start_time << " " << first_query_time << std::endl;
  return true;
}

void QueryLogEntry::execute(DBThread *t)
{
  std::vector<std::string>::iterator it;
  QueryResult r;

  if(g_run_set_timestamp)
  {
    QueryResult expected_result;
    QueryResult discarded_timestamp_result;
    expected_result.setRowsSent(0);
    expected_result.setRowsExamined(0);
    expected_result.setError(0);
    t->execute_query(set_timestamp_query, &discarded_timestamp_result,
		     expected_result);
  }

  QueryResult expected_result;
  expected_result.setRowsSent(rows_sent);
  expected_result.setRowsExamined(rows_examined);
  expected_result.setError(0);

  boost::posix_time::time_duration expected_duration=
    boost::posix_time::microseconds(long(query_time * 1000000));
  expected_result.setDuration(expected_duration);

  boost::posix_time::ptime start_time;
  start_time= boost::posix_time::microsec_clock::universal_time();

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

void QueryLogEntry::add_query_line(const std::string &s)
{
  const std::string timestamp_query("SET timestamp=");
  if(!g_run_set_timestamp
     && s.compare(0, timestamp_query.length(), timestamp_query) == 0)
    set_timestamp_query= s;
  else
  {
    //Append space insead of \r\n
    std::string::const_iterator end = s.end() - 1;
    if (s.length() >= 2 && *(s.end() - 2) == '\r')
      --end;
    //Remove initial spaces for best query viewing in reports
    std::string::const_iterator begin;
    for (begin = s.begin(); begin != end; ++begin)
      if (*begin != ' ' && *begin != '\t')
        break;
    query.append(begin, end);
    query.append(" ");
  }
}

bool QueryLogEntry::parse_metadata(const std::string &s)
{
  bool r= false;
  {
    size_t location= s.find("Thread_id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.c_str() + location + strlen("Thread_Id: "), NULL, 10);
      r= true;
    }
  }
  {
    // starting from MySQL 5.6.2 (bug #53630) the thread id is included as "Id:"
    size_t location= s.find("Id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.c_str() + location + strlen("Id: "), NULL, 10);
      r= true;
    }
  }

  {
    size_t location= s.find("Rows_sent: ");
    if (location != std::string::npos)
    {
      rows_sent = strtoull(s.c_str() + location + strlen("Rows_sent: "), NULL, 10);
      r= true;
    }
  }

  {
    size_t location= s.find("Rows_Examined: ");
    if (location != std::string::npos)
    {
      rows_examined = strtoull(s.c_str() + location + strlen("Rows_examined: "), NULL, 10);
      r= true;
    }
  }

  {
    std::string qt_str("Query_time: ");
    size_t location= s.find(qt_str);
    if (location != std::string::npos)
    {
      query_time= strtod(s.c_str() + location + qt_str.length(), NULL);
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

void* dispatch (void *input_)
{
    std::vector<boost::shared_ptr<QueryLogEntry> > *input= 
      static_cast<std::vector<boost::shared_ptr<QueryLogEntry> >*>(input_);
    for (unsigned int i=0; i< input->size(); i++)
    {
      //      usleep(10);
      g_dispatcher_plugin->dispatch((*input)[i]);
    }
    delete input;
    return NULL;
}

class DispatchQueriesFunc : public tbb::filter {
public:
  DispatchQueriesFunc() : tbb::filter(true) {};

  void* operator() (void *input_)
  {
    return dispatch(input_);
  }
};

static void LogReaderThread(boost::string_ref data, unsigned int run_count, struct percona_playback_run_result *r)
{
  tbb::pipeline p;
  tbb::atomic<uint64_t> entries;
  tbb::atomic<uint64_t> queries;
  entries=0;
  queries=0;

  ParseQueryLogFunc f2(data, run_count, &entries, &queries);
  DispatchQueriesFunc f4;
  p.add_filter(f2);
  p.add_filter(f4);
  p.run(2);

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

  virtual void run(percona_playback_run_result  &result)
  {
    int fd = -1;
    boost::string_ref data;
    if (std_in)
    {
      assert(0);
    }
    else
    {
      struct stat s;
      fd = open(file_name.c_str(), O_RDONLY);
      if (fd == -1 || fstat(fd, &s) == -1) {
        fprintf(stderr,
          _("ERROR: Error opening file '%s': %s"),
          file_name.c_str(), strerror(errno));
        return;
      }
      int size = s.st_size;
      const char* ptr = (const char *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
      data = boost::string_ref(ptr, size);
    }

    boost::thread log_reader_thread(LogReaderThread,
            data,
				    read_count,
				    &result);

    log_reader_thread.join();
    munmap(const_cast<char*>(data.data()), data.size());
    if (fd != -1)
      close(fd);
  }
};

static void init(percona_playback::PluginRegistry&r)
{
  r.add("query-log", new QueryLogPlugin("query-log"));
}

PERCONA_PLAYBACK_PLUGIN(init);
