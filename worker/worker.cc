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
  * A Nimbus worker. 
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#include "worker/worker.h"

using namespace nimbus; // NOLINT

Worker::Worker(std::string scheduler_ip, port_t scheduler_port,
    port_t listening_port, Application* a)
: scheduler_ip_(scheduler_ip),
  scheduler_port_(scheduler_port),
  listening_port_(listening_port),
  application_(a) {
    log.InitTime();
    id_ = -1;
}

void Worker::Run() {
  std::cout << "Running the Worker" << std::endl;

  SetupSchedulerInterface();
  application_->start(client_);

  SetupDataExchangerInterface();

  WorkerCoreProcessor();
}

void Worker::ProcessSchedulerCommand(SchedulerCommand* cm) {
  std::string command_name = cm->name();

  if (command_name == "spawnjob") {
    SpawnJobCommand* sjc = reinterpret_cast<SpawnJobCommand*>(cm);
    Job * j = application_->cloneJob(sjc->job_name());

    std::vector<Data*> da;
    IDSet<data_id_t>::IDSetIter iter;

    IDSet<data_id_t> read = sjc->read_set();
    for (iter = read.begin(); iter != read.end(); iter++)
      da.push_back(data_map_[*iter]);

    IDSet<data_id_t> write = sjc->write_set();
    for (iter = write.begin(); iter != write.end(); iter++)
      da.push_back(data_map_[*iter]);

    job_id_t id = *(sjc->job_id().begin());

    log.StartTimer();
    j->execute(sjc->params(), da);
    log.StopTimer();

    char buff[MAX_BUFF_SIZE];
    snprintf(buff, sizeof(buff),
        "Execute Job, name: %25s  id: %4ld  length(ms): %6.3lf  time(s): %6.3lf",
        sjc->job_name().c_str(), id, 1000 * log.timer(), log.GetTime());

    log.writeToFile(std::string(buff), LOG_INFO);
  } else if (command_name == "definedata") {
    DefineDataCommand* ddc = reinterpret_cast<DefineDataCommand*>(cm);
    Data * d = application_->cloneData(ddc->data_name());

    data_id_t id = *(ddc->data_id().begin());

    log.StartTimer();
    d->create();
    d->set_id(id);
    AddData(d);
    log.StopTimer();

    char buff[MAX_BUFF_SIZE];
    snprintf(buff, sizeof(buff),
        "Create Data, name: %25s  id: %4ld  length(ms): %6.3lf  time(s): %6.3lf",
        ddc->data_name().c_str(), id, 1000 * log.timer(), log.GetTime());

    log.writeToFile(std::string(buff), LOG_INFO);
  } else if (command_name == "handshake") {
    HandshakeCommand* hsc = reinterpret_cast<HandshakeCommand*>(cm);

    ID<port_t> port(listening_port_);
    HandshakeCommand new_cm = HandshakeCommand(hsc->worker_id(),
        boost::asio::ip::host_name(), port);
    client_->sendCommand(&new_cm);

    id_ = hsc->worker_id().elem();
  } else {
    std::cout << "ERROR: " << cm->toString() <<
      " have not been implemented in ProcessSchedulerCommand yet." <<
      std::endl;
  }
}

void Worker::SetupDataExchangerInterface() {
  data_exchanger_ = new WorkerDataExchanger(listening_port_);
  data_exchanger_thread_ = new boost::thread(
      boost::bind(&WorkerDataExchanger::Run, data_exchanger_));
}

void Worker::SetupSchedulerInterface() {
  LoadSchedulerCommands();
  client_ = new SchedulerClient(scheduler_ip_, scheduler_port_);
  client_->set_scheduler_command_set(&scheduler_command_set_);
  client_->run();
  // client_thread_ = new boost::thread(
  //     boost::bind(&SchedulerClient::run, client_));
}

void Worker::LoadSchedulerCommands() {
  // std::stringstream cms("runjob killjob haltjob resumejob jobdone createdata copydata deletedata");   // NOLINT
  scheduler_command_set_.insert(
      std::make_pair(std::string("spawnjob"), COMMAND_SPAWN_JOB));
  scheduler_command_set_.insert(
      std::make_pair(std::string("definedata"), COMMAND_DEFINE_DATA));
  scheduler_command_set_.insert(
      std::make_pair(std::string("handshake"), COMMAND_HANDSHAKE));
  scheduler_command_set_.insert(
      std::make_pair(std::string("jobdone"), COMMAND_JOB_DONE));
  scheduler_command_set_.insert(
      std::make_pair(std::string("computejob"), COMMAND_COMPUTE_JOB));
  scheduler_command_set_.insert(
      std::make_pair(std::string("createdata"), COMMAND_CREATE_DATA));
  scheduler_command_set_.insert(
      std::make_pair(std::string("remotecopy"), COMMAND_REMOTE_COPY));
  scheduler_command_set_.insert(
      std::make_pair(std::string("localcopy"), COMMAND_LOCAL_COPY));
}

void Worker::AddJob(Job* job) {
  job_map_[job->id()] = job;
}

void Worker::DeleteJob(int id) {
}

void Worker::AddData(Data* data) {
  data_map_[data->id()] = data;
}

void Worker::DeleteData(int id) {
}

worker_id_t Worker::id() {
  return id_;
}

void Worker::set_id(worker_id_t id) {
  id_ = id;
}

