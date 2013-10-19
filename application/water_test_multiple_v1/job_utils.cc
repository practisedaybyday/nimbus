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
 * Author: Chinmayee Shah <chinmayee.shah@stanford.edu>
 */

#include "app_config.h"
#include "data_face_arrays.h"
#include "job_utils.h"
#include "shared/nimbus.h"
#include "water_data_driver.h"
#include "water_driver.h"

namespace water_app_job {

    JobData::JobData() :
        driver(0),
        sim_data(0) {
        }

    void SimJob::CollectData(const ::nimbus::DataArray& da, JobData& job_data) {
        unsigned int data_num = da.size();
        for (unsigned int i = 0; i < data_num; i++) {
            switch (da[i]->get_debug_info()) {
                case driver_id:
                    job_data.driver = (WaterDriver<TV> *)da[i];
                    break;
                case non_adv_id:
                    job_data.sim_data = (NonAdvData<TV, T> *)da[i];
                    break;
                case face_array_id:
                    FaceArray *vel = (FaceArray * )da[i];
                    if (vel->region() == ::water_app_data::kDataInterior)
                        job_data.central_vels.push_back(vel);
                    else
                        job_data.boundary_vels.push_back(vel);
                    break;
            }
        }
    }

} // namespace water_app_job