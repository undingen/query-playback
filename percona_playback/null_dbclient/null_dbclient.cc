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

#include <percona_playback/plugin.h>
#include <percona_playback/db_thread.h>
#include <percona_playback/query_result.h>

class NULLDBThread : public DBThread
{
 public:
  NULLDBThread(uint64_t _thread_id, boost::chrono::duration<int64_t, boost::micro> diff) :
          DBThread(_thread_id, diff) {
  }

  bool connect() { return true; };
  void disconnect() {};
  void execute_query(boost::string_ref q, QueryResult *r,
		     const QueryResult &expected_result) {
    //std::cout << "executing: " <<  boost::chrono::system_clock::now() << " '" << q << "'" << std::endl;
    *r= expected_result;
  };
};

class NULLDBClientPlugin : public percona_playback::DBClientPlugin
{
public:
  NULLDBClientPlugin(std::string _name) : DBClientPlugin(_name) {};

  virtual DBThread* create(uint64_t _thread_id, boost::chrono::duration<int64_t, boost::micro> diff) {
    return new NULLDBThread(_thread_id, diff);
  }
};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("null", new NULLDBClientPlugin("null"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
