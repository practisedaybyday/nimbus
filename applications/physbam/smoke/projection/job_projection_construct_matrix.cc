/* Copyright 2013 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 vd* are met:
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

#include <sstream>
#include <string>

#include "applications/physbam/smoke/app_utils.h"
#include "applications/physbam/smoke/physbam_utils.h"
#include "applications/physbam/smoke/smoke_driver.h"
#include "applications/physbam/smoke/smoke_example.h"
#include "src/shared/dbg.h"
#include "src/shared/nimbus.h"

#include "applications/physbam/smoke/projection/job_projection_construct_matrix.h"

namespace application {

JobProjectionConstructMatrix::JobProjectionConstructMatrix(nimbus::Application *app) {
  set_application(app);
};

nimbus::Job* JobProjectionConstructMatrix::Clone() {
  return new JobProjectionConstructMatrix(application());
}

void JobProjectionConstructMatrix::Execute(
    nimbus::Parameter params,
    const nimbus::DataArray& da) {
  dbg(APP_LOG, "Executing PROJECTION_CONSTRUCT_MATRIX job.\n");

  InitConfig init_config;
  init_config.set_boundary_condition = false;
  T dt;
  std::string params_str(params.ser_data().data_ptr_raw(),
                         params.ser_data().size());
  LoadParameter(params_str, &init_config.frame, &init_config.time, &dt,
                &init_config.global_region, &init_config.local_region);

  // Assume time, dt, frame is ready from here.
  dbg(APP_LOG,
      "In PROJECTION_CONSTRUCT_MATRIX: Initialize SMOKE_DRIVER/SMOKE_EXAMPLE"
      "(Frame=%d, Time=%f).\n",
      init_config.frame, init_config.time);

  PhysBAM::SMOKE_EXAMPLE<TV> *example;
  PhysBAM::SMOKE_DRIVER<TV> *driver;

  DataConfig data_config;
  data_config.SetFlag(DataConfig::PSI_N);
  data_config.SetFlag(DataConfig::PSI_D);
  data_config.SetFlag(DataConfig::REGION_COLORS);
  data_config.SetFlag(DataConfig::PRESSURE);
  data_config.SetFlag(DataConfig::DIVERGENCE);
  data_config.SetFlag(DataConfig::U_INTERFACE);
  data_config.SetFlag(DataConfig::MATRIX_A);
  data_config.SetFlag(DataConfig::VECTOR_B);
  data_config.SetFlag(DataConfig::INDEX_M2C);
  data_config.SetFlag(DataConfig::INDEX_C2M);
  data_config.SetFlag(DataConfig::PROJECTION_LOCAL_N);
  data_config.SetFlag(DataConfig::PROJECTION_INTERIOR_N);
  data_config.SetFlag(DataConfig::PROJECTION_LOCAL_TOLERANCE);

  {
    application::ScopeTimer scope_timer(name() + "-load");
    InitializeExampleAndDriver(init_config, data_config,
			       this, da, example, driver);
  }

  dbg(APP_LOG, "Job PROJECTION_CONSTRUCT_MATRIX starts (dt=%f).\n", dt);

  // TODO(quhang), write to LOCAL_N, INTERIOR_N.
  {
    application::ScopeTimer scope_timer(name());
    driver->ProjectionConstructMatrixImpl(this, da, dt);
  }

  {
    application::ScopeTimer scope_timer(name() + "-save");
    example->Save_To_Nimbus(this, da, driver->current_frame + 1);
    // Free resources.
    DestroyExampleAndDriver(example, driver);
  }

  dbg(APP_LOG, "Completed executing PROJECTION_CONSTRUCT_MATRIX job\n");
}

}  // namespace application
