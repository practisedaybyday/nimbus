/*
 * Copyright 2013 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /*
  * Author: Hang Qu <quhang@stanford.edu>
  */

#include <unistd.h>
#include <fstream>  // NOLINT
#include <string>

#include "worker/worker.h"
#include "worker/worker_manager.h"
#include "worker/worker_thread.h"
#include "worker/worker_thread_monitor.h"

namespace nimbus {

WorkerThreadMonitor::WorkerThreadMonitor(WorkerManager* worker_manager)
    : WorkerThread(worker_manager) {
}

WorkerThreadMonitor::~WorkerThreadMonitor() {
}

void WorkerThreadMonitor::Run() {
  while (true) {
    continue;
  }
  std::ofstream output("worker_state.log");
  int64_t dispatched_computation_job_count_last = 0;
  int64_t dispatched_finish_job_count_last = 0;
  int64_t dispatched_fast_job_count_last = 0;

  int64_t dispatched_computation_job_count;
  int64_t dispatched_finish_job_count;
  int64_t dispatched_fast_job_count;
  int idle_computation_threads;
  int64_t ready_job_queue_length;
  int64_t finish_job_queue_length;
  int64_t fast_job_queue_length;

  output << "dispatched_computation_job_count "
      << "dispatched_finish_job_count "
      << "dispatched_fast_job_count "
      << "working_computation_thread_num "
      << "ready_job_queue_length "
      << "finish_job_queue_length "
      << "fast_job_queue_length "
      << std::endl;
  while (true) {
    usleep(100000);

    dispatched_computation_job_count =
        worker_manager_->dispatched_computation_job_count_;
    dispatched_finish_job_count =
        worker_manager_->dispatched_finish_job_count_;
    dispatched_fast_job_count =
        worker_manager_->dispatched_fast_job_count_;

    idle_computation_threads = worker_manager_->idle_computation_threads_;
    ready_job_queue_length = worker_manager_->ready_jobs_count_;
    // TODO(quhang) this is not synchronized.
    finish_job_queue_length = worker_manager_->finish_job_list_.size();
    fast_job_queue_length = worker_manager_->fast_job_list_.size();

    output << dispatched_computation_job_count
              - dispatched_computation_job_count_last
           << " "
           << dispatched_finish_job_count
              - dispatched_finish_job_count_last
           << " "
           << dispatched_fast_job_count
              - dispatched_fast_job_count_last
           << " "
           << worker_manager_->computation_thread_num - idle_computation_threads
           << " "
           << ready_job_queue_length
           << " "
           << finish_job_queue_length
           << " "
           << fast_job_queue_length
           << std::endl;
    output.flush();

    dispatched_computation_job_count_last =
        dispatched_computation_job_count;
    dispatched_finish_job_count_last =
        dispatched_finish_job_count;
    dispatched_fast_job_count_last =
        dispatched_fast_job_count;
  }
}

}  // namespace nimbus
