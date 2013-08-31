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
  * Nimbus abstraction of an application. 
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_WORKER_WORKER_H_
#define NIMBUS_WORKER_WORKER_H_

#include <boost/thread.hpp>
#include <string>
#include <vector>
#include <map>
#include "worker/data.h"
#include "worker/job.h"
#include "worker/application.h"
#include "shared/nimbus_types.h"
#include "shared/scheduler_client.h"
#include "shared/scheduler_command.h"
#include "shared/worker_data_exchanger.h"
#include "shared/cluster.h"
#include "shared/parser.h"
#include "shared/log.h"

namespace nimbus {

class Worker;
typedef std::map<int, Worker*> WorkerMap;

class Worker {
 public:
  Worker(std::string scheuler_ip, port_t scheduler_port,
      port_t listening_port_, Application* application);

  virtual void Run();
  virtual void WorkerCoreProcessor() {}
  virtual void ProcessSchedulerCommand(SchedulerCommand* command);

  worker_id_t id();
  void set_id(worker_id_t id);

 protected:
  SchedulerClient* client_;
  WorkerDataExchanger* data_exchanger_;
  CommandSet scheduler_command_set_;
  worker_id_t id_;
  std::string scheduler_ip_;
  port_t scheduler_port_;
  port_t listening_port_;

 private:
  Log log;
  Computer host_;
  boost::thread* client_thread_;
  boost::thread* data_exchanger_thread_;
  DataMap data_map_;
  JobMap job_map_;
  Application* application_;

  virtual void SetupSchedulerInterface();

  virtual void SetupDataExchangerInterface();

  virtual void AddJob(Job* job);
  virtual void DeleteJob(int id);
  virtual void DeleteJob(Job* job) {}
  virtual void AddData(Data* data);
  virtual void DeleteData(int id);
  virtual void DeleteData(Data* data) {}
  virtual void LoadSchedulerCommands();
};

}  // namespace nimbus

#endif  // NIMBUS_WORKER_WORKER_H_