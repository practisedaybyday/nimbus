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
 * Author: Chinmayee Shah <chinmayee.shah@stanford.edu>
 */

#include "applications/physbam/water//app_utils.h"
#include "applications/physbam/water//job_initialize.h"
#include "applications/physbam/water//physbam_utils.h"
#include "applications/physbam/water//water_driver.h"
#include "applications/physbam/water//water_example.h"
#include "applications/physbam/water//water_sources.h"
#include "applications/physbam/water//job_names.h"
#include "src/shared/dbg.h"
#include "src/shared/nimbus.h"

namespace application {

    JobInitialize::JobInitialize(nimbus::Application *app) {
        set_application(app);
    };

    nimbus::Job* JobInitialize::Clone() {
        return new JobInitialize(application());
    }

    void JobInitialize::Execute(nimbus::Parameter params, const nimbus::DataArray& da) {
        dbg(APP_LOG, "Executing initialize job\n");

        InitConfig init_config;
        init_config.init_phase = true;
        init_config.set_boundary_condition = true;
        init_config.global_region = kDefaultRegion;
        init_config.local_region = kDefaultRegion;
        std::string params_str(params.ser_data().data_ptr_raw(),
            params.ser_data().size());
        LoadParameter(params_str, &init_config);

        PhysBAM::WATER_EXAMPLE<TV> *example;
        PhysBAM::WATER_DRIVER<TV> *driver;

        DataConfig data_config;
        data_config.SetAll();
        InitializeExampleAndDriver(init_config, data_config,
                                   this, da, example, driver);

        // Free resources.
        DestroyExampleAndDriver(example, driver);

        // This job does not save anything to app_data, because it does not use
        // app_data. This job initializes the nimbus objects, and the app_data
        // objects are initialized using the nimbus objects by later jobs.

        dbg(APP_LOG, "Completed executing initialize job\n");
    }

} // namespace application
