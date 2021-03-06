//#####################################################################
// Copyright 2009, Michael Lentine.
// This file is part of PhysBAM whose distribution is governed by the license contained in the accompanying file PHYSBAM_COPYRIGHT.txt.
//#####################################################################
// This file contains initializers for water driver/ example, and
// different members contained in the example.
//#####################################################################
#include "stdio.h"
#include "string.h"

#include "applications/physbam/water//water_app.h"
#include "applications/physbam/water//app_utils.h"
#include "applications/physbam/water//data_names.h"
#include "applications/physbam/water//parameters.h"
#include "applications/physbam/water//physbam_include.h"
#include "applications/physbam/water//water_driver.h"
#include "applications/physbam/water//water_example.h"
#include "src/shared/dbg.h"
#include "src/shared/geometric_region.h"
#include "src/shared/nimbus.h"



using namespace PhysBAM;

// Distirbuted initialization for the first time
template<class TV> void WATER_DRIVER<TV>::InitializeFirstDistributed(
    const nimbus::Job *job,
    const nimbus::DataArray &da) {
  typedef application::DataConfig DataConfig;
  DEBUG_SUBSTEPS::Set_Write_Substeps_Level(example.write_substeps_level);
  output_number=current_frame;
  time=example.Time_At_Frame(current_frame);

  // domain boundaries
  {
    TV min_corner = example.mac_grid.Domain().Minimum_Corner();
    TV max_corner = example.mac_grid.Domain().Maximum_Corner();
    for (int i = 1; i <= TV::dimension; i++) {
      example.domain_boundary(i)(1) = (min_corner(i) <= 0.001);
      example.domain_boundary(i)(2) = (max_corner(i) >= 0.999);
    }
    example.domain_boundary(2)(2)=false;
    example.phi_boundary_water.Set_Velocity_Pointer(example.face_velocities);
    VECTOR<VECTOR<bool,2>,TV::dimension> domain_open_boundaries=VECTOR_UTILITIES::Complement(example.domain_boundary);
    example.phi_boundary=&example.phi_boundary_water;
    example.phi_boundary->Set_Constant_Extrapolation(domain_open_boundaries);
    example.boundary=&example.boundary_scalar;
    example.boundary->Set_Constant_Extrapolation(domain_open_boundaries);
  }

  // allocates array for levelset/ particles/ removed particles
  {
    InitializeParticleLevelsetEvolutionHelper(
        example.data_config,
        example.mac_grid,
        &example.particle_levelset_evolution,
        true);
    InitializeIncompressibleProjectionHelper(
        example.data_config,
        example.mac_grid,
        &example.incompressible,
        &example.projection,
        true);
    if (example.data_config.GetFlag(DataConfig::VELOCITY)) {
      // [Allocate] face_velocities.
      example.face_velocities.Resize(example.mac_grid);
    }
    if (example.data_config.GetFlag(DataConfig::VELOCITY_GHOST)) {
      // [Allocate] face_velocities_ghost.
      example.face_velocities_ghost.Resize(example.incompressible.grid,
          example.number_of_ghost_cells, false);
    }
  }

  {
    // policies etc
    // example.collision_bodies_affecting_fluid->Initialize_Grids();
    // example.incompressible.Set_Custom_Advection(example.advection_scalar);

    example.particle_levelset_evolution.particle_levelset.Set_Band_Width(6);
    example.particle_levelset_evolution.Set_Time(time);
    example.particle_levelset_evolution.Set_CFL_Number((T).9);
    // example.particle_levelset_evolution.Levelset_Advection(1).Set_Custom_Advection(example.advection_scalar);
    example.particle_levelset_evolution.Set_Number_Particles_Per_Cell(16);
    example.particle_levelset_evolution.Set_Levelset_Callbacks(example);
    example.particle_levelset_evolution.Initialize_FMM_Initialization_Iterative_Solver(true);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Custom_Boundary(*example.phi_boundary);
    example.particle_levelset_evolution.Bias_Towards_Negative_Particles(false);
    example.particle_levelset_evolution.particle_levelset.Use_Removed_Positive_Particles();
    example.particle_levelset_evolution.particle_levelset.Use_Removed_Negative_Particles();
    example.particle_levelset_evolution.particle_levelset.Store_Unique_Particle_Id();
    example.particle_levelset_evolution.Use_Particle_Levelset(true);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Collision_Body_List(*example.collision_bodies_affecting_fluid);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Face_Velocities_Valid_Mask(&example.incompressible.valid_mask);
    example.particle_levelset_evolution.particle_levelset.Set_Collision_Distance_Factors(.1,1);

    example.incompressible.Set_Custom_Boundary(*example.boundary);
    example.incompressible.projection.elliptic_solver->Set_Relative_Tolerance(1e-8);
    example.incompressible.projection.elliptic_solver->pcg.Set_Maximum_Iterations(40);
    example.incompressible.projection.elliptic_solver->pcg.evolution_solver_type=krylov_solver_cg;
    example.incompressible.projection.elliptic_solver->pcg.cg_restart_iterations=0;
    example.incompressible.projection.elliptic_solver->pcg.Show_Results();
    example.incompressible.projection.collidable_solver->Use_External_Level_Set(example.particle_levelset_evolution.particle_levelset.levelset);
    LAPLACE_COLLIDABLE_UNIFORM<T_GRID>* laplace_solver =
      dynamic_cast<LAPLACE_COLLIDABLE_UNIFORM<T_GRID>* >(
          example.projection.elliptic_solver);
    example.laplace_solver_wrapper.BindLaplaceAndInitialize(laplace_solver);

    // example.collision_bodies_affecting_fluid->Update_Intersection_Acceleration_Structures(false);
    // example.collision_bodies_affecting_fluid->Rasterize_Objects();
    // example.collision_bodies_affecting_fluid->Compute_Occupied_Blocks(false,(T)2*example.mac_grid.Minimum_Edge_Length(),5);
    // Initializes for particles_levelset_evolution.phi
    application::WaterApp *app = dynamic_cast<application::WaterApp *>(job->application());
    assert(app);
    example.Initialize_Phi(app->water_level());
    example.Adjust_Phi_With_Sources(time);
    // example.particle_levelset_evolution.Make_Signed_Distance();
    example.projection.p.Fill(0);
    // example.particle_levelset_evolution.Fill_Levelset_Ghost_Cells(time);

    // example.collision_bodies_affecting_fluid->Compute_Grid_Visibility();
    // example.particle_levelset_evolution.Set_Seed(2606);
    // example.particle_levelset_evolution.Seed_Particles(time);
    // example.particle_levelset_evolution.Delete_Particles_Outside_Grid();

    //add forces
    example.incompressible.Set_Gravity();
    example.incompressible.Set_Body_Force(true);
    example.incompressible.projection.Use_Non_Zero_Divergence(false);
    example.incompressible.projection.elliptic_solver->Solve_Neumann_Regions(false);
    example.incompressible.projection.elliptic_solver->solve_single_cell_neumann_regions=false;
    example.incompressible.Use_Explicit_Part_Of_Implicit_Viscosity(false);
    example.incompressible.Set_Maximum_Implicit_Viscosity_Iterations(40);
    example.incompressible.Use_Variable_Vorticity_Confinement(false);
    example.incompressible.Set_Surface_Tension(0);
    example.incompressible.Set_Variable_Surface_Tension(false);
    example.incompressible.Set_Viscosity(0);
    example.incompressible.Set_Variable_Viscosity(false);
    example.incompressible.projection.Set_Density(1e3);

//    ARRAY<T,TV_INT> exchanged_phi_ghost(example.mac_grid.Domain_Indices(8));
//    example.particle_levelset_evolution.particle_levelset.levelset.boundary->Fill_Ghost_Cells(example.mac_grid,example.particle_levelset_evolution.phi,exchanged_phi_ghost,0,time,8);
//    example.incompressible.Extrapolate_Velocity_Across_Interface(example.face_velocities,exchanged_phi_ghost,false,3,0,TV());
    example.Set_Boundary_Conditions(time); // get so CFL is correct
  }
  // write, save
  //  Write_Output_Files(example.first_frame);
  // example.collision_bodies_affecting_fluid->Compute_Occupied_Blocks(true,0,0);
  example.particle_levelset_evolution.Set_Number_Particles_Per_Cell(16);
  example.Save_To_Nimbus_No_AppData(job, da, current_frame);
}

// Initialize when application app_data is being used, with app_data
template<class TV> void WATER_DRIVER<TV>::InitializeUseCachedAppData(
    const nimbus::Job *job,
    const nimbus::DataArray &da) {
  typedef application::DataConfig DataConfig;
  DEBUG_SUBSTEPS::Set_Write_Substeps_Level(example.write_substeps_level);
  output_number=current_frame;

  // domain boundaries
  {
    application::ScopeTimer scope_timer("part_1");
    // [OK] Insignificant.
    TV min_corner = example.mac_grid.Domain().Minimum_Corner();
    TV max_corner = example.mac_grid.Domain().Maximum_Corner();
    for (int i = 1; i <= TV::dimension; i++) {
      example.domain_boundary(i)(1) = (min_corner(i) <= 0.001);
      example.domain_boundary(i)(2) = (max_corner(i) >= 0.999);
    }
    example.domain_boundary(2)(2)=false;
    example.phi_boundary_water.Set_Velocity_Pointer(example.face_velocities);
    VECTOR<VECTOR<bool,2>,TV::dimension> domain_open_boundaries=VECTOR_UTILITIES::Complement(example.domain_boundary);
    example.phi_boundary=&example.phi_boundary_water;
    example.phi_boundary->Set_Constant_Extrapolation(domain_open_boundaries);
    example.boundary=&example.boundary_scalar;
    example.boundary->Set_Constant_Extrapolation(domain_open_boundaries);
  }
  // allocates array for levelset/ particles/ removed particles
  {
    application::ScopeTimer scope_timer("part_2.1");
    InitializeParticleLevelsetEvolutionHelperUseCachedAppData(
        example.data_config,
        example.mac_grid,
        &example.particle_levelset_evolution);
  }
  {
    application::ScopeTimer scope_timer("part_2.2");
    // [OK] Insignificant.
    InitializeIncompressibleProjectionHelper(
        example.data_config,
        example.mac_grid,
        &example.incompressible,
        &example.projection);
  }
  {
    application::ScopeTimer scope_timer("part_2.3");
    // example.collision_bodies_affecting_fluid->Initialize_Grids();
  }
  {
    application::ScopeTimer scope_timer("part_3.1");
    // [OK] Insignificant.
    // policies etc
    example.incompressible.projection.elliptic_solver->Set_Relative_Tolerance(1e-8);
    example.incompressible.projection.elliptic_solver->pcg.Set_Maximum_Iterations(40);
    example.incompressible.projection.elliptic_solver->pcg.evolution_solver_type=krylov_solver_cg;
    example.incompressible.projection.elliptic_solver->pcg.cg_restart_iterations=0;
    example.incompressible.projection.elliptic_solver->pcg.Show_Results();
  }

  {
    application::ScopeTimer scope_timer("part_3.2");
    // [OK] Insignificant. No way to solve.
    // load
    example.Load_From_Nimbus(job, da, current_frame);
  }

  {
    application::ScopeTimer scope_timer("part_3.3");
    // [OK] Insignificant.
    // For threading.
    if (example.nimbus_thread_queue) {
      ADVECTION_SEMI_LAGRANGIAN_UNIFORM_BETA<GRID<TV>,T>* threaded_advection_scalar
          =new ADVECTION_SEMI_LAGRANGIAN_UNIFORM_BETA<GRID<TV>,T>(example.nimbus_thread_queue);
      example.particle_levelset_evolution.Levelset_Advection(1).Set_Custom_Advection(*threaded_advection_scalar);
      example.incompressible.Set_Custom_Advection(*threaded_advection_scalar);
      example.particle_levelset_evolution.particle_levelset.Set_Thread_Queue(example.nimbus_thread_queue);
      example.particle_levelset_evolution.particle_levelset.levelset.thread_queue=example.nimbus_thread_queue;
    } else {
      example.particle_levelset_evolution.Levelset_Advection(1).Set_Custom_Advection(example.advection_scalar);
      example.incompressible.Set_Custom_Advection(example.advection_scalar);
      example.particle_levelset_evolution.particle_levelset.Set_Thread_Queue(NULL);
      example.particle_levelset_evolution.particle_levelset.levelset.thread_queue=NULL;
    }
  }

  {
    application::ScopeTimer scope_timer("part_4.1");
    // [OK] Insignificant.

    // example specific init
    example.particle_levelset_evolution.Set_Time(time);
    example.particle_levelset_evolution.Set_Levelset_Callbacks(example);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Custom_Boundary(*example.phi_boundary);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Collision_Body_List(*example.collision_bodies_affecting_fluid);
    example.particle_levelset_evolution.particle_levelset.levelset.Set_Face_Velocities_Valid_Mask(&example.incompressible.valid_mask);
    example.particle_levelset_evolution.Set_Seed(2606);

  }
  {
    application::ScopeTimer scope_timer("part_4.2");
    // example.collision_bodies_affecting_fluid->Rasterize_Objects();
    // example.collision_bodies_affecting_fluid->
    //   Compute_Occupied_Blocks(false, (T)2*example.mac_grid.Minimum_Edge_Length(),5);
    // example.collision_bodies_affecting_fluid->Compute_Grid_Visibility();

    example.incompressible.Set_Custom_Boundary(*example.boundary);
    example.incompressible.projection.collidable_solver->Use_External_Level_Set(example.particle_levelset_evolution.particle_levelset.levelset);
  }
  {
    application::ScopeTimer scope_timer("part_4.3");
    // [OK] Insignificant.
    LAPLACE_COLLIDABLE_UNIFORM<T_GRID>* laplace_solver =
      dynamic_cast<LAPLACE_COLLIDABLE_UNIFORM<T_GRID>* >(
          example.projection.elliptic_solver);
    example.laplace_solver_wrapper.BindLaplaceAndInitialize(laplace_solver);
  }
  {
    application::ScopeTimer scope_timer("part_4.4.1");
    //add forces
    example.incompressible.Set_Gravity();
  }
  {
    application::ScopeTimer scope_timer("part_4.4.2");
    example.incompressible.use_force = true;
    example.incompressible.force.Nimbus_Delete_Base_Pointer();
    T_FACE_ARRAYS_SCALAR::Nimbus_Copy_Arrays(
        example.incompressible.force,
        *example.static_config_force->GetData());
  }
  {
    application::ScopeTimer scope_timer("part_4.4.3");
    example.incompressible.projection.Use_Non_Zero_Divergence(false);
  }
  {
    application::ScopeTimer scope_timer("part_4.4.4");
    example.incompressible.projection.elliptic_solver->Solve_Neumann_Regions(false);
  }
  {
    application::ScopeTimer scope_timer("part_4.4.4");
    example.incompressible.projection.elliptic_solver->solve_single_cell_neumann_regions=false;
  }
  {
    application::ScopeTimer scope_timer("part_4.5");
    // [OK] Insignificant.
    example.incompressible.Use_Explicit_Part_Of_Implicit_Viscosity(false);
    example.incompressible.Set_Maximum_Implicit_Viscosity_Iterations(40);
    example.incompressible.Use_Variable_Vorticity_Confinement(false);
    example.incompressible.Set_Surface_Tension(0);
    example.incompressible.Set_Variable_Surface_Tension(false);
    example.incompressible.Set_Viscosity(0);
    example.incompressible.Set_Variable_Viscosity(false);
    example.incompressible.projection.Set_Density(1e3);
  }
  {
    application::ScopeTimer scope_timer("part_4.6");
    // example.collision_bodies_affecting_fluid->Compute_Occupied_Blocks(true,0,0);
    example.particle_levelset_evolution.Set_Number_Particles_Per_Cell(16);
  }
}

template<class TV> bool WATER_DRIVER<TV>::InitializeIncompressibleProjectionHelper(
    const application::DataConfig& data_config,
    const GRID<TV>& grid_input,
    INCOMPRESSIBLE_UNIFORM<GRID<TV> >* incompressible,
    PROJECTION_DYNAMICS_UNIFORM<GRID<TV> >* projection,
    bool forced_alloc) {
  typedef application::DataConfig DataConfig;
  // T_FACE_ARRAYS_BOOL.
  if (data_config.GetFlag(DataConfig::VALID_MASK)) {
    if (example.static_config_valid_mask) {
      incompressible->valid_mask.Nimbus_Delete_Base_Pointer();
      T_FACE_ARRAYS_BOOL::Nimbus_Copy_Arrays(
          incompressible->valid_mask, *example.static_config_valid_mask->GetData());
    } else {
      incompressible->valid_mask.Resize(
          grid_input.Domain_Indices(3), true, true, true);
    }
  }
  incompressible->grid = grid_input.Get_MAC_Grid();
  // Strain is not considered.
  assert(incompressible->strain == NULL);
  assert(grid_input.Is_MAC_Grid());
  projection->p_grid = grid_input;
  // Laplace solver is used.
  assert(projection->poisson == NULL);
  assert(projection->laplace != NULL);
  assert(grid_input.DX()==TV() || grid_input.Is_MAC_Grid());
  // projection->laplace->Initialize_Grid(grid_input);
  {
    LAPLACE_COLLIDABLE_UNIFORM<GRID<TV> >* laplace =
      dynamic_cast<LAPLACE_COLLIDABLE_UNIFORM<GRID<TV> >*>(
          projection->laplace);
    laplace->grid = grid_input;
    laplace->second_order_cut_cell_method = true;
    if (data_config.GetFlag(DataConfig::U_INTERFACE)) {
      if (example.static_config_u_interface) {
        laplace->u_interface.Nimbus_Delete_Base_Pointer();
        T_FACE_ARRAYS_SCALAR::Nimbus_Copy_Arrays(
            laplace->u_interface,
            *example.static_config_u_interface->GetData());
      } else {
        laplace->u_interface.Resize(grid_input);
      }
    } else {
      laplace->u_interface.Clean_Memory();
    }
    // T_ARRAYS_SCALAR.
    if (data_config.GetFlag(DataConfig::DIVERGENCE)) {
      if (forced_alloc) {
        laplace->f.Resize(grid_input.Domain_Indices(1));
      }
    }
    // T_FACE_ARRAYS_BOOL.
    if (data_config.GetFlag(DataConfig::PSI_N)) {
      if (forced_alloc) {
        laplace->psi_N.Resize(grid_input, 1);
      }
    }
    // T_ARRAYS_BOOL.
    if (data_config.GetFlag(DataConfig::PSI_D)) {
      if (forced_alloc) {
        laplace->psi_D.Resize(grid_input.Domain_Indices(1));
      }
    }
    // T_ARRAYS_INT.
    if (data_config.GetFlag(DataConfig::REGION_COLORS)) {
      if (forced_alloc) {
        laplace->filled_region_colors.Resize(
            grid_input.Domain_Indices(1));
      }
    }
    // Assume uniform region coloring.
    laplace->number_of_regions = 1;
    laplace->filled_region_touches_dirichlet.Resize(1);
    laplace->filled_region_touches_dirichlet(1) = true;
  }
  // Flag use_non_zero_divergence is expected to be false.
  assert(!projection->use_non_zero_divergence);
  projection->divergence.Clean_Memory();
  // T_ARRAYS_SCALAR.
  if (data_config.GetFlag(DataConfig::PRESSURE)) {
    if (forced_alloc) {
      projection->p.Resize(grid_input.Domain_Indices(1));
    }
  }
  // T_ARRAYS_SCALAR.
  if (data_config.GetFlag(DataConfig::PRESSURE_SAVE)) {
    // projection->p_save_for_projection.Resize(grid_input.Domain_Indices(1));
  }
  // T_FACE_ARRAYS_SCALAR.
  if (data_config.GetFlag(DataConfig::VELOCITY_SAVE)) {
    // projection->face_velocities_save_for_projection.Resize(grid_input);
  }
  // dsd is not considered.
  assert(projection->dsd == NULL);
  return true;
}

template<class TV> bool WATER_DRIVER<TV>::InitializeParticleLevelsetEvolutionHelper(
    const application::DataConfig& data_config,
    const GRID<TV>& grid_input,
    PARTICLE_LEVELSET_EVOLUTION_UNIFORM<GRID<TV> >*
    particle_levelset_evolution,
    bool forced_alloc) {
  typedef application::DataConfig DataConfig;
  PARTICLE_LEVELSET_UNIFORM<GRID<TV> >* particle_levelset =
    &particle_levelset_evolution->particle_levelset;
  assert(grid_input.Is_MAC_Grid());
  particle_levelset_evolution->grid = grid_input;
  // Resizes phi here.
  if (data_config.GetFlag(DataConfig::LEVELSET)
      || data_config.GetFlag(DataConfig::LEVELSET_READ)
      || data_config.GetFlag(DataConfig::LEVELSET_WRITE)) {
    particle_levelset_evolution->phi.Resize(
        grid_input.Domain_Indices(particle_levelset->number_of_ghost_cells));
  }
  // Resizes particles.
  if (data_config.GetFlag(DataConfig::POSITIVE_PARTICLE)) {
    particle_levelset->positive_particles.Resize(
        particle_levelset->levelset.grid.Block_Indices(
          particle_levelset->number_of_ghost_cells));
  }
  if (data_config.GetFlag(DataConfig::NEGATIVE_PARTICLE)) {
    particle_levelset->negative_particles.Resize(
        particle_levelset->levelset.grid.Block_Indices(
          particle_levelset->number_of_ghost_cells));
  }
  particle_levelset->use_removed_positive_particles=true;
  particle_levelset->use_removed_negative_particles=true;
  // Resizes removed particles.
  if (data_config.GetFlag(DataConfig::REMOVED_POSITIVE_PARTICLE)) {
    particle_levelset->removed_positive_particles.Resize(
        particle_levelset->levelset.grid.Block_Indices(
          particle_levelset->number_of_ghost_cells));
  }
  if (data_config.GetFlag(DataConfig::REMOVED_NEGATIVE_PARTICLE)) {
    particle_levelset->removed_negative_particles.Resize(
        particle_levelset->levelset.grid.Block_Indices(
          particle_levelset->number_of_ghost_cells));
  }
  particle_levelset->Set_Minimum_Particle_Radius(
      (T).1*particle_levelset->levelset.grid.Minimum_Edge_Length());
  particle_levelset->Set_Maximum_Particle_Radius(
      (T).5*particle_levelset->levelset.grid.Minimum_Edge_Length());
  if (particle_levelset->half_band_width &&
      particle_levelset->levelset.grid.Minimum_Edge_Length()) {
    particle_levelset->Set_Band_Width(particle_levelset->half_band_width /
        ((T).5*particle_levelset->levelset.grid.Minimum_Edge_Length()));
  } else {
    particle_levelset->Set_Band_Width();
  }
  particle_levelset->levelset.Initialize_Levelset_Grid_Values();
  if (particle_levelset_evolution->
      levelset_advection.semi_lagrangian_collidable) {
    particle_levelset->levelset.Initialize_Valid_Masks(grid_input);
  }
  particle_levelset_evolution->Set_CFL_Number((T).9);
  particle_levelset_evolution->Set_Number_Particles_Per_Cell(16);
  particle_levelset_evolution->Initialize_FMM_Initialization_Iterative_Solver(true);
  particle_levelset_evolution->Bias_Towards_Negative_Particles(false);
  particle_levelset_evolution->particle_levelset.Use_Removed_Positive_Particles();
  particle_levelset_evolution->particle_levelset.Use_Removed_Negative_Particles();
  particle_levelset_evolution->particle_levelset.Store_Unique_Particle_Id();
  particle_levelset_evolution->Use_Particle_Levelset(true);
  particle_levelset_evolution->particle_levelset.Set_Collision_Distance_Factors(.1,1);
  return true;
}

template<class TV> bool WATER_DRIVER<TV>::InitializeParticleLevelsetEvolutionHelperUseCachedAppData(
    const application::DataConfig& data_config,
    const GRID<TV>& grid_input,
    PARTICLE_LEVELSET_EVOLUTION_UNIFORM<GRID<TV> >*
    particle_levelset_evolution,
    bool forced_alloc) {
  typedef application::DataConfig DataConfig;
  PARTICLE_LEVELSET_UNIFORM<GRID<TV> >* particle_levelset =
    &particle_levelset_evolution->particle_levelset;
  assert(grid_input.Is_MAC_Grid());
  // If this flag is true, it suggests that the ple is not a app_datad version.
  if (example.create_destroy_ple) {
    {
      application::ScopeTimer scope_timer("part_2.1.1");
      particle_levelset_evolution->grid = grid_input;
      // Resizes phi here.
      if (data_config.GetFlag(DataConfig::LEVELSET)
          || data_config.GetFlag(DataConfig::LEVELSET_READ)
          || data_config.GetFlag(DataConfig::LEVELSET_WRITE)) {
        if (forced_alloc) {
          particle_levelset_evolution->phi.Resize(
              grid_input.Domain_Indices(particle_levelset->number_of_ghost_cells));
        }
      }
      // Resizes particles.
      particle_levelset_evolution->particle_levelset.Set_Band_Width(6);
      if (data_config.GetFlag(DataConfig::POSITIVE_PARTICLE)) {
        if (forced_alloc) {
          particle_levelset->positive_particles.Resize(
              particle_levelset->levelset.grid.Block_Indices(
                  particle_levelset->number_of_ghost_cells));
        }
      }
      if (data_config.GetFlag(DataConfig::NEGATIVE_PARTICLE)) {
        if (forced_alloc) {
          particle_levelset->negative_particles.Resize(
              particle_levelset->levelset.grid.Block_Indices(
                  particle_levelset->number_of_ghost_cells));
        }
      }
      particle_levelset->use_removed_positive_particles=true;
      particle_levelset->use_removed_negative_particles=true;
      // Resizes removed particles.
      if (forced_alloc &&
          data_config.GetFlag(DataConfig::REMOVED_POSITIVE_PARTICLE)) {
        particle_levelset->removed_positive_particles.Resize(
            particle_levelset->levelset.grid.Block_Indices(
                particle_levelset->number_of_ghost_cells));
      }
      if (forced_alloc &&
          data_config.GetFlag(DataConfig::REMOVED_NEGATIVE_PARTICLE)) {
        particle_levelset->removed_negative_particles.Resize(
            particle_levelset->levelset.grid.Block_Indices(
                particle_levelset->number_of_ghost_cells));
      }
      particle_levelset->Set_Minimum_Particle_Radius(
          (T).1*particle_levelset->levelset.grid.Minimum_Edge_Length());
      particle_levelset->Set_Maximum_Particle_Radius(
          (T).5*particle_levelset->levelset.grid.Minimum_Edge_Length());
    }
    {
      application::ScopeTimer scope_timer("part_2.1.2");
      if (particle_levelset->half_band_width &&
          particle_levelset->levelset.grid.Minimum_Edge_Length()) {
        particle_levelset->Set_Band_Width(particle_levelset->half_band_width /
                                          ((T).5*particle_levelset->levelset.grid.Minimum_Edge_Length()));
      } else {
        particle_levelset->Set_Band_Width();
      }
      if (forced_alloc) {
        particle_levelset->levelset.Initialize_Levelset_Grid_Values();
      }
      if (forced_alloc &&
          particle_levelset_evolution->
          levelset_advection.semi_lagrangian_collidable) {
        particle_levelset->levelset.Initialize_Valid_Masks(grid_input);
      }
    }
    {
      application::ScopeTimer scope_timer("part_2.1.3");
      particle_levelset_evolution->Set_CFL_Number((T).9);
      particle_levelset_evolution->Set_Number_Particles_Per_Cell(16);
    }
    {
      application::ScopeTimer scope_timer("part_2.1.4");
      particle_levelset_evolution->Initialize_FMM_Initialization_Iterative_Solver(true);
    }
    {
      application::ScopeTimer scope_timer("part_2.1.5");
      particle_levelset_evolution->Bias_Towards_Negative_Particles(false);
      if (forced_alloc) {
        particle_levelset_evolution->particle_levelset.Use_Removed_Positive_Particles();
        particle_levelset_evolution->particle_levelset.Use_Removed_Negative_Particles();
      } else {
        particle_levelset_evolution->particle_levelset.use_removed_positive_particles = true;
        particle_levelset_evolution->particle_levelset.use_removed_negative_particles = true;
      }
    }
    {
      application::ScopeTimer scope_timer("part_2.1.6");
      particle_levelset_evolution->particle_levelset.Store_Unique_Particle_Id();
      particle_levelset_evolution->Use_Particle_Levelset(true);
      particle_levelset_evolution->particle_levelset.Set_Collision_Distance_Factors(.1,1);
    }
  }
  return true;
}

template class WATER_DRIVER<VECTOR<float,3> >;
