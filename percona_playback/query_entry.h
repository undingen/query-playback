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

#ifndef PERCONA_PLAYBACK_QUERY_ENTRY_H
#define PERCONA_PLAYBACK_QUERY_ENTRY_H

#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>
#include <stdint.h>
#include <vector>

class DBThread;

class QueryEntry
{
protected:
  uint64_t thread_id;
  uint64_t id;
public:
  QueryEntry(uint64_t _thread_id = 0) :
    thread_id(_thread_id), id(0) {}
  virtual ~QueryEntry() {}

  virtual bool is_quit()= 0;

  uint64_t getThreadId() const { return thread_id; }

  virtual void execute(DBThread *t)= 0;
  virtual boost::chrono::system_clock::time_point getStartTime() const { return boost::chrono::system_clock::time_point(); }

};

typedef boost::shared_ptr<QueryEntry> QueryEntryPtr;
typedef std::vector<QueryEntryPtr> QueryEntryPtrVec;
#endif /* PERCONA_PLAYBACK_QUERY_ENTRY_H */
