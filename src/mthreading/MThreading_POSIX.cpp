/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* MThreading_POSIX.cpp:
**  Copyright (C) 2018 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 TODO: Monotonic time for WaitSemTimeout()

 TODO: Make thread ID correct for threads created outside
       of Mednafen's framework.
*/

#include <mednafen/types.h>
#include <mednafen/driver.h>
#include <mednafen/MThreading.h>

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

namespace Mednafen
{
namespace MThreading
{
static __thread uintptr_t LocalThreadID = 0;

static INLINE void TimeSpec_AddNanoseconds(struct timespec* ts, uint64 add_ns)
{
 uint64 ns = (uint64)ts->tv_nsec + add_ns;

 ts->tv_sec += ns / (1000 * 1000 * 1000);
 ts->tv_nsec = ns % (1000 * 1000 * 1000);
}

struct Thread
{
 Thread() { }

 pthread_t t;
 int (*ep)(void*);
 void* data;
 int rv;
 private:
 Thread(const Thread&);
};

struct Mutex
{
 Mutex() { }

 pthread_mutex_t m;
 private:
 Mutex(const Mutex&);
};

struct Cond
{
 Cond() { }

 pthread_cond_t c;
 private:
 Cond(const Cond&);
};

struct Sem
{
 Sem() { }

 sem_t s;
 private:
 Sem(const Sem&);
};

static void* PTCEP(void* arg)
{
 Thread* t = (Thread*)arg;

 LocalThreadID = (uintptr_t)t;

 //printf("%016llx\n", (unsigned long long)ThreadID);

 t->rv = t->ep(t->data);

 return &t->rv;
}

Thread* CreateThread(int (*fn)(void *), void *data, const char* debug_name)
{
 try
 {
  std::unique_ptr<Thread> ret(new Thread());
  int en;

  ret->ep = fn;
  ret->data = data;

  if((en = pthread_create(&ret->t, nullptr, PTCEP, ret.get())))
   throw MDFN_Error(0, _("pthread_create() failed: %d"), en);

  if(debug_name)
  {
#ifdef HAVE_PTHREAD_SETNAME_NP
   char tmp[16];

   strncpy(tmp, debug_name, 16);
   tmp[15] = 0;

   pthread_setname_np(ret->t, tmp);
#endif
  }

  return ret.release();
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
  return nullptr;
 }
}

void WaitThread(Thread* thread, int* status)
{
 try
 {
  void* vp;

  if(pthread_join(thread->t, &vp))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_join() failed: %s"), ene.StrError());
  }

  if(vp != &thread->rv)
  {
   delete thread;
   throw MDFN_Error(0, _("Thread being joined exited improperly."));
  }

  if(status)
   *status = thread->rv;

  delete thread;
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
 }
}

uintptr_t ThreadID(void)
{
 //printf("%16llx -- %16llx -- %16llx\n", (unsigned long long)ThreadID, (unsigned long long)pthread_self(), (unsigned long long)SDL_ThreadID());
 return LocalThreadID;
}

Mutex* CreateMutex(void)
{
 try
 {
  std::unique_ptr<Mutex> ret(new Mutex);
  pthread_mutexattr_t attr;

  if(pthread_mutexattr_init(&attr))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_mutexattr_init() failed: %s"), ene.StrError());
  }

  if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK/*PTHREAD_MUTEX_NORMAL*/))
  {
   ErrnoHolder ene(errno);

   pthread_mutexattr_destroy(&attr);

   throw MDFN_Error(ene.Errno(), _("pthread_mutexattr_settype() failed: %s"), ene.StrError());
  }

  if(pthread_mutex_init(&ret->m, &attr))
  {
   ErrnoHolder ene(errno);

   pthread_mutexattr_destroy(&attr);

   throw MDFN_Error(ene.Errno(), _("pthread_mutex_init() failed: %s"), ene.StrError());
  }

  pthread_mutexattr_destroy(&attr);
  //
  //
  //
  if(pthread_mutex_lock(&ret->m))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_mutex_lock() failed: %s"), ene.StrError());
  }

  if(pthread_mutex_unlock(&ret->m))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_mutex_unlock() failed: %s"), ene.StrError());
  }

  return ret.release();
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
  return nullptr;
 }
}

void DestroyMutex(Mutex* mutex)
{
 try
 {
  if(pthread_mutex_destroy(&mutex->m))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_mutex_destroy() failed: %s"), ene.StrError());
  }

  delete mutex;
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
 }
}

int LockMutex(Mutex *mutex)
{
 if(pthread_mutex_lock(&mutex->m))
 {
  fprintf(stderr, "pthread_mutex_lock() failed: %m\n");
  return -1;
 }

 return 0;
}

int UnlockMutex(Mutex *mutex)
{
 if(pthread_mutex_unlock(&mutex->m))
 {
  fprintf(stderr, "pthread_mutex_unlock() failed: %m\n");
  return -1;
 }

 return 0;
}

Cond* CreateCond(void)
{
 try
 {
  std::unique_ptr<Cond> ret(new Cond);
  pthread_condattr_t attr;

  if(pthread_condattr_init(&attr))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_condattr_init() failed: %s"), ene.StrError());
  }

  if(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC))
  {
   ErrnoHolder ene(errno);

   pthread_condattr_destroy(&attr);

   throw MDFN_Error(ene.Errno(), _("pthread_condattr_setclock() failed: %s"), ene.StrError());
  }

  if(pthread_cond_init(&ret->c, &attr))
  {
   ErrnoHolder ene(errno);

   pthread_condattr_destroy(&attr);

   throw MDFN_Error(ene.Errno(), _("pthread_cond_init() failed: %s"), ene.StrError());
  }

  pthread_condattr_destroy(&attr);

  return ret.release();
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
  return nullptr;
 }
}

void DestroyCond(Cond* cond)
{
 try
 {
  if(pthread_cond_destroy(&cond->c))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("pthread_cond_destroy() failed: %s"), ene.StrError());
  }

  delete cond;
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
 }
}

int SignalCond(Cond* cond)
{
 if(pthread_cond_signal(&cond->c))
 {
  fprintf(stderr, "pthread_cond_signal() failed: %m\n");
  return -1;
 }

 return 0;
}

int WaitCond(Cond* cond, Mutex* mutex)
{
 if(pthread_cond_wait(&cond->c, &mutex->m))
 {
  fprintf(stderr, "pthread_cond_wait() failed: %m\n");
  return -1;
 }

 return 0;
}

int WaitCondTimeout(Cond* cond, Mutex* mutex, unsigned ms)
{
 struct timespec abstime;

 memset(&abstime, 0, sizeof(abstime));

 if(clock_gettime(CLOCK_MONOTONIC, &abstime))
 {
  fprintf(stderr, "clock_gettime() failed: %m\n");
  return -1;
 }

 TimeSpec_AddNanoseconds(&abstime, (uint64)ms * 1000 * 1000);

 if(pthread_cond_timedwait(&cond->c, &mutex->m, &abstime))
 {
  fprintf(stderr, "pthread_cond_timedwait() failed: %m\n");
  return -1;
 }

 return 0;
}


Sem* CreateSem(void)
{
 try
 {
  std::unique_ptr<Sem> ret(new Sem);

  if(sem_init(&ret->s, 0, 0))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("sem_init() failed: %s"), ene.StrError());
  }

  return ret.release();
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
  return nullptr;
 }
}

void DestroySem(Sem* sem)
{
 try
 {
  if(sem_destroy(&sem->s))
  {
   ErrnoHolder ene(errno);

   throw MDFN_Error(ene.Errno(), _("sem_destroy() failed: %s"), ene.StrError());
  }

  delete sem;
 }
 catch(std::exception& e)
 {
  MDFND_OutputNotice(MDFN_NOTICE_ERROR, e.what());
 }
}

int WaitSem(Sem* sem)
{
 tryagain:
 if(sem_wait(&sem->s))
 {
  if(errno == EINTR)
   goto tryagain;

  fprintf(stderr, "sem_wait() failed: %m");
  return -1;
 }

 return 0;
}

int WaitSemTimeout(Sem* sem, unsigned ms)
{
 struct timespec abstime;

 memset(&abstime, 0, sizeof(abstime));

 if(clock_gettime(/*CLOCK_MONOTONIC*/CLOCK_REALTIME, &abstime))
 {
  fprintf(stderr, "clock_gettime() failed: %m");
  return -1;
 }

 TimeSpec_AddNanoseconds(&abstime, (uint64)ms * 1000 * 1000);

 tryagain:
 /*if(sem_timedwait_monotonic(&sem->s, &abstime))*/
 if(sem_timedwait(&sem->s, &abstime))
 {
  if(errno == EINTR)
   goto tryagain;

  if(errno == ETIMEDOUT)
   return SEM_TIMEDOUT;
  //
  fprintf(stderr, "sem_timedwait() failed: %m");
  return -1;
 }

 return 0;
}

int PostSem(Sem* sem)
{
 if(sem_post(&sem->s))
 { 
  fprintf(stderr, "sem_post() failed: %m");
  return -1;
 }

 return 0;
}

}
}
