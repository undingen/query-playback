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

#include "percona_playback/plugin.h"
#include "percona_playback/db_thread.h"

#include <boost/chrono.hpp>

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

static void dispatchQueries(QueryEntryPtrVec query_entries) {
  if (query_entries.empty())
    return;

  std::cout << "query_entries: " << query_entries.size() << std::endl;

  typedef std::map<uint64_t, DBThread*> DBExecutorsTable;
  DBExecutorsTable  executors;

  boost::chrono::system_clock::time_point start_time = query_entries[0]->getStartTime();
  boost::chrono::system_clock::time_point now = boost::chrono::system_clock::now();
  boost::chrono::duration<int64_t, boost::micro> diff = boost::chrono::duration_cast<boost::chrono::duration<int64_t, boost::micro> >(now - start_time);

  for (QueryEntryPtrVec::const_iterator it = query_entries.begin(), it_end = query_entries.end(); it != it_end; ++it) {
    QueryEntryPtr entry = *it;

    uint64_t thread_id= entry->getThreadId();
    DBThread*& db_thread = executors[thread_id];
    if (!db_thread)
      db_thread= g_dbclient_plugin->create(thread_id, diff);
    db_thread->queries.push(entry);
  }

  for (DBExecutorsTable::iterator it = executors.begin(), it_end = executors.end(); it != it_end; ++it)
  {
    it->second->start_thread();
  }


  for (DBExecutorsTable::iterator it = executors.begin(), it_end = executors.end(); it != it_end; ++it)
  {
    it->second->join();
    delete it->second;
  }
}

class ThreadPerConnectionDispatcher :
	public percona_playback::DispatcherPlugin
{
  void start_thread(DBThread *thread);

public:
  ThreadPerConnectionDispatcher(std::string _name) :
	  DispatcherPlugin(_name) {}

  void dispatch(const QueryEntryPtrVec &query_entries);
  void finish_all_and_wait();

  boost::thread thread;
};


void
ThreadPerConnectionDispatcher::dispatch(const QueryEntryPtrVec& query_entries)
{
  thread = boost::thread(dispatchQueries, query_entries);
}

void
ThreadPerConnectionDispatcher::finish_all_and_wait()
{
  if (thread.joinable())
    thread.join();
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("thread-per-connection",
	new ThreadPerConnectionDispatcher("thread-per-connection"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
