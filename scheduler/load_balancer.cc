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
  * This is the base class that serves the scheduler regarding job assignment
  * queries in the cluster. It tries to minimize the completion of simulation
  * by reducing the cost of communication by locality aware data placement and
  * mitigate the effect of stragglers in the system by adapting the job
  * assignment strategies to the dynamic changes of the cloud.
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */


#include "scheduler/load_balancer.h"

#define WEIGHT_NUM 8
#define WEIGHT_X {1, 1, 1, 1, 1, 1, 1, 1}
#define WEIGHT_Y {1, 1, 1, 1, 1, 1, 1, 1}
#define WEIGHT_Z {1, 1, 1, 1, 1, 1, 1, 1}

namespace nimbus {

LoadBalancer::LoadBalancer() {
  Initialize();
}

LoadBalancer::LoadBalancer(ClusterMap* cluster_map) {
  Initialize();
  cluster_map_ = cluster_map;
}

void LoadBalancer::Initialize() {
  worker_num_ = 0;
  global_region_ = GeometricRegion(0, 0, 0, 0, 0, 0);
  update_ = false;
  cluster_map_ = NULL;
  job_manager_ = NULL;
  data_manager_ = NULL;
  log_.set_file_name("load_balancer_log");
}

LoadBalancer::~LoadBalancer() {
}

ClusterMap* LoadBalancer::cluster_map() {
  return cluster_map_;
}

void LoadBalancer::set_cluster_map(ClusterMap* cluster_map) {
  cluster_map_ = cluster_map;
}

void LoadBalancer::set_job_manager(JobManager *job_manager) {
  job_manager_ = job_manager;
}

void LoadBalancer::set_data_manager(DataManager *data_manager) {
  data_manager_ = data_manager;
}

void LoadBalancer::Run() {
  while (true) {
    boost::unique_lock<boost::mutex> update_lock(update_mutex_);
    while (!update_) {
      update_cond_.wait(update_lock);
    }
    InitializeRegionMap();
    UpdateRegionMap();
    update_ = false;
  }
}


bool LoadBalancer::GetWorkerToAssignJob(
    JobEntry *job, SchedulerWorker*& worker) {
  assert(worker_map_.size() > 0);
  // TODO(omidm): Fill out the method.
  return false;
}


void LoadBalancer::NotifyJobAssignment(
    const JobEntry *job, const SchedulerWorker* worker) {
  double time = log_.GetTime();

  if (job->job_type() != JOB_COMP) {
    return;
  }

  JobProfile *job_profile =
    new JobProfile(
        job->job_type(),
        job->job_name(),
        job->job_id(),
        job->parent_job_id(),
        worker->worker_id(),
        job->sterile());

  job_profile->set_assign_time(time);
  job_profile->set_assigned(true);


  Vertex<JobEntry, job_id_t>* vertex;
  job_manager_->job_graph_p()->GetVertex(job->job_id(), &vertex);

  typename Edge<JobEntry, job_id_t>::Iter iter;
  for (iter = vertex->incoming_edges()->begin(); iter != vertex->incoming_edges()->end(); ++iter) {
    JobEntry *j = iter->second->start_vertex()->entry();
    if (!j->done()) {
      job_profile->waiting_set_p()->insert(j->job_id());
    } else {
      /*
      JobHistory::iterator it = job_history_.find(j->job_id());
      if (it != job_history_.end()) {
        JobProfile *jp = it->second;
        job_profile->add_log_entry(
            jp->worker_id(), jp->job_id(), jp->done_time());
      } else {
        dbg(DBG_WARN, "WARNING: Load balancer, could not find done job in job history.");
        exit(-1);
      }
      */
    }
  }

  if (job_profile->waiting_set_p()->size() == 0) {
    job_profile->set_ready_time(time);
    job_profile->set_ready(true);
  }

  job_history_[job->job_id()] = job_profile;
}

void LoadBalancer::NotifyJobDone(const JobEntry *job) {
  double time = log_.GetTime();

  if (job->job_type() != JOB_COMP) {
    return;
  }

  assert(job->done());
  done_jobs_.push_back(job->job_id());

  JobHistory::iterator it = job_history_.find(job->job_id());
  assert(it != job_history_.end());
  JobProfile *job_profile = it->second;

  job_profile->set_done_time(time);
  job_profile->set_done(true);
  job_profile->set_execute_duration(time - job_profile->ready_time());

  Vertex<JobEntry, job_id_t>* vertex;
  job_manager_->job_graph_p()->GetVertex(job->job_id(), &vertex);

  typename Edge<JobEntry, job_id_t>::Iter iter;
  for (iter = vertex->outgoing_edges()->begin(); iter != vertex->outgoing_edges()->end(); ++iter) {
    JobEntry *j = iter->second->end_vertex()->entry();
    it = job_history_.find(j->job_id());
    if (it != job_history_.end()) {
      assert(j->assigned());
      JobProfile *jp = it->second;
      assert(jp->assigned());
      jp->add_log_entry(
          job_profile->worker_id(), job->job_id(), job->job_name(), time);
      jp->waiting_set_p()->remove(job->job_id());
      if (jp->waiting_set_p()->size() == 0) {
        jp->set_ready_time(time);
        jp->set_ready(true);
      }
    }
  }

  log_.WriteToFile(job_profile->Print());
}


void LoadBalancer::NotifyRegisteredWorker(SchedulerWorker *worker) {
  boost::unique_lock<boost::mutex> update_lock(update_mutex_);
  boost::unique_lock<boost::mutex> worker_map_lock(worker_map_mutex_);

  worker_id_t worker_id = worker->worker_id();
  WorkerMapIter iter = worker_map_.find(worker_id);
  if (iter == worker_map_.end()) {
    worker_map_[worker_id] = worker;
    update_ = true;
    update_cond_.notify_all();
  } else {
    dbg(DBG_ERROR, "ERROR: LoadBalancer: worker with the same id %lu has already been registered.\n", // NOLINT
        worker_id);
  }
}

void LoadBalancer::InitializeRegionMap() {
  boost::unique_lock<boost::mutex> worker_map_lock(worker_map_mutex_);
  boost::unique_lock<boost::mutex> region_map_lock(region_map_mutex_);

  if (!data_manager_->initialized_global_bounding_region() ||
      worker_num_ == 0) {
    return;
  }

  if (worker_num_ == worker_map_.size() &&
      global_region_ == data_manager_->global_bounding_region()) {
    return;
  }

  worker_num_ = worker_map_.size();
  global_region_ = data_manager_->global_bounding_region();

  size_t num_x, num_y, num_z;
  SplitDimensions(&num_x, &num_y, &num_z);

  size_t weight_x[WEIGHT_NUM] = WEIGHT_X;
  size_t weight_y[WEIGHT_NUM] = WEIGHT_Y;
  size_t weight_z[WEIGHT_NUM] = WEIGHT_Z;
  GenerateRegionMap(num_x, num_y, num_z, weight_x, weight_y, weight_z);
}

void LoadBalancer::UpdateRegionMap() {
  boost::unique_lock<boost::mutex> worker_map_lock(worker_map_mutex_);
  boost::unique_lock<boost::mutex> region_map_lock(region_map_mutex_);

  return;
}

void LoadBalancer::GenerateRegionMap(size_t num_x, size_t num_y, size_t num_z,
                                     size_t *weight_x, size_t *weight_y, size_t *weight_z) {
  boost::unique_lock<boost::mutex> worker_map_lock(worker_map_mutex_);
  boost::unique_lock<boost::mutex> region_map_lock(region_map_mutex_);

  assert((sizeof(weight_x) / sizeof(weight_x[0])) >= num_x);
  assert((sizeof(weight_y) / sizeof(weight_y[0])) >= num_y);
  assert((sizeof(weight_z) / sizeof(weight_z[0])) >= num_z);

  std::vector<int_dimension_t> width_x;
  size_t weight_sum_x = 0;
  for (size_t i = 0; i < num_x; ++i) {
    weight_sum_x += weight_x[i];
  }
  for (size_t i = 0; i < num_x; ++i) {
    width_x.push_back(global_region_.dx() * weight_x[i] / weight_sum_x);
  }
  std::vector<int_dimension_t> marker_x;
  marker_x.push_back(global_region_.x());
  for (size_t i = 0; i < num_x; ++i) {
    marker_x.push_back(marker_x[i] + width_x[i]);
  }


  std::vector<int_dimension_t> width_y;
  size_t weight_sum_y = 0;
  for (size_t i = 0; i < num_y; ++i) {
    weight_sum_y += weight_y[i];
  }
  for (size_t i = 0; i < num_y; ++i) {
    width_y.push_back(global_region_.dy() * weight_y[i] / weight_sum_y);
  }
  std::vector<int_dimension_t> marker_y;
  marker_y.push_back(global_region_.y());
  for (size_t i = 0; i < num_y; ++i) {
    marker_y.push_back(marker_y[i] + width_y[i]);
  }

  std::vector<int_dimension_t> width_z;
  size_t weight_sum_z = 0;
  for (size_t i = 0; i < num_z; ++i) {
    weight_sum_z += weight_z[i];
  }
  for (size_t i = 0; i < num_z; ++i) {
    width_z.push_back(global_region_.dz() * weight_z[i] / weight_sum_z);
  }
  std::vector<int_dimension_t> marker_z;
  marker_z.push_back(global_region_.z());
  for (size_t i = 0; i < num_z; ++i) {
    marker_z.push_back(marker_z[i] + width_z[i]);
  }

  std::vector<GeometricRegion> domains;
  for (size_t i = 0; i < num_x; ++i) {
    for (size_t j = 0; j < num_y; ++j) {
      for (size_t k = 0; k < num_z; ++k) {
        domains.push_back(
            GeometricRegion(
              marker_x[i],
              marker_y[j],
              marker_z[k],
              marker_x[i + 1] - marker_x[i],
              marker_y[j + 1] - marker_y[j],
              marker_z[k + 1] - marker_z[k]));
      }
    }
  }

  region_map_.clear();
  size_t index = 0;
  assert(domains.size() == worker_map_.size());
  WorkerMapIter iter = worker_map_.begin();
  for (; iter != worker_map_.end(); ++iter) {
    region_map_[iter->first] = domains[index];
    ++index;
  }
}

void LoadBalancer::SplitDimensions(size_t *num_x, size_t *num_y, size_t *num_z) {
  switch (worker_num_) {
    case 1 :
      *num_x = 1;
      *num_y = 1;
      *num_z = 1;
      break;
    case 2 :
      *num_x = 2;
      *num_y = 1;
      *num_z = 1;
      break;
    case 3 :
      *num_x = 3;
      *num_y = 1;
      *num_z = 1;
      break;
    case 4 :
      *num_x = 4;
      *num_y = 1;
      *num_z = 1;
      break;
    case 5 :
      *num_x = 5;
      *num_y = 1;
      *num_z = 1;
      break;
    case 6 :
      *num_x = 6;
      *num_y = 1;
      *num_z = 1;
      break;
    case 7 :
      *num_x = 7;
      *num_y = 1;
      *num_z = 1;
      break;
    case 8 :
      *num_x = 8;
      *num_y = 1;
      *num_z = 1;
      break;
    default:
      dbg(DBG_ERROR, "ERROR: Do not know how to split!");
      exit(-1);
  }
}

}  // namespace nimbus
