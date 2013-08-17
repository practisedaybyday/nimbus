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
 * Helper functions in water_driver.
 *
 * Author: Chinmayee Shah <chinmayee.shah@stanford.edu>
 */

#include <iostream>
#include "assert.h"
#include "shared/nimbus.h"
#include "./water_data_types.h"

using namespace PhysBAM;
using nimbus::Data;

// TODO(someone): right now, create may/ may not allocate required amount of
// memory. Will have to dig deep into PhysBAM to allocate required amount of
// memory at the beginning itself.

template <class TV> FaceArray<TV>::
FaceArray(int size)
{
    id_debug = face_array_id;
    this->size_ = size;
    grid = NULL;
    data = NULL;
}

template <class TV> void FaceArray<TV>::
create()
{
    std::cout << "Creating FaceArray\n";

    typedef typename TV::template REBIND<int>::TYPE TV_INT;
    grid = new GRID<TV> (TV_INT::All_Ones_Vector()*size_,
            RANGE<TV>::Unit_Box(), true);
    assert(grid);

    data = new  ARRAY<T,FACE_INDEX<TV::dimension> >();
    assert(data);
    data->Resize(*grid);
}

template <class TV> Data* FaceArray<TV>::
clone()
{
    std::cout << "Cloning facearray\n";
    return new FaceArray<TV>(size_);
}

template <class TV>
int FaceArray<TV> :: get_debug_info()
{
    return id_debug;
}

template <class TV> FaceArrayGhost<TV>::
FaceArrayGhost(int size)
{
    id_debug = face_array_ghost_id;
    this->size_ = size;
    grid = NULL;
    data = NULL;
}

template <class TV> void FaceArrayGhost<TV>::
create()
{
    std::cout << "Creating FaceArrayGhost\n";

    typedef typename TV::template REBIND<int>::TYPE TV_INT;
    grid = new GRID<TV> (TV_INT::All_Ones_Vector()*size_,
            RANGE<TV>::Unit_Box(), true);
    assert(grid);

    data = new typename GRID_ARRAYS_POLICY<GRID<TV> >::
        FACE_ARRAYS();
    assert(data);
}

template <class TV> Data* FaceArrayGhost<TV>::
clone()
{
    std::cout << "Cloning facearrayghost\n";
    return new FaceArrayGhost<TV>(size_);
}

template <class TV>
int FaceArrayGhost<TV> :: get_debug_info()
{
    return id_debug;
}

template <class TV, class T> NonAdvData<TV, T>::
NonAdvData(int size)
{
    id_debug = non_adv_id;

    this->size_ = size;

    number_of_ghost_cells = 3;
    time = (T)0;
    current_frame = 0;

    grid = NULL;

    boundary_scalar = NULL;
    boundary = NULL;
    phi_boundary = NULL;
    phi_boundary_water = NULL;
    domain_boundary = NULL;

    sources = NULL;

    particle_levelset_evolution = NULL;
    advection_scalar = NULL;

    collision_bodies_affecting_fluid = NULL;

    projection = NULL;
    incompressible = NULL;
}

template <class TV, class T> void NonAdvData<TV, T>::
create()
{
    std::cout << "Creating NonAdvData\n";

    typedef typename TV::template REBIND<int>::TYPE TV_INT;
    grid = new GRID<TV> (TV_INT::All_Ones_Vector()*size_,
            RANGE<TV>::Unit_Box(), true);
    assert(grid);

    boundary_scalar = new BOUNDARY_UNIFORM<GRID<TV>, T>();
    phi_boundary_water = new
        typename GEOMETRY_BOUNDARY_POLICY<GRID<TV> >::
        BOUNDARY_PHI_WATER();
    domain_boundary = new VECTOR<VECTOR<bool,2>,TV::dimension>();

    sources = new ARRAY<IMPLICIT_OBJECT<TV> *>();

    particle_levelset_evolution = new
        PARTICLE_LEVELSET_EVOLUTION_UNIFORM<GRID<TV> >
        (*grid, number_of_ghost_cells);
    advection_scalar = new ADVECTION_SEMI_LAGRANGIAN_UNIFORM<GRID<TV>,T>();

    collision_bodies_affecting_fluid = new
        typename COLLISION_GEOMETRY_COLLECTION_POLICY<GRID<TV> >::
        GRID_BASED_COLLISION_GEOMETRY(*grid);

    projection = new PROJECTION_DYNAMICS_UNIFORM< GRID<TV> >
        (*grid, false, false, false, false, NULL);
    incompressible = new INCOMPRESSIBLE_UNIFORM<GRID<TV> >(*grid, *projection);
}

template <class TV, class T> Data* NonAdvData<TV, T>::
clone()
{
    std::cout << "Cloning nonadvdata\n";
    return new NonAdvData<TV, T>(size_);
}

template <class TV, class T>
int NonAdvData<TV, T> :: get_debug_info()
{
    return id_debug;
}

template <class TV, class T> bool NonAdvData<TV, T>::
initialize
(WaterDriver<TV> *driver, FaceArray<TV> *face_velocities, const int frame)
{
    std::cout << "Initializaing non advection data ...\n";

    typedef typename TV::template REBIND<int>::TYPE TV_INT;

    current_frame = frame;
    time = driver->Time_At_Frame(frame);

    driver->current_frame = current_frame;
    driver->output_number = current_frame;
    driver->time = time;

    for(int i=1;i<=TV::dimension;i++)
    {
        (*domain_boundary)(i)(1)=true;
        (*domain_boundary)(i)(2)=true;
    }
    (*domain_boundary)(2)(2)=false;

    phi_boundary_water->Set_Velocity_Pointer(*face_velocities->data);

    VECTOR<VECTOR<bool,2>,TV::dimension> domain_open_boundaries = 
        VECTOR_UTILITIES::Complement(*domain_boundary);

    phi_boundary = phi_boundary_water;
    phi_boundary->Set_Constant_Extrapolation(domain_open_boundaries);

    boundary = boundary_scalar;
    boundary->Set_Constant_Extrapolation(domain_open_boundaries);

    std::cout << "Moving to incompressible ...\n";

    incompressible->Initialize_Grids(*grid);
    incompressible->Set_Custom_Advection(*advection_scalar);
    incompressible->Set_Custom_Advection(*advection_scalar);
    incompressible->Set_Custom_Boundary(*boundary);
    incompressible->projection.elliptic_solver->Set_Relative_Tolerance(1e-8);
    incompressible->projection.elliptic_solver->pcg.Set_Maximum_Iterations(40);
    incompressible->projection.elliptic_solver->pcg.evolution_solver_type =
        krylov_solver_cg;
    incompressible->projection.elliptic_solver->pcg.cg_restart_iterations=0;
    incompressible->projection.elliptic_solver->pcg.Show_Results();
    incompressible->projection.collidable_solver->Use_External_Level_Set
        (particle_levelset_evolution->particle_levelset.levelset);
    //add forces
    incompressible->Set_Gravity();
    incompressible->Set_Body_Force(true);
    incompressible->projection.Use_Non_Zero_Divergence(false);
    incompressible->projection.elliptic_solver->Solve_Neumann_Regions(true);
    incompressible->projection.elliptic_solver->solve_single_cell_neumann_regions=false;
    incompressible->Use_Explicit_Part_Of_Implicit_Viscosity(false);
    incompressible->Set_Maximum_Implicit_Viscosity_Iterations(40);
    incompressible->Use_Variable_Vorticity_Confinement(false);
    incompressible->Set_Surface_Tension(0);
    incompressible->Set_Variable_Surface_Tension(false);
    incompressible->Set_Viscosity(0);
    incompressible->Set_Variable_Viscosity(false);
    incompressible->projection.Set_Density(1e3);

    std::cout << "Moving to collision bodies affecting fluid ...\n";

    collision_bodies_affecting_fluid->Initialize_Grids();
    collision_bodies_affecting_fluid->Update_Intersection_Acceleration_Structures(false);
    collision_bodies_affecting_fluid->Rasterize_Objects();
    collision_bodies_affecting_fluid->Compute_Occupied_Blocks
        (false, (T)2*grid->Minimum_Edge_Length(), 5);
    collision_bodies_affecting_fluid->Compute_Grid_Visibility();

    std::cout << "Moving to particle levelset evolution ...\n";

    particle_levelset_evolution->Initialize_Domain(*grid);
    particle_levelset_evolution->particle_levelset.Set_Band_Width(6);
    particle_levelset_evolution->Set_Time(time);
    particle_levelset_evolution->Set_CFL_Number((T).9);
    particle_levelset_evolution->Levelset_Advection(1).
        Set_Custom_Advection(*advection_scalar);
    particle_levelset_evolution->Set_Number_Particles_Per_Cell(16);
    particle_levelset_evolution->Set_Levelset_Callbacks(*driver);
    particle_levelset_evolution->Initialize_FMM_Initialization_Iterative_Solver(true);
    particle_levelset_evolution->particle_levelset.levelset.
        Set_Custom_Boundary(*phi_boundary);
    particle_levelset_evolution->Bias_Towards_Negative_Particles(false);
    particle_levelset_evolution->particle_levelset.Use_Removed_Positive_Particles();
    particle_levelset_evolution->particle_levelset.Use_Removed_Negative_Particles();
    particle_levelset_evolution->particle_levelset.Store_Unique_Particle_Id();
    particle_levelset_evolution->Use_Particle_Levelset(true);
    particle_levelset_evolution->particle_levelset.levelset.
        Set_Collision_Body_List(*collision_bodies_affecting_fluid);
    particle_levelset_evolution->particle_levelset.levelset.
        Set_Face_Velocities_Valid_Mask(&incompressible->valid_mask);
    particle_levelset_evolution->particle_levelset.Set_Collision_Distance_Factors(.1,1);

    Initialize_Phi();

    std::cout << "Initialized phi ...\n";

    std::cout << "Just before adjust phi with sources ...\n";
    Adjust_Phi_With_Sources(time);
    std::cout << "After adjust phi with sources ...\n";

    std::cout << "1\n";
    particle_levelset_evolution->Make_Signed_Distance();
    std::cout << "2\n";
    particle_levelset_evolution->Set_Seed(2606);
    std::cout << "3\n";
    particle_levelset_evolution->Seed_Particles(time);
    std::cout << "4\n";
    particle_levelset_evolution->Delete_Particles_Outside_Grid();
    std::cout << "5\n";

    std::cout << "Extrapolate etc ...\n";

    ARRAY<T,TV_INT> exchanged_phi_ghost(grid->Domain_Indices(8));
    particle_levelset_evolution->particle_levelset.levelset.boundary->
        Fill_Ghost_Cells(*grid, particle_levelset_evolution->phi,
                exchanged_phi_ghost, 0, time, 8);
    incompressible->Extrapolate_Velocity_Across_Interface
        (*face_velocities->data, exchanged_phi_ghost, false, 3, 0, TV());

    projection->Initialize_Grid(*grid);

    std::cout << "Moving to incomplete implementation ...\n";

    Set_Boundary_Conditions(driver, time, face_velocities); // get so CFL is correct

    driver->Write_Output_Files(driver->first_frame);

    std::cout << "Successfully initialized non advection data\n";
    return false;
}

template <class TV, class T> void NonAdvData<TV, T>::
BeforeAdvection
(WaterDriver<TV> *driver, FaceArray<TV> *face_velocities, const T target_time)
{

//TODO(omidm): This part should be added to the Loop Job itself. 
// ***********************************************************************
//    bool done=false;for(int substep=1;!done;substep++){
        LOG::Time("Calculate Dt");
        (*particle_levelset_evolution).Set_Number_Particles_Per_Cell(16);
        T dt = driver->cfl * incompressible->CFL(*face_velocities->data);
        dt = min(dt, (*particle_levelset_evolution).CFL(false,false));
//        if(time+dt>=target_time){dt=target_time-time;done=true;}
//        else if(time+2*dt>=target_time){dt=.5*(target_time-time);}
// ***********************************************************************



        LOG::Time("Compute Occupied Blocks");
        T maximum_fluid_speed= face_velocities->data->Maxabs().Max();
        T max_particle_collision_distance=particle_levelset_evolution->particle_levelset.max_collision_distance_factor
          * grid->dX.Max();
        collision_bodies_affecting_fluid->Compute_Occupied_Blocks(true, dt *
            maximum_fluid_speed + 2 * max_particle_collision_distance + (T).5 *
            grid->dX.Max(), 10);

        LOG::Time("Adjust Phi With Objects");
        T_FACE_ARRAYS_SCALAR face_velocities_ghost;
        face_velocities_ghost.Resize(incompressible->grid, number_of_ghost_cells, false);
        incompressible->boundary->Fill_Ghost_Cells_Face(*grid,
            *face_velocities->data, face_velocities_ghost, time + dt, number_of_ghost_cells);

        //Advect Phi 3.6% (Parallelized)
        LOG::Time("Advect Phi");
        phi_boundary_water->Use_Extrapolation_Mode(false);
        particle_levelset_evolution->Advance_Levelset(dt);
        phi_boundary_water->Use_Extrapolation_Mode(true);

        //Advect Particles 12.1% (Parallelized)
        LOG::Time("Step Particles");
        particle_levelset_evolution->particle_levelset.Euler_Step_Particles(face_velocities_ghost,dt,time,true,true,false,false);

        //Advect removed particles (Parallelized)
        LOG::Time("Advect Removed Particles");
        RANGE<TV_INT> domain(grid->Domain_Indices());
        domain.max_corner += TV_INT::All_Ones_Vector();

        // TODO(omidm): unomment and fix it.
//        DOMAIN_ITERATOR_THREADED_ALPHA<WATER_DRIVER<TV>,TV>(domain,0).template Run<T,T>(*this,&WATER_DRIVER<TV>::Run,dt,time);

}
#ifndef TEMPLATE_USE
#define TEMPLATE_USE
typedef VECTOR<float, 2> TVF2;
typedef float TF;
#endif  // TEMPLATE_USE

template class FaceArray<TVF2>;
template class FaceArrayGhost<TVF2>;
template class NonAdvData<TVF2, TF>;
