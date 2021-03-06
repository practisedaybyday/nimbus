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
  * Complex job entry in the job table of the job manager. This job contains a
  * group of compute or explicit copy jobs that are spawned with in a template.
  * The meta data calculated for the template including job dependencies and
  * versioning information is precomputed and is accessible by the complex job.
  * The idea is no avoid expanding the jobs within the template spawning. 
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SRC_SCHEDULER_COMPLEX_JOB_ENTRY_H_
#define NIMBUS_SRC_SCHEDULER_COMPLEX_JOB_ENTRY_H_

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <vector>
#include <string>
#include <set>
#include <list>
#include <utility>
#include <map>
#include "src/shared/nimbus_types.h"
#include "src/shared/idset.h"
#include "src/shared/parameter.h"
#include "src/shared/geometric_region.h"
#include "src/scheduler/job_entry.h"
#include "src/scheduler/shadow_job_entry.h"
#include "src/scheduler/template_job_entry.h"
#include "src/scheduler/version_map.h"
#include "src/scheduler/dense_version_map.h"
#include "src/scheduler/data_manager.h"
#include "src/scheduler/meta_before_set.h"
#include "src/scheduler/logical_data_lineage.h"

namespace nimbus {

class TemplateEntry;

class ComplexJobEntry : public JobEntry {
  public:
    ComplexJobEntry();

    ComplexJobEntry(const job_id_t& job_id,
                    const job_id_t& parent_job_id,
                    TemplateEntry* template_entry,
                    const std::vector<job_id_t>& inner_job_ids,
                    const std::vector<job_id_t>& outer_job_ids,
                    const std::vector<Parameter>& parameters);

    virtual ~ComplexJobEntry();

    TemplateEntry* template_entry() const;
    std::vector<job_id_t> inner_job_ids() const;
    std::vector<job_id_t> outer_job_ids() const;
    std::vector<Parameter> parameters() const;
    const std::vector<job_id_t>* inner_job_ids_p() const;
    const std::vector<job_id_t>* outer_job_ids_p() const;
    const std::vector<Parameter>* parameters_p() const;

    bool cascaded_bound() const;
    bool parent_cascaded_bound() const;
    void set_cascaded_bound(bool flag);
    void set_parent_cascaded_bound(bool flag);

    ShadowJobEntryMap shadow_jobs();
    const ShadowJobEntryMap* shadow_jobs_p();

    size_t GetParentJobIds(std::list<job_id_t>* list);

    size_t GetShadowJobsForAssignment(JobEntryList* list, size_t max_num, bool append = false);

    bool OMIDGetShadowJobEntryByIndex(size_t index, ShadowJobEntry*& shadow_job);

    bool OMIDGetShadowJobEntryById(job_id_t job_id, ShadowJobEntry*& shadow_job);

    size_t OMIDGetParentShadowJobs(ShadowJobEntryList* list, bool append = false);

    bool DrainedAllShadowJobsForAssignment();

    bool ShadowJobContained(job_id_t job_id);

    bool ShadowJobSterile(job_id_t job_id);

    void MarkShadowJobAssigned(job_id_t job_id);

    void MarkShadowJobDone(job_id_t job_id);

    bool ShadowJobAssigned(job_id_t job_id);

    bool ShadowJobDone(job_id_t job_id);

    bool AllShadowJobsAssigned();

    bool AllShadowJobsDone();

    class Cursor {
      public:
        enum State {
          MID_BATCH = 0,
          END_BATCH = 1,
          END_ALL   = 2
        };

        Cursor();
        ~Cursor();

        State state();
        size_t index();
        size_t pivot();

        void set_state(State state);
        void set_index(size_t index);
        void set_pivot(size_t pivot);

      private:
        State state_;
        size_t index_;
        size_t pivot_;
    };

  private:
    typedef boost::unordered_map<job_id_t, size_t> IdMap;
    typedef boost::unordered_set<job_id_t> IdPool;

    TemplateEntry* template_entry_;
    std::vector<job_id_t> inner_job_ids_;
    std::vector<job_id_t> outer_job_ids_;
    std::vector<Parameter> parameters_;

    bool cascaded_bound_;
    bool parent_cascaded_bound_;

    IdMap shadow_job_ids_;
    IdPool assigned_shadow_job_ids_;
    IdPool done_shadow_job_ids_;

    ShadowJobEntryMap shadow_jobs_;
    bool shadow_jobs_complete_;

    std::list<job_id_t> parent_job_ids_;
    std::list<size_t> parent_job_indices_;
    bool parent_job_ids_set_;
    bool parent_job_indices_set_;

    Cursor cursor_;
    bool drained_all_;
    bool initialized_cursor_;

    boost::recursive_mutex mutex_;

    void Initialize();

    void SetParentJobIds();

    void SetParentJobIndices();

    void CompleteShadowJobs();
};

typedef boost::unordered_map<job_id_t, ComplexJobEntry*> ComplexJobEntryMap;

}  // namespace nimbus
#endif  // NIMBUS_SRC_SCHEDULER_COMPLEX_JOB_ENTRY_H_

