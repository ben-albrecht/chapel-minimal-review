/*
 * Copyright 2004-2016 Cray Inc.
 * Other additional copyright holders may be indicated within.
 * 
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 * 
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// FIFO implementation of Chapel tasking interface
//

#include "chplrt.h"
#include "chpl_rt_utils_static.h"
#include "chplcgfns.h"
#include "chpl-comm.h"
#include "chplexit.h"
#include "chpl-locale-model.h"
#include "chpl-mem.h"
#include "chpl-tasks.h"
#include "chpl-tasks-callbacks-internal.h"
#include "chplsys.h"
#include "chpl-linefile-support.h"
#include "error.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>


//
// task pool: linked list of tasks
//
typedef struct task_pool_struct* task_pool_p;

typedef struct {
  chpl_task_prvData_t prvdata;
} chpl_task_prvDataImpl_t;

typedef struct task_pool_struct {
  chpl_taskID_t    id;           // task identifier
  chpl_fn_p        fun;          // function to call for task
  void*            arg;          // argument to the function
  chpl_bool        is_executeOn; // is this the body of an executeOn?
  int32_t          filename;
  int              lineno;
  chpl_task_prvDataImpl_t chpl_data;
  task_pool_p*     p_list_head;  // task list we're on, if any
  task_pool_p      list_next;    // double-link pointers for list
  task_pool_p      list_prev;
  task_pool_p      next;         // double-link pointers for pool
  task_pool_p      prev;
} task_pool_t;


//
// This is a descriptor for movedTaskWrapper().
//
typedef struct {
  chpl_fn_p fp;
  void* arg;
  chpl_bool countRunning;
  chpl_task_prvDataImpl_t chpl_data;
} movedTaskWrapperDesc_t;


typedef struct lockReport {
  int32_t            filename;
  int                lineno;
  uint64_t           prev_progress_cnt;
  chpl_bool          maybeLocked;
  struct lockReport* next;
} lockReport_t;


// This is the data that is private to each thread.
typedef struct {
  task_pool_p   ptask;
  lockReport_t* lockRprt;
} thread_private_data_t;


static chpl_bool        initialized = false;

static volatile chpl_bool canCountRunningTasks = false;

static chpl_thread_mutex_t threading_lock;     // critical section lock
static chpl_thread_mutex_t extra_task_lock;    // critical section lock
static chpl_thread_mutex_t task_id_lock;       // critical section lock
static chpl_thread_mutex_t task_list_lock;     // critical section lock
static volatile task_pool_p
                           task_pool_head;     // head of task pool
static volatile task_pool_p
                           task_pool_tail;     // tail of task pool

static int                 queued_task_cnt;    // number of tasks in task pool
static int                 running_task_cnt;   // number of running tasks
static int64_t             extra_task_cnt;     // number of tasks being run by
                                               //   threads occupied already
static int                 blocked_thread_cnt; // number of threads that
                                               //   cannot make progress
static int                 idle_thread_cnt;    // number of threads looking
                                               //   for work
static uint64_t            progress_cnt;       // number of unblock operations,
                                               //   as a proxy for progress

static chpl_thread_mutex_t block_report_lock;   // critical section lock
static lockReport_t* lockReportHead = NULL;
static lockReport_t* lockReportTail = NULL;

static chpl_bool do_taskReport = false;
static chpl_thread_mutex_t taskTable_lock;     // critical section lock

static chpl_fn_p comm_task_fn;

//
// Internal functions.
//
static void                    enqueue_task(task_pool_p, task_pool_p*);
static void                    dequeue_task(task_pool_p);
static void                    comm_task_wrapper(void*);
static void                    movedTaskWrapper(void* a);
static chpl_taskID_t           get_next_task_id(void);
static thread_private_data_t*  get_thread_private_data(void);
static task_pool_p             get_current_ptask(void);
static void                    set_current_ptask(task_pool_p);
static void                    report_locked_threads(void);
static void                    report_all_tasks(void);
static void                    SIGINT_handler(int sig);
static void                    initializeLockReportForThread(void);
static chpl_bool               set_block_loc(int, int32_t);
static void                    unset_block_loc(void);
static void                    check_for_deadlock(void);
static void                    thread_begin(void*);
static void                    thread_end(void);
static void                    maybe_add_thread(void);
static task_pool_p             add_to_task_pool(chpl_fn_p, void*, chpl_bool,
                                                chpl_task_prvDataImpl_t,
                                                task_pool_p*, chpl_bool,
                                                int, int32_t);

//
// Condition variable methods
//
static void chpl_thread_condvar_init(chpl_thread_condvar_t* cv);

//
// Sync variable methods
//
static chpl_bool chpl_thread_sync_suspend(chpl_sync_aux_t *s,
                                   struct timeval *deadline);
static void chpl_thread_sync_awaken(chpl_sync_aux_t *s);

// Sync variables

static void sync_wait_and_lock(chpl_sync_aux_t *s,
                               chpl_bool want_full,
                               int32_t lineno, int32_t filename) {
  chpl_bool suspend_using_cond;

  chpl_thread_mutexLock(&s->lock);

  // If we're oversubscribing the hardware, we wait using conditionals
  // in order to ensure fairness and thus progress.  If we're not, we
  // can spin-wait.
  suspend_using_cond = (chpl_thread_getNumThreads() >=
                        chpl_getNumLogicalCpus(true));

  while (s->is_full != want_full) {
    if (!suspend_using_cond) {
      chpl_thread_mutexUnlock(&s->lock);
    }
    if (set_block_loc(lineno, filename)) {
      // all other tasks appear to be blocked
      struct timeval deadline, now;
      chpl_bool timed_out = false;
      // default value so that always allows condition to be true if not
      // using conditionals

      gettimeofday(&deadline, NULL);
      deadline.tv_sec += 1;
      do {
        if (suspend_using_cond)
          timed_out = chpl_thread_sync_suspend(s, &deadline);
        else
          chpl_thread_yield();
        
        if (s->is_full != want_full && !timed_out)
          gettimeofday(&now, NULL);
      } while (s->is_full != want_full
               && !timed_out
               && (now.tv_sec < deadline.tv_sec
                   || (now.tv_sec == deadline.tv_sec
                       && now.tv_usec < deadline.tv_usec)));
      if (s->is_full != want_full)
        check_for_deadlock();
    }
    else {
      do {
        if (suspend_using_cond)
          (void) chpl_thread_sync_suspend(s, NULL);
        else
          chpl_thread_yield();
      } while (s->is_full != want_full);
    }
    unset_block_loc();
    if (!suspend_using_cond)
      chpl_thread_mutexLock(&s->lock);
  }

  if (blockreport)
    progress_cnt++;
}

void chpl_sync_lock(chpl_sync_aux_t *s) {
  chpl_thread_mutexLock(&s->lock);
}

void chpl_sync_unlock(chpl_sync_aux_t *s) {
  chpl_thread_mutexUnlock(&s->lock);
}

void chpl_sync_waitFullAndLock(chpl_sync_aux_t *s,
                                  int32_t lineno, int32_t filename) {
  sync_wait_and_lock(s, true, lineno, filename);
}

void chpl_sync_waitEmptyAndLock(chpl_sync_aux_t *s,
                                   int32_t lineno, int32_t filename) {
  sync_wait_and_lock(s, false, lineno, filename);
}

static chpl_bool chpl_thread_sync_suspend(chpl_sync_aux_t *s,
                                   struct timeval *deadline) {
  chpl_thread_condvar_t* cond;
  cond = s->is_full ? &s->signal_empty : &s->signal_full;

  if (deadline == NULL) {
    (void) pthread_cond_wait(cond, (pthread_mutex_t*) &s->lock);
    return false;
  }
  else {
    struct timespec ts;
    ts.tv_sec  = deadline->tv_sec;
    ts.tv_nsec = deadline->tv_usec * 1000UL;
    return (pthread_cond_timedwait(cond, (pthread_mutex_t*) &s->lock, &ts)
            == ETIMEDOUT);
  }
}

static void chpl_thread_sync_awaken(chpl_sync_aux_t *s) {
  if (pthread_cond_signal(s->is_full ?
                          &s->signal_full : &s->signal_empty))
    chpl_internal_error("pthread_cond_signal() failed");
}

void chpl_sync_markAndSignalFull(chpl_sync_aux_t *s) {
  s->is_full = true;
  chpl_thread_sync_awaken(s);
  chpl_sync_unlock(s);
}

void chpl_sync_markAndSignalEmpty(chpl_sync_aux_t *s) {
  s->is_full = false;
  chpl_thread_sync_awaken(s);
  chpl_sync_unlock(s);
}

chpl_bool chpl_sync_isFull(void *val_ptr,
                            chpl_sync_aux_t *s) {
  return s->is_full;
}

static void chpl_thread_condvar_init(chpl_thread_condvar_t* cv) {
  if (pthread_cond_init((pthread_cond_t*) cv, NULL))
    chpl_internal_error("pthread_cond_init() failed");
}

void chpl_sync_initAux(chpl_sync_aux_t *s) {
  s->is_full = false;
  chpl_thread_mutexInit(&s->lock);
  chpl_thread_condvar_init(&s->signal_full);
  chpl_thread_condvar_init(&s->signal_empty);
}

static void chpl_thread_condvar_destroy(chpl_thread_condvar_t* cv) {
// Leak condvars on cygwin. Some bug results from condvars still being used at
// this point on cygwin. For now, just leak them to avoid errors as a result of
// the undefined behavior of trying to destroy an in-use condvar.
#ifndef __CYGWIN__
  if (pthread_cond_destroy((pthread_cond_t*) cv))
    chpl_internal_error("pthread_cond_destroy() failed");
#endif
}

void chpl_sync_destroyAux(chpl_sync_aux_t *s) {
  chpl_thread_condvar_destroy(&s->signal_full);
  chpl_thread_condvar_destroy(&s->signal_empty);
  chpl_thread_mutexDestroy(&s->lock);
}

// Tasks

void chpl_task_init(void) {
  chpl_thread_mutexInit(&threading_lock);
  chpl_thread_mutexInit(&extra_task_lock);
  chpl_thread_mutexInit(&task_id_lock);
  chpl_thread_mutexInit(&task_list_lock);
  queued_task_cnt = 0;
  running_task_cnt = 1;                     // only main task running
  blocked_thread_cnt = 0;
  idle_thread_cnt = 0;
  extra_task_cnt = 0;
  task_pool_head = task_pool_tail = NULL;

  chpl_thread_init(thread_begin, thread_end);

  //
  // Set main thread private data, so that things that require access
  // to it, like chpl_task_getID() and chpl_task_setSerial(), can be
  // called early (notably during standard module initialization).
  //
  // This needs to be done after the threading layer initialization,
  // because it's based on thread layer capabilities, but before we
  // install the signal handlers, because when those are invoked they
  // may use the thread private data.
  //
  {
    thread_private_data_t* tp;

    tp = (thread_private_data_t*) chpl_mem_alloc(sizeof(thread_private_data_t),
                                                 CHPL_RT_MD_THREAD_PRV_DATA,
                                                 0, 0);

    tp->ptask = (task_pool_p) chpl_mem_alloc(sizeof(task_pool_t),
                                             CHPL_RT_MD_TASK_POOL_DESC,
                                             0, 0);
    tp->ptask->id           = get_next_task_id();
    tp->ptask->fun          = NULL;
    tp->ptask->arg          = NULL;
    tp->ptask->is_executeOn = false;
    tp->ptask->filename     = CHPL_FILE_IDX_MAIN_PROGRAM;
    tp->ptask->lineno       = 0;
    tp->ptask->p_list_head  = NULL;
    tp->ptask->next         = NULL;
    tp->lockRprt            = NULL;

    // Set up task-private data for locale (architectural) support.
    tp->ptask->chpl_data.prvdata.serial_state = true;     // Set to false in chpl_task_callMain().

    chpl_thread_setPrivateData(tp);
  }

  if (blockreport) {
    progress_cnt = 0;
    chpl_thread_mutexInit(&block_report_lock);
    initializeLockReportForThread();
  }

  if (blockreport || taskreport) {
    signal(SIGINT, SIGINT_handler);
  }

  initialized = true;
}


void chpl_task_exit(void) {
  if (!initialized)
    return;

  chpl_thread_exit();
}


void chpl_task_callMain(void (*chpl_main)(void)) {
  chpl_main();
}


void chpl_task_stdModulesInitialized(void) {
  //
  // It's not safe to call the module code to count the main task as
  // running until after the modules have been initialized.
  //
  canCountRunningTasks = true;
  chpl_taskRunningCntInc(0, 0);

  //
  // The task table is implemented in Chapel code in the modules, so
  // we can't use it, and thus can't support task reporting on ^C or
  // deadlock, until the other modules on which it depends have been
  // initialized and the supporting code here is set up.  In this
  // function we're guaranteed that is true, because it is called only
  // after all the standard module initialization is complete.
  //

  //
  // Register this main task in the task table.
  //
  if (taskreport) {
    thread_private_data_t* tp = chpl_thread_getPrivateData();

    chpldev_taskTable_add(tp->ptask->id,
                          tp->ptask->lineno, tp->ptask->filename,
                          (uint64_t) (intptr_t) tp->ptask);
    chpldev_taskTable_set_active(tp->ptask->id);

    chpl_thread_mutexInit(&taskTable_lock);
  }

  //
  // Now we can do task reporting if the user requested it.
  //
  do_taskReport = taskreport;
}


int chpl_task_createCommTask(chpl_fn_p fn, void* arg) {
  comm_task_fn = fn;
  return chpl_thread_createCommThread(comm_task_wrapper, arg);
}


static void comm_task_wrapper(void* arg) {
  thread_private_data_t* tp;

  tp = (thread_private_data_t*) chpl_mem_alloc(sizeof(thread_private_data_t),
                                               CHPL_RT_MD_THREAD_PRV_DATA,
                                               0, 0);

  tp->ptask = (task_pool_p) chpl_mem_alloc(sizeof(task_pool_t),
                                           CHPL_RT_MD_TASK_POOL_DESC,
                                           0, 0);
  tp->ptask->id           = get_next_task_id();
  tp->ptask->fun          = comm_task_fn;
  tp->ptask->arg          = arg;
  tp->ptask->is_executeOn = false;
  tp->ptask->filename     = CHPL_FILE_IDX_COMM_TASK;
  tp->ptask->lineno       = 0;
  tp->ptask->p_list_head  = NULL;
  tp->ptask->next         = NULL;

  //
  // The comm (polling) task shouldn't really need this information.
  //
  tp->ptask->chpl_data.prvdata.serial_state = true;

  tp->lockRprt = NULL;

  chpl_thread_setPrivateData(tp);

  (*comm_task_fn)(arg);
}


//
// Enqueue and dequeue tasks from the pool.
//
static inline
void enqueue_task(task_pool_p ptask, task_pool_p* p_task_list_head) {
  queued_task_cnt++;

  //
  // Add to pool.
  //
  if (task_pool_tail)
    task_pool_tail->next = ptask;
  else
    task_pool_head = ptask;
  ptask->prev = task_pool_tail;
  task_pool_tail = ptask;

  //
  // Add to list, if any.
  //
  if (p_task_list_head == NULL) {
    ptask->p_list_head = NULL;
  }
  else {
    ptask->p_list_head = p_task_list_head;
    ptask->list_next = *p_task_list_head;
    if (*p_task_list_head != NULL)
      (*p_task_list_head)->list_prev = ptask;
    ptask->list_prev = NULL;
    *p_task_list_head = ptask;
  }
}


static inline
void dequeue_task(task_pool_p ptask) {
  assert(queued_task_cnt > 0);
  queued_task_cnt--;

  //
  // Remove from pool.
  //
  if (ptask == task_pool_head) {
    if ((task_pool_head = task_pool_head->next) == NULL)
      task_pool_tail = NULL;
    else
      task_pool_head->prev = NULL;
  }
  else {
    if ((ptask->prev->next = ptask->next) == NULL)
      task_pool_tail = ptask->prev;
    else
      ptask->next->prev = ptask->prev;
  }

  //
  // Remove from list, if on one.
  //
  if (ptask->p_list_head != NULL) {
    if (ptask == *(ptask->p_list_head))
      *(ptask->p_list_head) = ptask->list_next;
    else
      ptask->list_prev->list_next = ptask->list_next;
    if (ptask->list_next != NULL)
      ptask->list_next->list_prev = ptask->list_prev;
  }
}


void chpl_task_addToTaskList(chpl_fn_int_t fid, void* arg,
                             c_sublocid_t subloc,
                             void** p_task_list_void,
                             int32_t task_list_locale,
                             chpl_bool is_begin_stmt,
                             int lineno,
                             int32_t filename) {
  task_pool_p curr_ptask = get_current_ptask();
  chpl_task_prvDataImpl_t chpl_data =
    { .prvdata = { .serial_state = curr_ptask->chpl_data.prvdata.serial_state }
    };

  assert(subloc == 0 || subloc == c_sublocid_any);

  if (chpl_data.prvdata.serial_state) {
    (*chpl_ftable[fid])(arg);
    return;
  }

  // begin critical section
  chpl_thread_mutexLock(&threading_lock);

  if (task_list_locale == chpl_nodeID) {
    (void) add_to_task_pool(chpl_ftable[fid], arg, false, chpl_data,
                            (task_pool_p*) p_task_list_void, is_begin_stmt,
                            lineno, filename);

  }
  else {
    //
    // is_begin_stmt should be true here because if task_list_locale !=
    // chpl_nodeID, then this function could not have been called from
    // the context of a cobegin or coforall statement.
    //
    assert(is_begin_stmt);
    (void) add_to_task_pool(chpl_ftable[fid], arg, false, chpl_data,
                            NULL, true, 0, CHPL_FILE_IDX_UNKNOWN);
  }

  // end critical section
  chpl_thread_mutexUnlock(&threading_lock);
}


void chpl_task_executeTasksInList(void** p_task_list_void) {
  task_pool_p* p_task_list_head = (task_pool_p*) p_task_list_void;
  task_pool_p curr_ptask;
  task_pool_p child_ptask;

  //
  // If we're serial, all the tasks have already been executed.
  //
  if (chpl_task_getSerial())
    return;

  curr_ptask = get_current_ptask();

  while (*p_task_list_head != NULL) {
    chpl_fn_p task_to_run_fun = NULL;

    // begin critical section
    chpl_thread_mutexLock(&threading_lock);

    if ((child_ptask = *p_task_list_head) != NULL) {
      task_to_run_fun = child_ptask->fun;
      dequeue_task(child_ptask);
    }

    // end critical section
    chpl_thread_mutexUnlock(&threading_lock);

    if (task_to_run_fun == NULL)
      continue;

    set_current_ptask(child_ptask);

    // begin critical section
    chpl_thread_mutexLock(&extra_task_lock);

    extra_task_cnt++;

    // end critical section
    chpl_thread_mutexUnlock(&extra_task_lock);

    if (do_taskReport) {
      chpl_thread_mutexLock(&taskTable_lock);
      chpldev_taskTable_set_suspended(curr_ptask->id);
      chpldev_taskTable_set_active(child_ptask->id);
      chpl_thread_mutexUnlock(&taskTable_lock);
    }

    if (blockreport)
      initializeLockReportForThread();

    chpl_task_do_callbacks(chpl_task_cb_event_kind_begin,
                           child_ptask->filename,
                           child_ptask->lineno,
                           child_ptask->id,
                           child_ptask->is_executeOn);

    (*task_to_run_fun)(child_ptask->arg);

    chpl_task_do_callbacks(chpl_task_cb_event_kind_end,
                           child_ptask->filename,
                           child_ptask->lineno,
                           child_ptask->id,
                           child_ptask->is_executeOn);

    if (do_taskReport) {
      chpl_thread_mutexLock(&taskTable_lock);
      chpldev_taskTable_set_active(curr_ptask->id);
      chpldev_taskTable_remove(child_ptask->id);
      chpl_thread_mutexUnlock(&taskTable_lock);
    }

    // begin critical section
    chpl_thread_mutexLock(&extra_task_lock);

    extra_task_cnt--;

    // end critical section
    chpl_thread_mutexUnlock(&extra_task_lock);

    set_current_ptask(curr_ptask);
    chpl_mem_free(child_ptask, 0, 0);

  }
}


void chpl_task_startMovedTask(chpl_fn_p fp,
                              void* a,
                              c_sublocid_t subloc,
                              chpl_taskID_t id,
                              chpl_bool serial_state) {
  movedTaskWrapperDesc_t* pmtwd;
  chpl_task_prvDataImpl_t private = {
    .prvdata = { .serial_state = serial_state } };

  assert(subloc == 0 || subloc == c_sublocid_any);
  assert(id == chpl_nullTaskID);

  pmtwd = (movedTaskWrapperDesc_t*)
          chpl_mem_alloc(sizeof(*pmtwd),
                         CHPL_RT_MD_THREAD_PRV_DATA,
                         0, 0);
  *pmtwd = (movedTaskWrapperDesc_t)
           { fp, a, canCountRunningTasks,
             private };

  // begin critical section
  chpl_thread_mutexLock(&threading_lock);

  (void) add_to_task_pool(movedTaskWrapper, pmtwd, true, pmtwd->chpl_data,
                          NULL, false, 0, CHPL_FILE_IDX_UNKNOWN);

  // end critical section
  chpl_thread_mutexUnlock(&threading_lock);
}


static void movedTaskWrapper(void* a) {
  movedTaskWrapperDesc_t* pmtwd = (movedTaskWrapperDesc_t*) a;
  if (pmtwd->countRunning)
    chpl_taskRunningCntInc(0, 0);
  (pmtwd->fp)(pmtwd->arg);
  if (pmtwd->countRunning)
    chpl_taskRunningCntDec(0, 0);
  chpl_mem_free(pmtwd, 0, 0);
}


//
// chpl_task_getSubloc() is in tasks-fifo.h.
//


//
// chpl_task_setSubloc() is in tasks-fifo.h.
//


//
// chpl_task_getRequestedSubloc() is in tasks-fifo.h.
//


chpl_taskID_t chpl_task_getId(void) {
  return get_current_ptask()->id;
}


void chpl_task_yield(void) {
  chpl_thread_yield();
}


void chpl_task_sleep(double secs) {
  struct timeval deadline;
  struct timeval now;

  //
  // Figure out when this task can proceed again, and until then, keep
  // yielding.
  //
  gettimeofday(&deadline, NULL);
  deadline.tv_usec += (suseconds_t) ((secs - trunc(secs)) * 1.0e6);
  if (deadline.tv_usec > 1000000) {
    deadline.tv_sec++;
    deadline.tv_usec -= 1000000;
  }
  deadline.tv_sec += (time_t) trunc(secs);

  do {
    chpl_task_yield();
    gettimeofday(&now, NULL);
  } while (now.tv_sec < deadline.tv_sec
           || (now.tv_sec == deadline.tv_sec
               && now.tv_usec < deadline.tv_usec));
}

chpl_bool chpl_task_getSerial(void) {
  return get_current_ptask()->chpl_data.prvdata.serial_state;
}

void chpl_task_setSerial(chpl_bool state) {
  get_current_ptask()->chpl_data.prvdata.serial_state = state;
}

uint32_t chpl_task_getMaxPar(void) {
  uint32_t max;
  uint32_t maxThreads;

  //
  // We expect that even if the physical CPUs have multiple hardware
  // threads, cache and pipeline conflicts will typically prevent
  // applications from gaining by using them.  So, we just return the
  // lesser of the number of physical CPUs and whatever the threading
  // layer says it can do.
  //
  max = (uint32_t) chpl_getNumPhysicalCpus(true);
  maxThreads = chpl_thread_getMaxThreads();
  if (maxThreads < max && maxThreads > 0)
    max = maxThreads;
  return max;
}

c_sublocid_t chpl_task_getNumSublocales(void) {
  return 0;
}

chpl_task_prvData_t* chpl_task_getPrvData(void) {
  return & get_current_ptask()->chpl_data.prvdata;
}

size_t chpl_task_getCallStackSize(void) {
  return chpl_thread_getCallStackSize();
}

uint32_t chpl_task_getNumQueuedTasks(void) { return queued_task_cnt; }

uint32_t chpl_task_getNumRunningTasks(void) {
  chpl_internal_error("chpl_task_getNumRunningTasks() called");
  return 1;
}

int32_t  chpl_task_getNumBlockedTasks(void) {
  if (blockreport) {
    int numBlockedTasks;

    // begin critical section
    chpl_thread_mutexLock(&threading_lock);
    chpl_thread_mutexLock(&block_report_lock);

    numBlockedTasks = blocked_thread_cnt - idle_thread_cnt;

    // end critical section
    chpl_thread_mutexUnlock(&block_report_lock);
    chpl_thread_mutexUnlock(&threading_lock);

    assert(numBlockedTasks >= 0);
    return numBlockedTasks;
  }
  else
    return 0;
}


// Internal utility functions for task management

//
// Get a new task ID.
//
static chpl_taskID_t get_next_task_id(void) {
  static chpl_taskID_t       id = chpl_nullTaskID + 1;

  chpl_taskID_t              next_id;

  chpl_thread_mutexLock(&task_id_lock);
  next_id = id++;
  chpl_thread_mutexUnlock(&task_id_lock);

  return next_id;
}


//
// Get the the thread private data pointer for my thread.
//
static thread_private_data_t* get_thread_private_data(void) {
  thread_private_data_t* tp;

  tp = (thread_private_data_t*) chpl_thread_getPrivateData();

  if (tp == NULL)
    chpl_internal_error("no thread private data");

  return tp;
}


//
// Get the descriptor for the task now running on my thread.
//
static task_pool_p get_current_ptask(void) {
  return get_thread_private_data()->ptask;
}


//
// Set the descriptor for the task now running on my thread.
//
static void set_current_ptask(task_pool_p ptask) {
  get_thread_private_data()->ptask = ptask;
}


//
// Walk over the linked list of thread states and print the ones that
// are blocked/waiting.  This is used by both the deadlock reporting
// and the ^C signal handler.
//
static void report_locked_threads(void) {
  lockReport_t* rep;

  fflush(stdout);

  rep = lockReportHead;
  while (rep != NULL) {
    if (rep->maybeLocked) {
      if (rep->lineno > 0 && rep->filename)
        fprintf(stderr, "Waiting at: %s:%d\n",
                chpl_lookupFilename(rep->filename), rep->lineno);
      else if (rep->lineno == 0 && rep->filename == CHPL_FILE_IDX_IDLE_TASK)
        fprintf(stderr, "Waiting for more work\n");
    }
    rep = rep->next;
  }

  fflush(stdout);
}


//
// This signal handler prints an overall task report, containing
// pending tasks and those that are running.
//
static void report_all_tasks(void) {
  task_pool_p pendingTask = task_pool_head;

  printf("Task report\n");
  printf("--------------------------------\n");

  // print out pending tasks
  printf("Pending tasks:\n");
  while (pendingTask != NULL) {
    printf("- %s:%d\n", chpl_lookupFilename(pendingTask->filename),
           (int)pendingTask->lineno);
    pendingTask = pendingTask->next;
  }
  printf("\n");

  // print out running tasks
  printf("Known tasks:\n");
  chpldev_taskTable_print();
}


//
// This is a signal handler that does thread and task reporting.
//
static void SIGINT_handler(int sig) {
  signal(sig, SIG_IGN);

  if (blockreport)
    report_locked_threads();

  if (do_taskReport)
    report_all_tasks();

  chpl_exit_any(1);
}


//
// This function should be called exactly once per thread (not task!),
// including the main thread. It should be called before the first task
// this thread was created to do is started.
//
// Our handling of lock report list entries could be improved.  We
// allocate one each time this function is called, and this is called
// just before each task wrapper is called.  We never remove these
// from the list or deallocate them.  If we do traverse the list while
// reporting a deadlock, we just skip the leaked ones, because they
// don't say "blocked".
//
static void initializeLockReportForThread(void) {
  lockReport_t* newLockReport;

  newLockReport = (lockReport_t*) chpl_mem_alloc(sizeof(lockReport_t),
                                                 CHPL_RT_MD_LOCK_REPORT_DATA,
                                                 0, 0);
  newLockReport->maybeLocked = false;
  newLockReport->next = NULL;

  get_thread_private_data()->lockRprt = newLockReport;

  // Begin critical section
  chpl_thread_mutexLock(&block_report_lock);
  if (lockReportHead) {
    lockReportTail->next = newLockReport;
    lockReportTail = newLockReport;
  } else {
    lockReportHead = newLockReport;
    lockReportTail = newLockReport;
  }
  // End critical section
  chpl_thread_mutexUnlock(&block_report_lock);
}



// Deadlock detection

//
// Inform task management that the thread (task) is about to suspend
// waiting for a sync or single variable to change state or the task
// pool to become nonempty.  The return value is true if the program
// may be deadlocked, indicating that the thread should use a timeout
// deadline on its suspension if possible, and false otherwise.
//
static chpl_bool set_block_loc(int lineno, int32_t filename) {
  thread_private_data_t* tp;
  chpl_bool isLastUnblockedThread;

  if (!blockreport)
    return false;

  isLastUnblockedThread = false;

  tp = get_thread_private_data();
  tp->lockRprt->filename = filename;
  tp->lockRprt->lineno = lineno;
  tp->lockRprt->prev_progress_cnt = progress_cnt;
  tp->lockRprt->maybeLocked = true;

  // Begin critical section
  chpl_thread_mutexLock(&block_report_lock);

  blocked_thread_cnt++;
  if (blocked_thread_cnt >= chpl_thread_getNumThreads()) {
    isLastUnblockedThread = true;
  }

  // End critical section
  chpl_thread_mutexUnlock(&block_report_lock);

  return isLastUnblockedThread;
}


//
// Inform task management that the thread (task) is no longer suspended.
//
static void unset_block_loc(void) {
  if (!blockreport)
    return;

  get_thread_private_data()->lockRprt->maybeLocked = false;

  // Begin critical section
  chpl_thread_mutexLock(&block_report_lock);

  blocked_thread_cnt--;

  // End critical section
  chpl_thread_mutexUnlock(&block_report_lock);
}


//
// Check for and report deadlock, when a suspension deadline passes.
//
static void check_for_deadlock(void) {
  // Blockreport should be true here, because this can't be called
  // unless set_block_loc() returns true, and it can't do that unless
  // blockreport is true.  So this is just a check for ongoing
  // internal consistency.
  assert(blockreport);

  if (get_thread_private_data()->lockRprt->prev_progress_cnt < progress_cnt)
    return;

  fflush(stdout);
  fprintf(stderr, "Program is deadlocked!\n");

  report_locked_threads();

  if (do_taskReport)
    report_all_tasks();

  chpl_exit_any(1);
}


//
// When we create a thread it runs this wrapper function, which just
// executes tasks out of the pool as they become available.
//
static void
thread_begin(void* ptask_void) {
  task_pool_p ptask;
  thread_private_data_t *tp;

  tp = (thread_private_data_t*) chpl_mem_alloc(sizeof(thread_private_data_t),
                                               CHPL_RT_MD_THREAD_PRV_DATA,
                                               0, 0);
  chpl_thread_setPrivateData(tp);

  tp->lockRprt = NULL;
  if (blockreport)
    initializeLockReportForThread();

  while (true) {
    //
    // wait for a task to be present in the task pool
    //

    // In revision 22137, we investigated whether it was beneficial to
    // implement this while loop in a hybrid style, where depending on
    // the number of tasks available, idle threads would either yield or
    // wait on a condition variable to waken them.  Through analysis, we
    // realized this could potential create a case where a thread would
    // become stranded, waiting for a condition signal that would never
    // come.  A potential solution to this was to keep a count of threads
    // that were waiting on the signal, but since there was a performance
    // impact from keeping it as a hybrid as opposed to merely yielding,
    // it was decided that we would return to the simple yield case.
    while (!task_pool_head) {
      if (set_block_loc(0, CHPL_FILE_IDX_IDLE_TASK)) {
        // all other tasks appear to be blocked
        struct timeval deadline, now;
        gettimeofday(&deadline, NULL);
        deadline.tv_sec += 1;
        do {
          chpl_thread_yield();
          if (!task_pool_head)
            gettimeofday(&now, NULL);
        } while (!task_pool_head
                 && (now.tv_sec < deadline.tv_sec
                     || (now.tv_sec == deadline.tv_sec
                         && now.tv_usec < deadline.tv_usec)));
        if (!task_pool_head) {
          check_for_deadlock();
        }
      }
      else {
        do {
          chpl_thread_yield();
        } while (!task_pool_head);
      }

      unset_block_loc();
    }
 
    //
    // Just now the pool had at least one task in it.  Lock and see if
    // there's something still there.
    //
    chpl_thread_mutexLock(&threading_lock);
    if (!task_pool_head) {
      chpl_thread_mutexUnlock(&threading_lock);
      continue;
    }

    //
    // We've found a task to run.
    //

    if (blockreport)
      progress_cnt++;

    //
    // start new task; increment running count and remove task from pool
    // also add to task to task-table (structure in ChapelRuntime that keeps
    // track of currently running tasks for task-reports on deadlock or
    // Ctrl+C).
    //
    ptask = task_pool_head;
    idle_thread_cnt--;
    running_task_cnt++;

    dequeue_task(ptask);

    // end critical section
    chpl_thread_mutexUnlock(&threading_lock);

    tp->ptask = ptask;

    if (do_taskReport) {
      chpl_thread_mutexLock(&taskTable_lock);
      chpldev_taskTable_set_active(ptask->id);
      chpl_thread_mutexUnlock(&taskTable_lock);
    }

    chpl_task_do_callbacks(chpl_task_cb_event_kind_begin,
                           ptask->filename,
                           ptask->lineno,
                           ptask->id,
                           ptask->is_executeOn);

    (*ptask->fun)(ptask->arg);

    chpl_task_do_callbacks(chpl_task_cb_event_kind_end,
                           ptask->filename,
                           ptask->lineno,
                           ptask->id,
                           ptask->is_executeOn);

    if (do_taskReport) {
      chpl_thread_mutexLock(&taskTable_lock);
      chpldev_taskTable_remove(ptask->id);
      chpl_thread_mutexUnlock(&taskTable_lock);
    }

    tp->ptask = NULL;
    chpl_mem_free(ptask, 0, 0);

    // begin critical section
    chpl_thread_mutexLock(&threading_lock);

    //
    // finished task; decrement running count and increment idle count
    //
    assert(running_task_cnt > 0);
    running_task_cnt--;
    idle_thread_cnt++;

    // end critical section
    chpl_thread_mutexUnlock(&threading_lock);
  }
}


//
// When a thread is destroyed it calls this ending function.
//
static void thread_end(void)
{
  thread_private_data_t* tp;

  tp = (thread_private_data_t*) chpl_thread_getPrivateData();
  if (tp != NULL) {
    if (tp->lockRprt != NULL) {
      chpl_mem_free(tp->lockRprt, 0, 0);
      tp->lockRprt = NULL;
    }
    chpl_mem_free(tp, 0, 0);
    chpl_thread_setPrivateData(NULL);
  }
}


//
// Launch another thread, if it seems useful to do so and we can.
//
static void maybe_add_thread(void) {
  static chpl_bool warning_issued = false;

  if (!warning_issued && chpl_thread_canCreate()) {
    if (chpl_thread_create(NULL) == 0) {
      idle_thread_cnt++;
    }
    else {
      int32_t max_threads = chpl_thread_getMaxThreads();
      uint32_t num_threads = chpl_thread_getNumThreads();
      char msg[256];
      if (max_threads)
        sprintf(msg,
                "max threads per locale is %" PRId32
                ", but unable to create more than %d threads",
                max_threads, num_threads);
      else
        sprintf(msg,
                "max threads per locale is unbounded"
                ", but unable to create more than %d threads",
                num_threads);
      chpl_warning(msg, 0, 0);
      warning_issued = true;
    }
  }
}


// create a task from the given function pointer and arguments
// and append it to the end of the task pool
// assumes threading_lock has already been acquired!
static inline
task_pool_p add_to_task_pool(chpl_fn_p fp,
                             void* a,
                             chpl_bool is_executeOn,
                             chpl_task_prvDataImpl_t chpl_data,
                             task_pool_p* p_task_list_head,
                             chpl_bool is_begin_stmt,
                             int lineno, int32_t filename) {
  task_pool_p ptask =
    (task_pool_p) chpl_mem_alloc(sizeof(task_pool_t),
                                        CHPL_RT_MD_TASK_POOL_DESC,
                                        0, 0);
  ptask->id           = get_next_task_id();
  ptask->fun          = fp;
  ptask->arg          = a;
  ptask->is_executeOn = is_executeOn;
  ptask->chpl_data    = chpl_data;
  ptask->filename     = filename;
  ptask->lineno       = lineno;
  ptask->p_list_head  = NULL;
  ptask->next         = NULL;

  enqueue_task(ptask, p_task_list_head);

  chpl_task_do_callbacks(chpl_task_cb_event_kind_create,
                         ptask->filename,
                         ptask->lineno,
                         ptask->id,
                         ptask->is_executeOn);

  if (do_taskReport) {
    chpl_thread_mutexLock(&taskTable_lock);
    chpldev_taskTable_add(ptask->id,
                          ptask->lineno, ptask->filename,
                          (uint64_t) (intptr_t) ptask);
    chpl_thread_mutexUnlock(&taskTable_lock);
  }

  //
  // If we now have more tasks than threads to run them on (taking
  // into account that the current parent of a structured parallel
  // construct can run at least one of that construct's children),
  // try to start another thread.
  //
  if (queued_task_cnt > idle_thread_cnt &&
      (p_task_list_head == NULL || ptask->list_next != NULL || is_begin_stmt)) {
    maybe_add_thread();
  }

  return ptask;
}


// Threads

uint32_t chpl_task_getNumThreads(void) {
  return chpl_thread_getNumThreads();
}

uint32_t chpl_task_getNumIdleThreads(void) {
  return idle_thread_cnt;
}