/* Copyright 2015 Pierre Ossman for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __OS_MUTEX_H__
#define __OS_MUTEX_H__

namespace os {
  class Condition;

  class Mutex {
  public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

  private:
    friend class Condition;

    void* systemMutex;
  };

  class AutoMutex {
  public:
    AutoMutex(Mutex* mutex) { m = mutex; m->lock(); }
    ~AutoMutex() { m->unlock(); }
  private:
    Mutex* m;
  };

  class Condition {
  public:
    Condition(Mutex* mutex);
    ~Condition();

    void wait();

    void signal();
    void broadcast();

  private:
    Mutex* mutex;
    void* systemCondition;
  };

}

#endif
