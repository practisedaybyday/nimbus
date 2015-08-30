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
  * This is ExecutionTemplate module to hold and instantiate execution template
  * for a worker without building/destroying the execution graph each time.
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SHARED_EXECUTION_TEMPLATE_H_
#define NIMBUS_SHARED_EXECUTION_TEMPLATE_H_

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <iostream> // NOLINT
#include <string>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include "shared/nimbus_types.h"
#include "shared/scheduler_command_include.h"
#include "shared/worker_data_exchanger.h"
#include "shared/dbg.h"
#include "shared/log.h"
#include "worker/job.h"
#include "worker/application.h"

namespace nimbus {

class SchedulerClient;

class ExecutionTemplate {
  public:
    ExecutionTemplate(const std::string& execution_template_name,
                      const std::vector<job_id_t>& inner_job_ids,
                      const std::vector<job_id_t>& outer_job_ids,
                      const std::vector<physical_data_id_t>& phy_ids);

    ~ExecutionTemplate();


    bool finalized();
    size_t job_num();
    size_t copy_job_num();
    size_t compute_job_num();
    std::string execution_template_name();
    template_id_t template_generation_id();


    bool Finalize();

    bool Instantiate(const std::vector<job_id_t>& inner_job_ids,
                     const std::vector<job_id_t>& outer_job_ids,
                     const std::vector<Parameter>& parameters,
                     const std::vector<physical_data_id_t>& physical_ids,
                     const WorkerDataExchanger::EventList& pending_events,
                     const template_id_t& template_generation_id,
                     JobList *ready_jobs);

    bool MarkJobDone(const job_id_t& shadow_job_id,
                     JobList *ready_jobs,
                     bool append);

    void ProcessReceiveEvent(const WorkerDataExchanger::Event& event,
                             JobList *ready_jobs,
                             bool append);

    bool AddComputeJobTemplate(ComputeJobCommand* command,
                               Application *app);

    bool AddLocalCopyJobTemplate(LocalCopyCommand* command,
                                 Application *app);

    bool AddRemoteCopySendJobTemplate(RemoteCopySendCommand* command,
                                      Application *app,
                                      WorkerDataExchanger *dx);


    bool AddRemoteCopyReceiveJobTemplate(RemoteCopyReceiveCommand* command,
                                         Application *app);

    bool AddMegaRCRJobTemplate(MegaRCRCommand* command,
                               Application *app);

  private:
    typedef boost::shared_ptr<job_id_t> JobIdPtr;
    typedef std::vector<JobIdPtr> JobIdPtrList;
    typedef boost::unordered_set<JobIdPtr> JobIdPtrSet;
    typedef boost::unordered_map<job_id_t, JobIdPtr> JobIdPtrMap;

    typedef boost::shared_ptr<physical_data_id_t> PhyIdPtr;
    typedef std::vector<PhyIdPtr> PhyIdPtrList;
    typedef boost::unordered_set<PhyIdPtr> PhyIdPtrSet;
    typedef boost::unordered_map<job_id_t, PhyIdPtr> PhyIdPtrMap;


    enum JobTemplateType {
      BASE,
      COMPUTE,
      LC,
      RCS,
      RCR,
      MEGA_RCR
    };

    class JobTemplate;
    typedef std::vector<JobTemplate*> JobTemplateVector;
    typedef boost::unordered_map<job_id_t, JobTemplate*> JobTemplateMap;

    class JobTemplate {
      public:
        JobTemplate(Job *job,
                    JobIdPtr job_id_ptr,
                    const IDSet<job_id_t>& before_set)
        : job_(job),
          job_id_ptr_(job_id_ptr),
          before_set_(before_set) {
          type_ = BASE;
          dependency_counter_ = 0;
        }
        ~JobTemplate() {}

        Job *job_;
        JobIdPtr job_id_ptr_;
        JobTemplateType type_;
        IDSet<job_id_t> before_set_;
        size_t dependency_num_;
        size_t dependency_counter_;
        JobTemplateVector after_set_job_templates_;

        void ClearAfterSet(JobTemplateVector *ready_list);

        virtual void Refresh(const std::vector<Parameter>& paramerts,
                             const template_id_t& template_generation_id) = 0;
    };

    class ComputeJobTemplate : public JobTemplate {
      public:
        ComputeJobTemplate(Job *job,
                           JobIdPtr job_id_ptr,
                           const PhyIdPtrSet& read_set_ptr,
                           const PhyIdPtrSet& write_set_ptr,
                           const IDSet<job_id_t>& before_set,
                           JobIdPtr future_job_id_ptr,
                           const size_t& param_index)
          : JobTemplate(job, job_id_ptr, before_set),
            read_set_ptr_(read_set_ptr),
            write_set_ptr_(write_set_ptr),
            future_job_id_ptr_(future_job_id_ptr),
            param_index_(param_index) {
              type_ = COMPUTE;
              dependency_num_ = before_set.size();
            }

        ~ComputeJobTemplate() {}

        PhyIdPtrSet read_set_ptr_;
        PhyIdPtrSet write_set_ptr_;
        JobIdPtr future_job_id_ptr_;
        size_t param_index_;

        virtual void Refresh(const std::vector<Parameter> & paramerts,
                             const template_id_t& template_generation_id);
    };

    class LocalCopyJobTemplate : public JobTemplate {
      public:
        LocalCopyJobTemplate(LocalCopyJob *job,
                             JobIdPtr job_id_ptr,
                             PhyIdPtr from_physical_data_id_ptr,
                             PhyIdPtr to_physical_data_id_ptr,
                             const IDSet<job_id_t>& before_set)
          : JobTemplate(job, job_id_ptr, before_set),
            from_physical_data_id_ptr_(from_physical_data_id_ptr),
            to_physical_data_id_ptr_(to_physical_data_id_ptr) {
              type_ = LC;
              dependency_num_ = before_set.size();
            }

        ~LocalCopyJobTemplate() {}

        PhyIdPtr from_physical_data_id_ptr_;
        PhyIdPtr to_physical_data_id_ptr_;

        virtual void Refresh(const std::vector<Parameter> & paramerts,
                             const template_id_t& template_generation_id);
    };


    class RemoteCopySendJobTemplate : public JobTemplate {
      public:
        RemoteCopySendJobTemplate(RemoteCopySendJob *job,
                                  JobIdPtr job_id_ptr,
                                  // JobIdPtr receive_job_id_ptr,
                                  // JobIdPtr mega_rcr_job_id_ptr,
                                  PhyIdPtr from_physical_data_id_ptr,
                                  const IDSet<job_id_t>& before_set)
          : JobTemplate(job, job_id_ptr, before_set),
            // receive_job_id_ptr_(receive_job_id_ptr),
            // mega_rcr_job_id_ptr_(mega_rcr_job_id_ptr),
            from_physical_data_id_ptr_(from_physical_data_id_ptr) {
              type_ = RCS;
              dependency_num_ = before_set.size();
            }

        ~RemoteCopySendJobTemplate() {}

        // JobIdPtr receive_job_id_ptr_;
        // JobIdPtr mega_rcr_job_id_ptr_;
        PhyIdPtr from_physical_data_id_ptr_;

        virtual void Refresh(const std::vector<Parameter> & paramerts,
                             const template_id_t& template_generation_id);
    };

    class RemoteCopyReceiveJobTemplate : public JobTemplate {
      public:
        RemoteCopyReceiveJobTemplate(RemoteCopyReceiveJob *job,
                                     JobIdPtr job_id_ptr,
                                     PhyIdPtr to_physical_data_id_ptr,
                                     const IDSet<job_id_t>& before_set)
          : JobTemplate(job, job_id_ptr, before_set),
            to_physical_data_id_ptr_(to_physical_data_id_ptr) {
              type_ = RCR;
              // +1 for data delivery
              dependency_num_ = before_set.size() + 1;
            }

        ~RemoteCopyReceiveJobTemplate() {}

        PhyIdPtr to_physical_data_id_ptr_;

        virtual void Refresh(const std::vector<Parameter> & paramerts,
                             const template_id_t& template_generation_id);
    };

    class MegaRCRJobTemplate : public JobTemplate {
      public:
        MegaRCRJobTemplate(MegaRCRJob *job,
                           JobIdPtr job_id_ptr,
                           // const JobIdPtrList& receive_job_id_ptrs,
                           const PhyIdPtrList& to_phy_id_ptrs,
                           const IDSet<job_id_t>& before_set)
          : JobTemplate(job, job_id_ptr, before_set),
            // receive_job_id_ptrs_(receive_job_id_ptrs),
            to_phy_id_ptrs_(to_phy_id_ptrs) {
              type_ = MEGA_RCR;
              // + receive_job_id_ptrs.size() for data delivery
              dependency_num_ = before_set.size() + to_phy_id_ptrs.size();
            }

        ~MegaRCRJobTemplate() {}

        // JobIdPtrList receive_job_id_ptrs_;
        PhyIdPtrList to_phy_id_ptrs_;

        virtual void Refresh(const std::vector<Parameter> & paramerts,
                             const template_id_t& template_generation_id);
    };

    bool finalized_;
    size_t copy_job_num_;
    size_t compute_job_num_;
    size_t job_done_counter_;
    std::string execution_template_name_;
    template_id_t template_generation_id_;
    // Currently we do not support future job - omidm
    JobIdPtr future_job_id_ptr_;

    PhyIdPtrMap phy_id_map_;
    PhyIdPtrList phy_id_list_;

    JobIdPtrMap inner_job_id_map_;
    JobIdPtrList inner_job_id_list_;

    JobIdPtrMap outer_job_id_map_;
    JobIdPtrList outer_job_id_list_;

    JobTemplateMap job_templates_;
    JobTemplateVector job_templates_list_;

    JobTemplateVector seed_job_templates_;
    std::vector<Parameter> parameters_;

    mutable boost::recursive_mutex mutex_;


    JobIdPtr GetExistingInnerJobIdPtr(job_id_t job_id);

    PhyIdPtr GetExistingPhyIdPtr(physical_data_id_t pdid);
};

}  // namespace nimbus
#endif  // NIMBUS_SHARED_EXECUTION_TEMPLATE_H_