// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/ligand_docking/ga_dock/GALigandDock.cc
///
/// @brief
/// @author Hahnbeom Park and Frank DiMaio

#include <protocols/ligand_docking/GALigandDock/GALigandDock.hh>
#include <protocols/ligand_docking/GALigandDock/GALigandDockCreator.hh>
#include <protocols/ligand_docking/GALigandDock/GAOptimizer.hh>
#include <protocols/ligand_docking/GALigandDock/GridScorer.hh>
#include <protocols/ligand_docking/GALigandDock/util.hh>
#include <protocols/ligand_docking/GALigandDock/EntropyEstimator.hh>
#include <protocols/ligand_docking/GALigandDock/TorsionSampler.hh>

#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/Energies.hh>
#include <core/scoring/rms_util.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/util.hh>
#include <core/chemical/ResidueTypeSet.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/select/util.hh>
#include <core/select/residue_selector/ResidueSelector.hh>
#include <core/pose/selection.hh>
#include <core/pose/util.hh>
#include <core/pose/PDBInfo.hh>
#include <core/pose/subpose_manipulation_util.hh>
#include <core/pack/optimizeH.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <protocols/minimization_packing/MinMover.hh>
#include <protocols/simple_moves/ExplicitWaterMover.hh>

#include <core/scoring/constraints/Constraint.fwd.hh>
#include <core/scoring/constraints/AngleConstraint.hh>
#include <core/scoring/constraints/AtomPairConstraint.hh>
#include <core/scoring/constraints/DihedralConstraint.hh>
#include <core/scoring/constraints/CoordinateConstraint.hh>
#include <core/scoring/func/ScalarWeightedFunc.hh>
#include <core/scoring/func/TopOutFunc.hh>
#include <core/scoring/func/HarmonicFunc.hh>
#include <core/scoring/func/CircularHarmonicFunc.hh>

#include <core/pack/pack_rotamers.hh>
#include <core/import_pose/import_pose.hh>
#include <core/optimization/AtomTreeMinimizer.hh>
#include <core/optimization/CartesianMinimizer.hh>

#include <core/io/silent/SilentFileData.hh>
#include <core/io/silent/SilentFileOptions.hh>

#include <basic/Tracer.hh>
#include <utility/vector1.hh>
#include <protocols/rosetta_scripts/util.hh>
#include <protocols/relax/FastRelax.hh>
#include <protocols/simple_moves/DeclareBond.hh>

#include <basic/datacache/DataMap.hh>
#include <utility/tag/XMLSchemaGeneration.hh>
#include <utility/tag/Tag.hh>
#include <protocols/moves/mover_schemas.hh>
#include <numeric/xyzVector.hh>
#include <numeric/xyzMatrix.hh>
#include <numeric/xyz.functions.hh>
#include <numeric/random/random.hh>
#include <core/pose/extra_pose_info_util.hh>
#include <core/id/AtomID.hh>
#include <utility/vector1.hh>

#include <basic/options/option.hh> // HACK
#include <basic/options/keys/out.OptionKeys.gen.hh>

#include <fstream>

#include <core/kinematics/Jump.hh> // AUTO IWYU For Jump
#include <core/optimization/MinimizerOptions.hh> // AUTO IWYU For MinimizerOptions
#include <core/pack/task/ResidueLevelTask.hh> // AUTO IWYU For ResidueLevelTask

namespace protocols {
namespace ligand_docking {
namespace ga_ligand_dock {

using namespace core;

static basic::Tracer TR( "protocols.ligand_docking.GALigandDock.GALigandDock" );

GALigandDock::GALigandDock() {
	scfxn_ = core::scoring::get_score_function();
	scfxn_relax_ = core::scoring::get_score_function();
	scfxn_->reset_energy_methods();
	scfxn_relax_->reset_energy_methods();

	ngen_ = 20;
	npool_ = 50;
	nrelax_ = 20;
	nreport_ = 20;
	pmut_ = 0.2;
	rmsdthreshold_ = 1.0;
	smoothing_ = 0.0;
	sample_ring_conformers_ = true; // still only works if params contains proper ring definition

	// grid
	grid_ = 0.25;
	padding_ = 4.0;
	hashsize_ = 8.0;
	subhash_ = 3;
	fa_rep_grid_ = 0.2;
	grid_bound_penalty_ = 100.0;

	debug_ = exact_ = false;
	sidechains_ = "none";
	sc_edge_buffer_ = 2.0; // in A
	optimize_input_H_ = true;
	pre_optH_relax_ = true;

	// final relaxation
	final_exact_minimize_ = "sc";
	min_neighbor_ = false;
	cartmin_lig_ = true; //default for all the protocols
	full_repack_before_finalmin_ = false;
	final_solvate_ = false;
	fast_relax_script_file_ = "";
	fast_relax_lines_ = std::vector<std::string>();
	turnon_flexscs_at_relax_ = false;
	redefine_flexscs_at_relax_ = false;

	// estimate and report dG
	estimate_dG_ = false;

	// packing behavior
	max_rot_cumulative_prob_ = 0.9;
	rot_energy_cutoff_ = 100;
	maxiter_ = 50;
	packer_cycles_ = 100;
	favor_native_ = 2;

	// input generation
	premin_ligand_ = false;
	random_oversample_ = 10;
	reference_oversample_ = 2;
	reference_frac_ = 0.5;
	reference_frac_auto_ = true;
	use_pharmacophore_ = true; // turn on by default!
	torsion_sampler_percentage_ = 0.0;
	contact_distance_ = 4.5;
	freeze_ligand_backbone_ = false;
	freeze_ligand_ = false;
	macrocycle_ligand_ = false;

	multiple_ligands_ = utility::vector1< std::string >();
	ligand_file_list_ = utility::vector1< std::string >();
	multi_ligands_maxRad_ = 0.0;
	initial_pool_ = "";
	reference_pool_ = "none";

	use_mean_maxRad_ = false;
	stdev_multiplier_ = 1.0; // most of the time, only mean value is too small

	move_water_ = true;

	align_reference_atom_ids_ = utility::vector1< core::id::AtomID >();

	runmode_ = "dockflex"; // high-resolution pharmacophore docking
}

void
GALigandDock::get_ligand_resids(pose::Pose const& pose,
	utility::vector1 < core::Size >& lig_resids) const
{
	lig_resids.clear();
	core::Size lastres = pose.total_residue();
	while ( pose.residue(lastres).is_virtual_residue() && lastres>1 ) lastres--;
	lig_resids.push_back( lastres );
	bool extending = true;
	while ( extending ) {
		if ( !pose.residue(lastres).is_polymer() ) {
			extending = false; // ligand not a polymer, we're done
		} else if ( pose.residue(lastres).has_lower_connect() ) {
			core::Size nextres = pose.residue(lastres).connected_residue_at_resconn( pose.residue(lastres).type().lower_connect_id() );
			if ( std::find (lig_resids.begin(), lig_resids.end(), nextres) == lig_resids.end() ) {
				lastres = nextres;
				lig_resids.push_back( lastres );
			} else {
				extending = false;  // finished a cyclic peptide, we're done
			}
		} else {
			extending = false; // hit n-term
		}
	}
}

//pass by value of pose_complex
core::pose::PoseOP
GALigandDock::replace_ligand(core::pose::Pose pose_complex, core::pose::Pose& pose_ligand, bool align) const
{
	utility::vector1 < core::Size > lig_resids;
	get_ligand_resids(pose_complex, lig_resids);
	if ( align ) {
		numeric::xyzVector< core::Real > com_tgt(0.0, 0.0, 0.0);
		numeric::xyzVector< core::Real > com_src(0.0, 0.0, 0.0);
		for ( core::Size ligid:lig_resids ) com_tgt += pose_complex.residue(ligid).nbr_atom_xyz();
		com_tgt /= lig_resids.size();

		for ( core::Size i=1; i<=pose_ligand.size(); ++i ) com_src += pose_ligand.residue(i).nbr_atom_xyz();
		com_src /= pose_ligand.size();

		numeric::xyzVector< core::Real > t_vec(com_tgt-com_src);

		for ( core::Size i=1; i<=pose_ligand.size(); ++i ) {
			for ( core::Size j=1; j<=pose_ligand.residue(i).natoms(); ++j ) {
				pose_ligand.set_xyz(core::id::AtomID( j,i), pose_ligand.residue(i).xyz(j)+t_vec );
			}
		}
	}

	core::Size startid, endid;
	startid = ( lig_resids[1] <= lig_resids.back())? lig_resids[1] : lig_resids.back() ;
	endid = ( lig_resids[1] > lig_resids.back())? lig_resids[1] : lig_resids.back() ;
	pose_complex.delete_residue_range_slow( startid, endid );

	core::Size rec_size(pose_complex.size());
	bool new_chain(false);
	core::pose::PoseOP pose_new( new core::pose::Pose(pose_complex) );
	append_pose_to_pose( *pose_new, pose_ligand, new_chain );

	if ( macrocycle_ligand_ ) {
		protocols::simple_moves::DeclareBondOP bond_close(new protocols::simple_moves::DeclareBond);
		bond_close->set(pose_new->size(),"C",rec_size+1,"N",false,false,0,0,false); //res1, atom1, res2, atom2, add_termini, run_kic, kic_res1, kic_res2, rebuild_fold_tree
		bond_close->apply(*pose_new);
	}


	return pose_new;
}

void
GALigandDock::apply( pose::Pose & pose )
{
	std::string prefix( basic::options::option[ basic::options::OptionKeys::out::prefix ]() );

	utility::vector1 < core::Size > lig_resids;
	if ( ligid_.length() > 0 ) {
		lig_resids = get_resnum_list_ordered( ligid_, pose );

		// make sure no polymer bonds across interface
		for ( auto i_lig : lig_resids ) {
			runtime_assert( pose.residue(i_lig).is_polymer() );
			if ( pose.residue(i_lig).has_lower_connect() ) {
				core::Size nextres = pose.residue(i_lig).connected_residue_at_resconn( pose.residue(i_lig).type().lower_connect_id() );
				runtime_assert( std::find (lig_resids.begin(), lig_resids.end(), nextres) != lig_resids.end() );
			}
			if ( pose.residue(i_lig).has_upper_connect() ) {
				core::Size nextres = pose.residue(i_lig).connected_residue_at_resconn( pose.residue(i_lig).type().upper_connect_id() );
				runtime_assert( std::find (lig_resids.begin(), lig_resids.end(), nextres) != lig_resids.end() );
			}
		}
	} else {
		get_ligand_resids(pose, lig_resids);
	}
	std::sort (lig_resids.begin(), lig_resids.end());

	if ( runmode_ == "eval" ) {
		eval_docked_pose(pose, lig_resids);
		return;
	}

	// fd: some of the code assumes lk_ball "interaction centers" are present.
	//     if disabled, turn on at very low weight (so score is unaffected
	//     but interaction centers are computed)
	if ( scfxn_->get_weight( core::scoring::lk_ball ) == 0 && scfxn_->get_weight( core::scoring::lk_ball_wtd ) == 0 ) {
		scfxn_->set_weight( core::scoring::lk_ball, 1e-9 );
	}

	if ( torsion_sampler_percentage_ > 0 ) {
		torsion_sampler_ = utility::pointer::make_shared<TorsionSampler>();
	}

	// [[1]] setup grid scoring
	GridScorerOP gridscore( utility::pointer::make_shared< GridScorer >( scfxn_ ));
	gridscore->set_voxel_spacing( grid_ );
	gridscore->set_bbox_padding( padding_ );
	gridscore->set_hash_gridding( hashsize_ );
	gridscore->set_hash_subgridding( subhash_ );
	gridscore->set_exact( exact_ );
	gridscore->set_debug( debug_ );
	gridscore->set_out_of_bound_e( grid_bound_penalty_ ); // should perhaps make gridscore parse tags...

	// prepare the grid but don't calculate scores yet
	TR << "Preparing grid using input pose " << std::endl;
	gridscore->prepare_grid( pose, lig_resids );

	// now figure out movable sidechains (using sidechains_ flag)
	utility::vector1< core::Size > movable_scs = get_movable_scs( pose, gridscore, lig_resids );

	// pass all the ligand residues for grid construction
	utility::vector1< core::conformation::Residue > rsds_to_build_grids;
	utility::vector1< bool > sconly;

	for ( auto i_lig : lig_resids ) {
		rsds_to_build_grids.push_back( pose.residue(i_lig) );
		sconly.push_back( false );
	}
	for ( auto i_mov : movable_scs ) {
		rsds_to_build_grids.push_back( pose.residue(i_mov) );
		sconly.push_back( true );
	}

	std::vector<core::Real> maxRads;
	use_mean_maxRad_ = use_mean_maxRad_ && ( multiple_ligands_.size()>1 or ligand_file_list_.size()>1 );

	if ( multiple_ligands_.size() > 0 ) {
		if ( lig_resids.size() > 1 ) {
			utility_exit_with_message("GALigandDock: multiple_ligands only works with single-res ligands!");
		}
		for ( core::Size ilig = 1; ilig <= multiple_ligands_.size(); ++ilig ) {
			TR << "Building multiple_ligands: "<< multiple_ligands_[ilig] << std::endl;
			core::conformation::ResidueOP ligand = core::conformation::get_residue_from_name( multiple_ligands_[ilig] );
			rsds_to_build_grids.push_back( *ligand );
			sconly.push_back( false );
			if ( use_mean_maxRad_ ) {
				numeric::xyzVector< core::Real > com(0,0,0);
				for ( core::Size i=1; i<=ligand->natoms(); ++i ) com += ligand->xyz(i);
				com /= ligand->natoms();
				core::Real maxRad = 0.0;
				for ( core::Size i=1; i<=ligand->natoms(); ++i ) maxRad = std::max( maxRad, com.distance(ligand->xyz(i)) );
				maxRads.push_back(maxRad);
				TR << "Building multiple_ligands: "<< multiple_ligands_[ilig] << " maxRad: " << maxRad << std::endl;
			}
		}
	}

	for ( std::string ligandfn:ligand_file_list_ ) {
		std::vector<core::Real> maxRads;
		numeric::xyzVector< core::Real > com(0,0,0);
		// if structure file is a pdb file
		if ( utility::endswith(ligandfn, "pdb") ) {
			core::pose::PoseOP ligand_pose;
			ligand_pose = core::import_pose::pose_from_file(ligandfn, core::import_pose::PDB_file);
			core::pose::initialize_disulfide_bonds(*ligand_pose);
			if ( !ligand_pose ) utility_exit_with_message("error! ligand pose is not loaded" );
			for ( core::Size resid=1; resid<=(*ligand_pose).size(); ++resid ) {
				if ( (*ligand_pose).residue(resid).is_virtual_residue() ) continue;
				rsds_to_build_grids.push_back( (*ligand_pose).residue(resid) );
				sconly.push_back( false );
			}
			if ( use_mean_maxRad_ ) {
				com = core::pose::get_center_of_mass( *ligand_pose );
				core::Real maxRad(0.0);
				for ( core::Size i=1; i<=(*ligand_pose).size(); ++i ) {
					for ( core::Size j=1; j<= (*ligand_pose).residue(i).natoms(); ++j ) {
						maxRad = std::max( maxRad, com.distance((*ligand_pose).residue(i).xyz(j)) );
					}
				}
				maxRads.push_back(maxRad);
				TR << "Building ligands from structure file: "<< ligandfn << " maxRad: " << maxRad << std::endl;
			}
		} //end reading pdb file

		// if structure file is a silent file
		if ( utility::endswith(ligandfn, "out") ) {
			core::io::silent::SilentFileOptions opts; // initialized from the command line
			core::io::silent::SilentFileData sfd( opts );
			bool success = sfd.read_file( ligandfn );
			if ( !success ) utility_exit_with_message("error when reading silent file: " + ligandfn );
			for ( auto iter=sfd.begin(); iter!=sfd.end(); ++iter ) {
				core::pose::PoseOP ligand_pose(new core::pose::Pose);
				iter->fill_pose( *ligand_pose );
				if ( !ligand_pose ) utility_exit_with_message("error! pose_ligand is not loaded" );
				for ( core::Size resid=1; resid<=(*ligand_pose).size(); ++resid ) {
					if ( (*ligand_pose).residue(resid).is_virtual_residue() ) continue;
					rsds_to_build_grids.push_back( (*ligand_pose).residue(resid) );
					sconly.push_back( false );
				}
				if ( use_mean_maxRad_ ) {
					com = core::pose::get_center_of_mass( *ligand_pose );
					core::Real maxRad(0.0);
					for ( core::Size i=1; i<=(*ligand_pose).size(); ++i ) {
						for ( core::Size j=1; j<= (*ligand_pose).residue(i).natoms(); ++j ) {
							maxRad = std::max( maxRad, com.distance((*ligand_pose).residue(i).xyz(j)) );
						}
					}
					maxRads.push_back(maxRad);
					TR << "Building ligands from structure file: "<< ligandfn << " maxRad: " << maxRad << std::endl;
				}
			}
		} //end reading silent file
	} // end reading structure file list

	if ( use_mean_maxRad_ ) {
		core::Real mean_maxRad = 0.0;
		for ( core::Real maxRad:maxRads ) mean_maxRad += maxRad;
		mean_maxRad /= (core::Real)maxRads.size();
		core::Real sum_square(0.0), stdev(0.0);
		for ( auto maxRad:maxRads ) {
			sum_square += (maxRad-mean_maxRad) * (maxRad-mean_maxRad);
		}
		stdev = std::sqrt(sum_square/(maxRads.size()-1));

		TR << "mean and stdev of maxRad: " << mean_maxRad <<
			", " << stdev << std::endl;
		multi_ligands_maxRad_ = mean_maxRad+stdev_multiplier_*stdev;
		TR << "Setting new maxRad with mean + " << stdev_multiplier_ <<
			" * " << "stdev: " << multi_ligands_maxRad_ << std::endl;
		gridscore->set_grid_dim_with_maxRad(multi_ligands_maxRad_);
	}

	if ( rsds_to_build_grids.size() != sconly.size() ) {
		utility_exit_with_message("error! Number of residues of building grids doesn't match sconly size" );
	}
	gridscore->get_grid_atomtypes( rsds_to_build_grids, sconly );
	// prepare the input pose
	idealize_and_repack_pose( pose, movable_scs, lig_resids );

	// compute the grid
	if ( !exact_ ) {
		TR << "Build grid for " << lig_resids.size() << " ligand residues and for "
			<< movable_scs.size() << " sidechain residues" << std::endl;

		TR << "Mobile sidechains: " << lig_resids[1];
		for ( core::Size i=2; i<=lig_resids.size(); ++i ) {
			TR << "+" << lig_resids[i];
		}
		for ( core::Size i=1; i<=movable_scs.size(); ++i ) {
			TR << "+" << movable_scs[i];
		}
		TR << std::endl;

		gridscore->calculate_grid( pose, lig_resids, movable_scs );
	} else {
		TR << "Skipping grid calculation." << std::endl;
	}

	// Prepare ligand aligner
	LigandAligner aligner;
	if ( reference_pool_ != "none" ) {
		scfxn_->score( pose ); // make sure scored to get neighbor graph
		TR << "Setting up Ligand aligner." << std::endl;
		aligner = setup_ligand_aligner( pose, lig_resids, movable_scs, rsds_to_build_grids, sconly );
	}

	if ( multiple_ligands_.size() > 0 ) {
		core::chemical::ResidueTypeSetCOP residue_set( core::chemical::ChemicalManager::get_instance()
			->residue_type_set( "fa_standard" ) );
		for ( core::Size ilig = 1; ilig <= multiple_ligands_.size(); ++ilig ) {
			auto start = std::chrono::steady_clock::now();
			TR << "===============================================================================" << std::endl;
			TR << " Starting " <<  multiple_ligands_[ilig]
				<< " (" << ilig << "/" << multiple_ligands_.size() << ")" << std::endl;
			TR << "===============================================================================" << std::endl;

			if ( ! residue_set->has_name( multiple_ligands_[ilig] ) ) {
				TR << "No residue info found, skip "<< multiple_ligands_[ilig] << std::endl;
				continue;
			}

			core::pose::PoseOP pose_working =
				make_starting_pose_for_virtual_screening( pose, lig_resids[1], multiple_ligands_[ilig] );

			if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.init."+std::to_string(ilig)+".pdb");

			if ( TR.Debug.visible() ) {
				for ( auto ligid: lig_resids ) {
					TR.Debug << "Ligand id: " << ligid << ", ligand restype: " << pose_working->residue(ligid).type().name() << std::endl;
				}
			}

			if ( premin_ligand_ ) {
				TR << "Preminimize ligand." << std::endl;
				premin_ligand( *pose_working, lig_resids );
				if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.premin."+std::to_string(ilig)+".pdb");
			}

			LigandConformer gene_initial( pose_working, lig_resids, movable_scs, freeze_ligand_backbone_, freeze_ligand_ );
			gene_initial.set_sample_ring_conformers( sample_ring_conformers_ );

			OutputStructureStore temporary_outputs;

			// take lowest score pose from each ligand
			pose = run_docking( gene_initial, gridscore, aligner, temporary_outputs );

			// store to remaining outputs
			core::Real score, rms, ligscore, recscore;
			std::string ligandname;
			score = (*scfxn_relax_)(pose);
			core::pose::getPoseExtraScore( pose, "ligscore", ligscore );
			core::pose::getPoseExtraScore( pose, "recscore", recscore );
			core::pose::getPoseExtraScore( pose, "lig_rms", rms );
			core::pose::getPoseExtraScore( pose, "ligandname", ligandname );
			//ignore ranking_prerelax
			remaining_outputs_.push( pose, score, rms, ligscore, recscore, 0, ligandname );
			auto end = std::chrono::steady_clock::now();
			std::chrono::duration<double> diff = end-start;
			TR << "GALigand Dock took " << (diff).count() << " seconds." << std::endl;
		}
		pose = *remaining_outputs_.pop();
	}


	if ( ligand_file_list_.size() > 0 ) {
		core::pose::Pose pose_complex(pose); //copy the initial pose
		core::Size struct_count(0);
		for ( core::Size i=1; i<=ligand_file_list_.size(); ++i ) {
			std::string ligandfn(ligand_file_list_[i]);
			if ( utility::endswith(ligandfn, "pdb") ) {
				auto start = std::chrono::steady_clock::now();
				TR << "===============================================================================" << std::endl;
				TR << " Starting " << ligandfn
					<< " (" << i << "/" << ligand_file_list_.size() << ")" << std::endl;
				TR << "===============================================================================" << std::endl;
				core::pose::PoseOP ligand_pose(new core::pose::Pose);
				ligand_pose = core::import_pose::pose_from_file(ligandfn, core::import_pose::PDB_file);
				if ( !ligand_pose ) utility_exit_with_message("error! pose_ligand is not loaded" );

				core::pose::PoseOP pose_working = replace_ligand(pose_complex, *ligand_pose);
				core::pose::initialize_disulfide_bonds( *pose_working );
				struct_count++;
				if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.init."+std::to_string(struct_count)+".pdb");

				// new pose may very likely have different ligand residue ids
				get_ligand_resids(*pose_working, lig_resids);
				std::sort (lig_resids.begin(), lig_resids.end());

				pose_working->energies().clear();
				pose_working->data().clear();
				if ( premin_ligand_ ) {
					TR << "Preminimize ligand." << std::endl;
					premin_ligand( *pose_working, lig_resids );
					if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.premin."+std::to_string(struct_count)+".pdb");
				}

				LigandConformer gene_initial( pose_working, lig_resids, movable_scs, freeze_ligand_backbone_, freeze_ligand_ );
				gene_initial.set_sample_ring_conformers( sample_ring_conformers_ );

				OutputStructureStore temporary_outputs;

				// take lowest score pose from each ligand
				pose = run_docking( gene_initial, gridscore, aligner, temporary_outputs );
				if ( pose.pdb_info() ) {
					if ( TR.Debug.visible() ) TR.Debug << "Fixing pdb info." << std::endl;
					core::Size newid(1);
					for ( core::Size ligid : lig_resids ) {
						pose.pdb_info()->chain( ligid, 'B' );
						pose.pdb_info()->number( ligid, newid );
						newid++;
					}
					pose.pdb_info()->obsolete( false );
				}

				// store to remaining outputs
				core::Real score, rms, ligscore, recscore;
				std::string ligandname;
				score = (*scfxn_relax_)(pose);
				core::pose::getPoseExtraScore( pose, "ligscore", ligscore );
				core::pose::getPoseExtraScore( pose, "recscore", recscore );
				core::pose::getPoseExtraScore( pose, "lig_rms", rms );
				core::pose::getPoseExtraScore( pose, "ligandname", ligandname );
				//ignore ranking_prerelax
				remaining_outputs_.push( pose, score, rms, ligscore, recscore, 0, ligandname );
				auto end = std::chrono::steady_clock::now();
				std::chrono::duration<double> diff = end-start;
				TR << "GALigand Dock took " << (diff).count() << " seconds." << std::endl;

			} else if ( utility::endswith(ligandfn, "out") ) {
				core::io::silent::SilentFileOptions opts; // initialized from the command line
				core::io::silent::SilentFileData sfd( opts );
				bool success = sfd.read_file( ligandfn );
				if ( !success ) utility_exit_with_message("error when reading silent file: " + ligandfn );
				for ( auto iter=sfd.begin(); iter!=sfd.end(); ++iter ) {
					auto start = std::chrono::steady_clock::now();
					core::pose::PoseOP ligand_pose(new core::pose::Pose);
					iter->fill_pose( *ligand_pose );
					if ( !ligand_pose ) {
						utility_exit_with_message("error! pose_ligand is not loaded" );
					}
					core::pose::PoseOP pose_working = replace_ligand(pose_complex, *ligand_pose);
					struct_count++;

					// new pose may very likely have different ligand residue ids
					get_ligand_resids(*pose_working, lig_resids);
					std::sort (lig_resids.begin(), lig_resids.end());

					LigandConformer gene_initial( pose_working, lig_resids, movable_scs, freeze_ligand_backbone_, freeze_ligand_ );
					gene_initial.set_sample_ring_conformers( sample_ring_conformers_ );

					OutputStructureStore temporary_outputs;

					// take lowest score pose from each ligand
					pose = run_docking( gene_initial, gridscore, aligner, temporary_outputs );
					if ( pose.pdb_info() ) {
						if ( TR.Debug.visible() ) TR.Debug << "Fixing pdb info." << std::endl;
						core::Size newid(1);
						for ( core::Size ligid : lig_resids ) {
							pose.pdb_info()->chain( ligid, 'B' );
							pose.pdb_info()->number( ligid, newid );
							newid++;
						}
						pose.pdb_info()->obsolete( false );
					}

					// store to remaining outputs
					core::Real score, rms, ligscore, recscore;
					std::string ligandname;
					score = (*scfxn_relax_)(pose);
					core::pose::getPoseExtraScore( pose, "ligscore", ligscore );
					core::pose::getPoseExtraScore( pose, "recscore", recscore );
					core::pose::getPoseExtraScore( pose, "lig_rms", rms );
					core::pose::getPoseExtraScore( pose, "ligandname", ligandname );
					//ignore ranking_prerelax
					remaining_outputs_.push( pose, score, rms, ligscore, recscore, 0, ligandname );
					auto end = std::chrono::steady_clock::now();
					std::chrono::duration<double> diff = end-start;
					TR << "GALigand Dock took " << (diff).count() << " seconds." << std::endl;
				}
			}
		} //end for loop ligand_file_list
		pose = *remaining_outputs_.pop();
	}

	if ( multiple_ligands_.size()==0 && ligand_file_list_.size()==0 ) {
		core::pose::PoseOP pose_working( new core::pose::Pose( pose ) );
		pose_working->energies().clear();
		pose_working->data().clear();
		if ( premin_ligand_ ) {
			TR << "Preminimize ligand." << std::endl;
			premin_ligand( *pose_working, lig_resids );
		}

		LigandConformer gene_initial( pose_working, lig_resids, movable_scs, freeze_ligand_backbone_, freeze_ligand_ );
		gene_initial.set_sample_ring_conformers( sample_ring_conformers_ );

		pose = run_docking( gene_initial, gridscore, aligner, remaining_outputs_ );
	}

}

core::pose::Pose
GALigandDock::run_docking( LigandConformer const &gene_initial,
	GridScorerOP gridscore,
	LigandAligner &aligner,
	OutputStructureStore &outputs )
{
	// load inputs & generate randomized starting points
	LigandConformers genes = generate_perturbed_structures( gene_initial, gridscore, protocol_[1].pool,
		aligner );

	// [[3]] main optimization cycle

	GAOptimizerOP optimizer = get_optimizer( gene_initial, gridscore );
	optimizer->run( genes );

	// trim genes for final min & report
	if ( nrelax_ <= genes.size() ) {
		genes.resize( nrelax_ );
	} else {
		TR << "Warning: Nrelax(" << nrelax_ << ") >= Npool(" << genes.size() << "); ignore." << std::endl;
	}

	// [[4]] optionally minimize a final generation with exact scores
	// FD this is starting to get ugly...
	bool finalbbscmin = (final_exact_minimize_.substr(0,4) == "bbsc");
	bool finalscmin = (final_exact_minimize_.substr(0,2) == "sc");
	bool finalLigandOnlyMin = (final_exact_minimize_.substr(0,10) == "ligandonly");
	//bool dualrelax = (finalbbscmin && final_exact_minimize_.length() > 8 && final_exact_minimize_.substr(5,9) == "dual");
	for ( core::Size i=1; i<=genes.size(); ++i ) {
		core::pose::PoseOP pose_tmp( new core::pose::Pose );
		genes[i].to_pose( pose_tmp );

		if ( TR.Debug.visible() ) {
			pose_tmp->dump_pdb("prefinalmin."+std::to_string(i)+".pdb");
		}

		utility::vector1< core::Size > const &movable_scs = genes[i].moving_scs();
		// idealize again... why do we need this again here?
		for ( core::Size i=1; i<=movable_scs.size(); ++i ) {
			utility::vector1< core::Real > chis_i = pose_tmp->residue(movable_scs[i]).chi();
			core::conformation::Residue newres( pose_tmp->residue_type(movable_scs[i]) , false);
			pose_tmp->replace_residue(movable_scs[i], newres, true);
			for ( core::Size j=1; j<=chis_i.size(); ++j ) {
				pose_tmp->set_chi( j, movable_scs[i], chis_i[j] );
			}
		}

		if ( final_solvate_ ) {
			final_solvate( genes[i], *pose_tmp );
		}

		if ( cartmin_lig_ ) {
			final_cartligmin(genes[i], *pose_tmp );
		}

		if ( TR.Debug.visible() ) {
			pose_tmp->dump_pdb("cartmin1."+std::to_string(i)+".pdb");
		}

		if ( finalbbscmin ) {
			core::Size N = 0;
			if ( final_exact_minimize_.length() > 4 ) {
				N = std::atoi( final_exact_minimize_.substr(4).c_str() );
			}
			//add_metal_ligand_constraints(*pose_tmp, genes[i].ligand_ids()[1]);
			final_exact_cartmin( N, genes[i], *pose_tmp );

			// post relax
			if ( cartmin_lig_ ) {
				//add_metal_ligand_constraints(*pose_tmp, genes[i].ligand_ids()[1]);
				final_cartligmin(genes[i], *pose_tmp);
			}
		} else if ( finalscmin ) {
			//add_metal_ligand_constraints(*pose_tmp, genes[i].ligand_ids()[1]);
			final_exact_scmin( genes[i], *pose_tmp );
			// post relax
			if ( cartmin_lig_ ) {
				final_cartligmin(genes[i], *pose_tmp );
			}
		} else if ( finalLigandOnlyMin ) {
			final_exact_ligmin( genes[i], *pose_tmp );
		}

		if ( TR.Debug.visible() ) {
			pose_tmp->dump_pdb("finalmin1."+std::to_string(i)+".pdb");
		}

		pose_tmp->energies().clear();
		pose_tmp->data().clear();
		core::Real score = (*scfxn_relax_)(*pose_tmp);

		//TR << "FINAL score for " << i << "-th"; scfxn_relax_->show(TR,*pose_tmp);

		core::Real rms = 0.0;
		if ( pose_native_ ) {
			if ( gene_initial.ligand_ids().size() == 1 ) {
				//fd  if the input ligand is the last residue, use the last residue of the _native_ as the ligand
				//fd  otherwise, match residue IDs
				core::Size lig_resno = gene_initial.ligand_ids()[1];
				core::Size native_lig = lig_resno;
				if ( lig_resno == pose_tmp->total_residue() ) {
					native_lig = pose_native_->total_residue();
				}
				rms = core::scoring::automorphic_rmsd(
					pose_native_->residue( native_lig ), pose_tmp->residue( lig_resno ), false );
			} else {
				rms = core::scoring::all_atom_rmsd_nosuper(
					*pose_native_, *pose_tmp,
					gene_initial.ligand_ids(), gene_initial.ligand_ids()
				);
			}
		}

		// report ligand-only energy
		core::Real ligscore = calculate_free_ligand_score( *pose_tmp, gene_initial.ligand_ids() );
		core::Real recscore = calculate_free_receptor_score( *pose_tmp, gene_initial.ligand_ids(), movable_scs, true );


		std::string ligandname = pose_tmp->residue(gene_initial.ligand_ids()[1]).name();
		for ( core::Size ires=2; ires <= gene_initial.ligand_ids().size(); ++ires ) {
			ligandname += "-"+pose_tmp->residue(gene_initial.ligand_ids()[ires]).name();
		}
		outputs.push( *pose_tmp, score, rms, ligscore, recscore, i, ligandname );
		if ( TR.Debug.visible() ) pose_tmp->dump_pdb("after_finalmin1."+std::to_string(i)+".pdb");
	}

	// lowest energy; use output class function instead of overrided one
	//core::pose::PoseOP pose = get_additional_output();
	core::pose::PoseOP pose = outputs.pop();

	if ( estimate_dG_ ) {
		EntropyEstimator entropy_estimator( scfxn_relax_, *pose, gene_initial.ligand_ids() );
		if ( runmode_ == "VSX" ) entropy_estimator.set_niter( 1000 );
		core::Real TdS = entropy_estimator.apply( *pose ); //comes out in energy unit; sign is opposite

		core::Real dH;
		std::string ligandname;
		core::pose::getPoseExtraScore( *pose, "dH", dH);
		core::pose::getPoseExtraScore( *pose, "ligandname", ligandname);
		core::Real dG = dH + TdS;

		TR << "Estimated Binding Free Energy (arbitrary energy unit, just for relative ranking)" << std::endl;
		TR << "dH: " << std::setw(6) << dH << std::endl;
		TR << "-T*dS: " << std::setw(6) << TdS << std::endl;
		TR << "Ligandname: "<< ligandname << " dG (dH-T*dS): " << dG << std::endl;
		core::pose::setPoseExtraScore( *pose, "-TdS", TdS );
		core::pose::setPoseExtraScore( *pose, "dG", dG );
		//core::pose::setPoseExtraScore( *pose, "ligandname", pose->residue(lig_resno).name() );
	}
	return *pose; // return lowest energy one
}

void
GALigandDock::eval_docked_pose_helper( core::pose::Pose &pose,
	utility::vector1< core::Size > const& lig_ids,
	utility::vector1< core::Size > &movable_scs
)
{
	if ( turnon_flexscs_at_relax_ ) {
		if ( !pose_native_ ) movable_scs = get_atomic_contacting_sidechains( pose, lig_ids, contact_distance_ );
		constraint_relax(pose, lig_ids, movable_scs);
	}
	TR << "Evaluating docked pose." << std::endl;
	auto time0 = std::chrono::steady_clock::now();
	core::Real complex_score = (*scfxn_relax_)(pose);
	core::Real ligscore = calculate_free_ligand_score( pose, lig_ids );
	core::Real recscore = calculate_free_receptor_score( pose, lig_ids, movable_scs, true );
	core::Real dH = complex_score - ligscore - recscore;
	EntropyEstimator entropy_estimator( scfxn_relax_, pose, lig_ids );
	entropy_estimator.set_niter( 2000 );
	core::Real TdS = entropy_estimator.apply( pose );
	core::Real dG = dH + TdS;
	core::pose::setPoseExtraScore( pose, "ligscore", ligscore );
	core::pose::setPoseExtraScore( pose, "recscore", recscore );
	core::pose::setPoseExtraScore( pose, "dH", dH );
	core::pose::setPoseExtraScore( pose, "-TdS", TdS );
	core::pose::setPoseExtraScore( pose, "dG", dG );

	std::string ligandname = pose.residue(lig_ids[1]).name();
	for ( core::Size ires=2; ires <= lig_ids.size(); ++ires ) {
		ligandname += "-"+pose.residue(lig_ids[ires]).name();
	}
	auto time1 = std::chrono::steady_clock::now();
	core::pose::setPoseExtraScore( pose, "ligandname", ligandname);
	std::chrono::duration<double> time_diff = time1-time0;
	TR << "Estimated Binding Free Energy (arbitrary energy unit, just for relative ranking)" << std::endl;
	TR << "dH: " << std::setw(6) << dH << std::endl;
	TR << "-T*dS: " << std::setw(6) << TdS << std::endl;
	TR << "Ligandname: "<< ligandname << " dG (dH-T*dS): " << dG << std::endl;
	TR << "Evaluating took " << time_diff.count() << " seconds." << std::endl;
}

void
GALigandDock::eval_docked_pose( core::pose::Pose &pose,
	utility::vector1< core::Size > const& lig_ids )
{
	core::pose::Pose pose_complex(pose);
	utility::vector1< core::Size > movable_scs;
	if ( pose_native_ ) {
		utility::vector1 < core::Size > lig_resids;
		get_ligand_resids(pose_complex, lig_resids);
		if ( turnon_flexscs_at_relax_ ) {
			movable_scs = get_atomic_contacting_sidechains( *pose_native_, lig_resids, contact_distance_ );
		}
	}

	if ( ligand_file_list_.size() > 0 ) {
		//copy the initial pose
		core::Size struct_count(0);
		for ( std::string ligandfn:ligand_file_list_ ) {
			if ( utility::endswith(ligandfn, "pdb") ) {
				core::pose::PoseOP ligand_pose(new core::pose::Pose);
				ligand_pose = core::import_pose::pose_from_file(ligandfn, core::import_pose::PDB_file);
				if ( !ligand_pose ) utility_exit_with_message("error! pose_ligand is not loaded" );

				core::pose::PoseOP pose_working = replace_ligand(pose_complex, *ligand_pose, false);
				struct_count++;
				if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.init."+std::to_string(struct_count)+".pdb");
				utility::vector1 < core::Size > lig_resids;
				get_ligand_resids(pose_complex, lig_resids);
				eval_docked_pose_helper(pose_complex, lig_resids, movable_scs);

			} else if ( utility::endswith(ligandfn, "out") ) {
				core::io::silent::SilentFileOptions opts; // initialized from the command line
				core::io::silent::SilentFileData sfd( opts );
				bool success = sfd.read_file( ligandfn );
				if ( !success ) utility_exit_with_message("error when reading silent file: " + ligandfn );
				for ( auto iter=sfd.begin(); iter!=sfd.end(); ++iter ) {
					core::pose::PoseOP ligand_pose(new core::pose::Pose);
					iter->fill_pose( *ligand_pose );
					if ( !ligand_pose ) {
						utility_exit_with_message("error! pose_ligand is not loaded" );
					}
					core::pose::PoseOP pose_working = replace_ligand(pose_complex, *ligand_pose, false);
					struct_count++;
					if ( TR.Debug.visible() ) pose_working->dump_pdb("pose.init."+std::to_string(struct_count)+".pdb");
					utility::vector1 < core::Size > lig_resids;
					get_ligand_resids(pose_complex, lig_resids);
					eval_docked_pose_helper(pose_complex, lig_resids, movable_scs);

				}
			}
		}
	} else {
		eval_docked_pose_helper(pose, lig_ids, movable_scs);
	}

}


utility::vector1< core::Size >
GALigandDock::get_movable_scs( core::pose::Pose const &pose,
	GridScorerCOP gridscore,
	utility::vector1 < core::Size > const &lig_resnos ) const
{
	utility::vector1< core::Size > movable_scs;
	std::set< core::Size > frozen_residues;
	if ( frozen_residues_ != nullptr ) {
		frozen_residues = core::select::get_residue_set_from_subset( frozen_residues_->apply(pose) );
	}

	if ( sidechains_.length() == 0 || sidechains_ == "none" || sidechains_ == "NONE" ) {
		; // do nothing
	} else if ( sidechains_.substr(0,4) == "auto" || sidechains_.substr(0,4) == "AUTO" ) {
		TR << "Detect flexible sidechains with mode: " << sidechains_ << ", "
			<< " edge_buffer=" << sc_edge_buffer_ << std::endl;

		for ( core::Size i=1; i<=pose.total_residue(); ++i ) {
			if ( std::find( lig_resnos.begin(), lig_resnos.end(), i ) != lig_resnos.end() ) continue;
			if ( frozen_residues.find(i) != frozen_residues.end() ) continue;

			if ( pose.residue(i).aa() == core::chemical::aa_ala || pose.residue(i).aa() == core::chemical::aa_gly ) continue;
			if ( pose.residue(i).type().has_variant_type( core::chemical::DISULFIDE ) ) continue;
			if ( !pose.residue(i).is_protein() ) continue; //skip anything not amino-acid

			if ( gridscore->is_residue_in_grid(pose.residue(i), 75.0, sc_edge_buffer_ ) ) { // 75 degree "angle buffer" check
				movable_scs.push_back( i );
			}
		}
	} else if ( sidechains_.substr(0,5) == "aniso" || sidechains_.substr(0,5) == "ANISO" ) {
		TR << "Detect flexible sidechains with mode: " << sidechains_ << ", "
			<< " edge_buffer = " << sc_edge_buffer_ << std::endl;

		// model ligand as ellipsoid
		// first compute mean and covariance of ligand position
		numeric::xyzVector< core::Real > mean(0.0,0.0,0.0);
		core::Size nAtms = 0;
		for ( auto lig_resno : lig_resnos ) {
			for ( core::Size iatm=1; iatm<=pose.residue(lig_resno).natoms(); ++iatm ) {
				mean += pose.residue(lig_resno).xyz(iatm);
			}
			nAtms += pose.residue(lig_resno).natoms();
		}
		mean /= nAtms;

		numeric::xyzMatrix< core::Real > covariance(0.0);
		for ( auto lig_resno : lig_resnos ) {
			for ( core::Size iatm=1; iatm<=pose.residue(lig_resno).natoms(); ++iatm ) {
				numeric::xyzVector< core::Real > diff = pose.residue(lig_resno).xyz(iatm) - mean;
				covariance += numeric::outer_product( diff , diff );
			}
		}
		covariance /= nAtms;

		// compute sorted eigenvectors
		numeric::xyzVector< core::Real > eigval, eigvalS;
		numeric::xyzMatrix< core::Real > eigvec, eigvecS;
		eigval = numeric::eigenvector_jacobi( covariance, 1e-4, eigvec );
		utility::vector1<core::Size> idx = {0,1,2};
		sort(idx.begin(), idx.end(), [&](core::Size i1, core::Size i2) {return eigval[i1] > eigval[i2];});
		TR << "idx: " << idx[1] << "," << idx[2] << "," << idx[3] << std::endl;
		for ( core::Size i=1; i<=3; ++i ) {
			eigvalS[i-1] = eigval[idx[i]];
			eigvecS.col(i, eigvec.col(idx[i]+1));  // .col is 1-indexed (for some reason)
		}

		// special case for planar ligands
		eigvalS[2] /= eigvalS[0];
		eigvalS[1] /= eigvalS[0];
		eigvalS[0] = 1.0;

		eigvalS[2] = std::max( eigvalS[2], 0.05 );
		eigvalS[1] = std::max( eigvalS[1], 0.05 );

		TR << "eigenvals = " << "[ " << eigvalS[0] << "," << eigvalS[1] << "," << eigvalS[2] << " ]" << std::endl;

		// for each residue, look and see if sidechain sphere intersects ellipsoid
		for ( core::Size i=1; i<=pose.total_residue(); ++i ) {
			if ( std::find( lig_resnos.begin(), lig_resnos.end(), i ) != lig_resnos.end() ) continue;
			if ( frozen_residues.find(i) != frozen_residues.end() ) continue;

			if ( pose.residue(i).aa() == core::chemical::aa_ala || pose.residue(i).aa() == core::chemical::aa_gly ) continue;
			if ( pose.residue(i).type().has_variant_type( core::chemical::DISULFIDE ) ) continue;
			if ( !pose.residue(i).is_protein() ) continue; //skip anything not amino-acid

			if ( gridscore->is_residue_in_grid(pose.residue(i), sc_edge_buffer_, eigvalS, eigvecS) ) {
				movable_scs.push_back( i );
			}
		}
	} else {
		// parse as residue numbers
		std::set<core::Size> resnums = core::pose::get_resnum_list( sidechains_, pose );
		for ( auto res_i : resnums ) {
			if ( std::find( lig_resnos.begin(), lig_resnos.end(), res_i ) != lig_resnos.end() ) continue;
			if ( frozen_residues.find(res_i) != frozen_residues.end() ) {
				TR.Warning << "Residue " << res_i << " is declared as both moving and frozen! Treating as frozen" << std::endl;
				continue;
			}
			if ( pose.residue(res_i).aa() == core::chemical::aa_ala
					|| pose.residue(res_i).aa() == core::chemical::aa_gly ) continue; // don't warn
			if ( pose.residue(res_i).type().has_variant_type( core::chemical::DISULFIDE ) ) continue; // don't warn

			if ( gridscore->is_residue_in_grid(pose.residue(res_i), 0.0, 0.0 ) ) { // for user-defined, ignore angle/dist checks
				movable_scs.push_back( res_i );
			} else {
				TR.Warning << "Residue " << res_i << " is not within grid!" << std::endl;
				TR.Warning << "Increase grid padding to include it!" << std::endl;
			}
		}
	}

	return movable_scs;
}

void
GALigandDock::idealize_and_repack_pose( core::pose::Pose &pose,
	utility::vector1< core::Size > const &movable_scs,
	utility::vector1< core::Size > const &lig_resnos ) const
{
	// do this only if flex sc case
	if ( movable_scs.size() == 0 ) return;

	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );
	core::Real score0 = (*scfxn_)(pose);

	for  ( auto lig_resno : lig_resnos ) {
		for ( core::Size i=1; i<=pose.residue(lig_resno).natoms(); ++i ) {
			pose.set_xyz( id::AtomID( i,lig_resno ), pose.residue(lig_resno).xyz(i)+250 );
		}
	}

	// a. idealize
	for ( core::Size i=1; i<=movable_scs.size(); ++i ) {
		utility::vector1< core::Real > chis_i = pose.residue(movable_scs[i]).chi();
		core::conformation::Residue newres( pose.residue_type(movable_scs[i]) , false);
		pose.replace_residue(movable_scs[i], newres, true);
		for ( core::Size j=1; j<=chis_i.size(); ++j ) {
			pose.set_chi( j, movable_scs[i], chis_i[j] );
			mm->set_chi( movable_scs[i], true );
		}
	}

	// opt hydrogen as apo + minimize; only for "docking" case
	// b.2 optimize hydrogen
	if ( optimize_input_H_ ) {
		TR << "Re-optimizing hydrogens in whole structure." << std::endl;
		core::pack::task::PackerTaskOP task = core::pack::task::TaskFactory::create_packer_task( pose );
		task->initialize_from_command_line();
		task->or_optimize_h_mode( true );
		task->or_include_current( true );
		task->or_flip_HNQ( true );
		task->or_multi_cool_annealer( true );
		core::pack::pack_rotamers( pose, *scfxn_, task );
	}

	// b.3 minimize
	core::optimization::AtomTreeMinimizer minimizer;
	core::optimization::MinimizerOptions options( "lbfgs_armijo", 0.0001, true , false );
	options.max_iter(50);
	core::Real score1 = (*scfxn_)(pose);
	minimizer.run( pose, *mm, *scfxn_, options );
	core::Real score2 = (*scfxn_)(pose);
	TR << "Sidechain idealize score: " << score0 << "->" << score1 << "->" << score2 << std::endl;

	for  ( auto lig_resno : lig_resnos ) {
		for ( core::Size i=1; i<=pose.residue(lig_resno).natoms(); ++i ) {
			pose.set_xyz( id::AtomID( i,lig_resno ), pose.residue(lig_resno).xyz(i)-250 );
		}
	}
}

// generate a pose for virtual screening with the ligand corresponding to ligand_name
core::pose::PoseOP
GALigandDock::make_starting_pose_for_virtual_screening( core::pose::Pose const &pose,
	core::Size const &lig_resno,
	std::string const ligand_name
) const
{
	// will it die here if ligand name not exist in residue type set??
	core::conformation::ResidueOP ligand = core::conformation::get_residue_from_name( ligand_name );

	core::Size jumpid = pose.fold_tree().get_jump_that_builds_residue( lig_resno );
	core::kinematics::Jump ligjump = pose.jump( jumpid );
	numeric::xyzVector< core::Real > T = ligjump.get_translation();

	core::pose::PoseOP pose_working( new core::pose::Pose( pose ) );
	pose_working->replace_residue( lig_resno, *ligand, false );

	ligjump.set_translation( T ); // no change in orientation; is this line necessary?
	pose_working->set_jump( jumpid, ligjump );

	return pose_working;
}

Real
GALigandDock::calculate_free_receptor_score(
	core::pose::Pose pose, // call by value
	utility::vector1< core::Size > const& lig_resnos,
	utility::vector1< core::Size > const& moving_scs,
	bool simple
) const
{
	if ( pose.size() == 1 ) return 0.0; // ligand-only

	// necessary for memory issue?
	pose.energies().clear();
	pose.data().clear();

	// delete ligand
	core::Size startid, endid;
	startid = ( lig_resnos[1] <= lig_resnos.back())? lig_resnos[1] : lig_resnos.back() ;
	endid = ( lig_resnos[1] > lig_resnos.back())? lig_resnos[1] : lig_resnos.back() ;
	pose.delete_residue_range_slow(startid, endid);

	if ( simple ) {
		return (*scfxn_relax_)(pose);
	}

	core::pack::task::PackerTaskOP task = core::pack::task::TaskFactory::create_packer_task( pose );
	task->initialize_from_command_line();
	task->or_optimize_h_mode( false ); // to reduce noise
	task->or_flip_HNQ( false ); // to reduce noise

	utility::vector1< core::Size > moving_his_scs;
	for ( core::Size ires = 1; ires <= moving_scs.size(); ++ires ) {
		if ( pose.residue(moving_scs[ires]).aa() == core::chemical::aa_his ) moving_his_scs.push_back( moving_scs[ires] );
	}
	task->or_fix_his_tautomer( moving_his_scs, true ); // to reduce noise
	//task->or_multi_cool_annealer( true );

	core::pack::task::TaskFactoryOP tf( new core::pack::task::TaskFactory() );
	tf->modify_task( pose, task );

	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );
	for ( core::Size j=1; j<=moving_scs.size(); ++j ) {
		core::Size resid = moving_scs[j];
		mm->set_chi( resid, true );
	}

	// let's use hard-coded scopt schedule for now...
	protocols::relax::FastRelax relax;
	relax.set_task_factory( tf );

	// sometimes just stuck here wasting huge memory... why is it?

	std::vector< std::string > lines;
	if ( fast_relax_script_file_ != "" ) {
		TR << "==== Use FastRelax script: " << fast_relax_script_file_ << std::endl;
		relax=protocols::relax::FastRelax( scfxn_relax_, fast_relax_script_file_ );
	} else if ( fast_relax_lines_.size() > 0 ) {
		lines = fast_relax_lines_;
	} else {
		TR << "==== Use FastRelax hardcoded. "<< std::endl;
		lines.push_back( "switch:torsion" );
		lines.push_back( "repeat 3" );
		lines.push_back( "ramp_repack_min 0.02 0.01 1.0 50" );
		lines.push_back( "ramp_repack_min 1.0  0.00001 0.0 50" );
		lines.push_back( "accept_to_best" );
		lines.push_back( "endrepeat" );
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_relax_ );
	}
	relax.set_movemap( mm );
	relax.set_movemap_disables_packing_of_fixed_chi_positions( true );
	relax.apply( pose );

	return (*scfxn_relax_)(pose);
}

Real
GALigandDock::calculate_free_ligand_score(
	core::pose::Pose pose_ref, // call by value
	utility::vector1< core::Size > const& lig_resnos
) const {
	// make a ligand-only pose; root ligand on virtual if no residues to anchor jump
	utility::vector1< core::Size > freeligresids;
	core::pose::PoseOP pose( new core::pose::Pose );
	pose->append_residue_by_jump( pose_ref.residue(lig_resnos[1]), 0 );
	freeligresids.push_back(pose->size());
	for ( core::Size i=2; i<=lig_resnos.size(); ++i ) {
		pose->append_residue_by_bond( pose_ref.residue(lig_resnos[i]) );
		freeligresids.push_back(pose->size());
	}
	core::pose::initialize_disulfide_bonds( *pose );
	core::pose::addVirtualResAsRoot(*pose);

	// optimize slightly...
	core::scoring::ScoreFunctionOP scfxn_ligmin( scfxn_relax_->clone() );
	scfxn_ligmin->set_weight( core::scoring::coordinate_constraint, 1.0 );

	if ( macrocycle_ligand_ ) {
		scfxn_ligmin->set_weight( core::scoring::atom_pair_constraint, 1.0 );
		scfxn_ligmin->set_weight( core::scoring::angle_constraint, 1.0 );
		scfxn_ligmin->set_weight( core::scoring::dihedral_constraint, 1.0 );
		add_macrocycle_constraints( *pose, freeligresids );
	}

	//core::pose::Pose pose_premin( *pose );
	core::id::AtomID anchorid( 1, pose->fold_tree().root() );
	for ( core::Size ires = 1; ires < pose->total_residue(); ++ires ) {
		for ( core::Size iatm = 1; iatm <= pose->residue(ires).natoms(); ++iatm ) {
			core::id::AtomID atomid( iatm, ires );
			core::Vector const &xyz = pose->xyz( atomid );
			core::scoring::func::FuncOP fx( new core::scoring::func::HarmonicFunc( 0.0, 1.0 ) );
			pose->add_constraint( core::scoring::constraints::ConstraintCOP
				( core::scoring::constraints::ConstraintOP
				( new core::scoring::constraints::CoordinateConstraint( atomid, anchorid, xyz, fx ) )));
		}
	}

	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( true ); mm->set_chi( true ); mm->set_jump( true );

	protocols::minimization_packing::MinMoverOP min_mover =
		protocols::minimization_packing::MinMoverOP
		( new protocols::minimization_packing::MinMover( mm, scfxn_ligmin, "linmin", 0.01, true ) );
	min_mover->max_iter( 30 );
	min_mover->apply( *pose );
	//turn off coordinate constraint in the actual scoring
	scfxn_ligmin->set_weight( core::scoring::coordinate_constraint, 0.0 );
	Real ligandscore = scfxn_ligmin->score( *pose );

	return ligandscore;
}

void
GALigandDock::premin_ligand(
	core::pose::Pose &pose, utility::vector1 < core::Size > const &lig_resnos
) const {
	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );

	for ( auto lig_resno : lig_resnos ) {
		mm->set_chi( lig_resno, true );
		mm->set_bb( lig_resno, true );
	}

	core::Real w_cart = (*scfxn_relax_)[ core::scoring::cart_bonded ];
	core::Real w_proclose = (*scfxn_relax_)[ core::scoring::pro_close ];
	core::Real w_atr = (*scfxn_relax_)[ core::scoring::fa_atr ];
	core::Real w_rep = (*scfxn_relax_)[ core::scoring::fa_rep ];
	if ( w_cart < 1.0e-6 ) {
		scfxn_relax_->set_weight( core::scoring::cart_bonded, 0.5 );
		scfxn_relax_->set_weight( core::scoring::pro_close, 0.0 );
	}
	// turn non-bonded terms off
	scfxn_relax_->set_weight( core::scoring::fa_atr, 0.0 );
	scfxn_relax_->set_weight( core::scoring::fa_rep, 0.0 );

	core::optimization::CartesianMinimizer minimizer;
	core::optimization::MinimizerOptions options( "lbfgs_armijo", 0.0001, true , false );
	options.max_iter(50);
	minimizer.run( pose, *mm, *scfxn_relax_, options );

	// reset
	scfxn_relax_->set_weight( core::scoring::cart_bonded, w_cart );
	scfxn_relax_->set_weight( core::scoring::pro_close, w_proclose );
	scfxn_relax_->set_weight( core::scoring::fa_atr, w_atr );
	scfxn_relax_->set_weight( core::scoring::fa_rep, w_rep );
}

void
GALigandDock::apply_coord_cst_to_sctip( core::pose::PoseOP pose,
	utility::vector1< core::Size > const& moving_scs
) const
{

	TR << "Applying coordinate cst to sidechain tips:" << std::endl;
	std::string reportline( "CSTATMS " );
	utility::vector1< core::id::AtomID > cstatoms;
	for ( core::Size isc = 1; isc <= moving_scs.size(); ++isc ) {
		std::string aname;
		core::Size ires( moving_scs[isc] );
		core::chemical::AA aa( pose->aa( ires ) );
		switch ( aa ){
		case core::chemical::aa_cys : { aname = "SG"; break; }
		case core::chemical::aa_asp : { aname = "CG"; break; }
		case core::chemical::aa_glu : { aname = "CD"; break; }
		case core::chemical::aa_phe : { aname = "CZ"; break; }
		case core::chemical::aa_his : { aname = "NE2"; break; }
		case core::chemical::aa_ile : { aname = "CG2"; break; }
		case core::chemical::aa_lys : { aname = "NZ"; break; }
		case core::chemical::aa_leu : { aname = "CD1"; break; }
		case core::chemical::aa_met : { aname = "CE"; break; }
		case core::chemical::aa_asn : { aname = "OD1"; break; }
		case core::chemical::aa_gln : { aname = "OE1"; break; }
		case core::chemical::aa_arg : { aname = "CZ"; break; }
		case core::chemical::aa_ser : { aname = "OG"; break; }
		case core::chemical::aa_thr : { aname = "OG1"; break; }
		case core::chemical::aa_val : { aname = "CG1"; break; }
		case core::chemical::aa_trp : { aname = "CZ2"; break; }
		case core::chemical::aa_tyr : { aname = "OH"; break; }
		default : continue;
		}

		reportline += " "+std::to_string(ires)+"."+aname;
		core::Size iatm( pose->residue(ires).atom_index(aname) );
		core::id::AtomID atomid( iatm, ires );
		cstatoms.push_back( atomid );
	}
	TR << reportline << std::endl;

	//gz: The following use of CoordinateConstraint is incorrect
	// but we are not using this function at this point
	// fixing it in the future once we need to use this function
	for ( core::Size i = 1; i <= cstatoms.size(); ++i ) {
		core::id::AtomID const& atomid = cstatoms[i];
		core::Vector const &xyz = pose->xyz( atomid );
		core::scoring::func::FuncOP fx( new core::scoring::func::HarmonicFunc( 0.0, 1.0 ) );
		pose->add_constraint( core::scoring::constraints::ConstraintCOP(
			core::scoring::constraints::ConstraintOP(
			new core::scoring::constraints::CoordinateConstraint( atomid, atomid, xyz, fx )
			)));
	}
}

void
GALigandDock::add_macrocycle_constraints(
	core::pose::Pose &pose,
	utility::vector1< core::Size > const &ligids
) const {
	runtime_assert( ligids[1] < ligids.back() );
	//distance constraint
	core::Size atomno1 = pose.residue_type(ligids.back()).atom_index("CA");
	core::Size atomno2 = pose.residue_type(ligids.back()).atom_index("C");
	core::Size atomno3 = pose.residue_type(ligids[1]).atom_index("N");
	core::Size atomno4 = pose.residue_type(ligids[1]).atom_index("CA");
	numeric::xyzVector< core::Real > xyz1 = pose.residue(ligids.back()).xyz(atomno1);
	numeric::xyzVector< core::Real > xyz2 = pose.residue(ligids.back()).xyz(atomno2);
	numeric::xyzVector< core::Real > xyz3 = pose.residue(ligids[1]).xyz(atomno3);
	numeric::xyzVector< core::Real > xyz4 = pose.residue(ligids[1]).xyz(atomno4);

	protocols::simple_moves::DeclareBondOP bond_close(new protocols::simple_moves::DeclareBond);
	bond_close->set(ligids.back(),"C",ligids[1],"N",false,false,0,0,false); //res1, atom1, res2, atom2, add_termini, run_kic, kic_res1, kic_res2, rebuild_fold_tree
	bond_close->apply(pose);

	core::Real dist = pose.residue(ligids[1]).xyz(atomno3).distance(pose.residue(ligids.back()).xyz(atomno2));
	core::scoring::func::FuncOP fx( new core::scoring::func::HarmonicFunc( dist, 0.01 ) );
	pose.add_constraint( core::scoring::constraints::ConstraintCOP(
		new core::scoring::constraints::AtomPairConstraint( core::id::AtomID(atomno2,ligids.back()), core::id::AtomID(atomno3,ligids[1]), fx )
		)
	);

	//angle constraint
	core::Real angle1 = numeric::angle_radians(xyz1, xyz2, xyz3);
	core::Real angle2 = numeric::angle_radians(xyz2, xyz3, xyz4);
	core::scoring::func::FuncOP fx1( new core::scoring::func::HarmonicFunc( angle1, 0.01 ) );
	pose.add_constraint( core::scoring::constraints::ConstraintCOP(
		new core::scoring::constraints::AngleConstraint( core::id::AtomID(atomno1,ligids.back()),
		core::id::AtomID(atomno2,ligids.back()),
		core::id::AtomID(atomno3,ligids[1]), fx1 )
		)
	);

	core::scoring::func::FuncOP fx2( new core::scoring::func::HarmonicFunc( angle2, 0.01 ) );
	pose.add_constraint( core::scoring::constraints::ConstraintCOP(
		new  core::scoring::constraints::AngleConstraint( core::id::AtomID(atomno2,ligids.back()),
		core::id::AtomID(atomno3,ligids[1]),
		core::id::AtomID(atomno4,ligids[1]), fx2 )
		)
	);

	core::Real torsion1 = numeric::dihedral_radians(xyz1, xyz2, xyz3, xyz4);
	core::scoring::func::FuncOP fx3( new core::scoring::func::CircularHarmonicFunc( torsion1, 0.01 ) );
	pose.add_constraint( core::scoring::constraints::ConstraintCOP(
		new core::scoring::constraints::DihedralConstraint(core::id::AtomID(atomno1,ligids.back()),
		core::id::AtomID(atomno2,ligids.back()),
		core::id::AtomID(atomno3,ligids[1]),
		core::id::AtomID(atomno4,ligids[1]), fx3 )
		)
	);

}

// final optimziation cycle with sidechain flexibility
void
GALigandDock::final_exact_cartmin(
	core::Size nneigh,
	LigandConformer & gene,
	core::pose::Pose &pose
	//bool dualrelax
) {
	std::set< core::Size > frozen_residues;
	if ( frozen_residues_ != nullptr ) {
		frozen_residues = core::select::get_residue_set_from_subset( frozen_residues_->apply(pose) );
	}
	/////
	// (0) let's clone the score function so that any changes to the score function here won't mess up the global score function
	core::scoring::ScoreFunctionOP scfxn_cartmin = scfxn_relax_->clone();
	/////
	// (1) setup movemap
	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );
	if ( pose.size() == 1 ) {
		mm->set_chi( true ); // ligand-only; virtual root
	} else {
		core::Size lig_jump = gene.get_jumpid();
		mm->set_jump( lig_jump, true );
		for ( auto res : gene.ligand_ids() ) {
			mm->set_chi( res, true );
			mm->set_bb( res, true );
		}

		for ( core::Size j=1; j<=gene.moving_scs().size(); ++j ) {
			int resid = (int)gene.moving_scs()[j];
			for ( int k=-((int)nneigh); k<=((int)nneigh); ++k ) {
				if ( resid+k < 1 || resid+k >= (int)pose.total_residue() ) continue;
				if ( !pose.residue(resid+k).is_protein() ) continue;

				if ( frozen_residues.count(resid+k) == 0 ) {
					mm->set_bb( resid+k, true );
					if ( k==0 ) {
						mm->set_chi( resid+k, true );
					}
				}
			}
		}
	}

	// waters, hetmol
	for ( core::Size j=1; j<=pose.total_residue(); ++j ) {
		if ( pose.residue_type(j).is_water() && move_water_ ) {
			if ( frozen_residues.count(j) == 0 ) {
				core::Size wjump = pose.fold_tree().get_jump_that_builds_residue( j );
				mm->set_jump( wjump, true );
				mm->set_chi( j, true );
			}
		}
	}

	std::vector< std::string > lines;

	/////
	// (2) setup constraints if cst weight is on
	core::Size MINSEQSEP=2;
	core::Real MAXDIST=5.0, TOPOUT_WIDTH=2.0;
	core::Real TOPOUT_WT=1/(TOPOUT_WIDTH*TOPOUT_WIDTH); // max penalty per cst = 1
	core::Size addedCsts = 0;

	if ( scfxn_cartmin->get_weight( core::scoring::atom_pair_constraint ) != 0 ) {
		TR << "Adding constraints to pose before final relax..." << std::endl;

		for ( core::Size j=1; j<pose.size(); ++j ) {
			for ( core::Size k=j+MINSEQSEP; k<pose.size(); ++k ) {
				if ( !mm->get_bb(j) && !mm->get_bb(k) ) continue;

				for ( core::Size jatm=1; jatm<pose.residue_type(j).nheavyatoms(); ++jatm ) {
					for ( core::Size katm=1; katm<pose.residue_type(k).nheavyatoms(); ++katm ) {

						core::Real dist = pose.residue(j).xyz(jatm).distance(
							pose.residue(k).xyz(katm)
						);

						if ( dist > MAXDIST ) continue;
						using namespace core::scoring::func;
						FuncOP fx( new ScalarWeightedFunc( 1.0, FuncOP( new TopOutFunc( TOPOUT_WT, dist, TOPOUT_WIDTH ) ) ) );
						pose.add_constraint(
							core::scoring::constraints::ConstraintCOP( scoring::constraints::ConstraintOP(
							new core::scoring::constraints::AtomPairConstraint(
							core::id::AtomID(jatm,j), core::id::AtomID(katm,k), fx ) ) )
						);
						addedCsts++;
					}
				}
			}
		}
		TR << "Added " << addedCsts << " atmpair constraints to the pose." << std::endl;
	}
	if ( scfxn_cartmin->get_weight( core::scoring::coordinate_constraint ) != 0 ) {
		TR << "Adding constraints to protein CAs before final relax..." << std::endl;
		core::pose::addVirtualResAsRoot(pose); //gz: add root virtual residue for CoordinateConstraint
		core::id::AtomID anchoratomid(1, pose.fold_tree().root() );

		for ( core::Size j=1; j<pose.size(); ++j ) {
			if ( !mm->get_bb(j) ) continue;
			if ( pose.residue(j).aa() == core::chemical::aa_vrt ) continue;
			if ( !pose.residue(j).is_protein() ) continue;

			core::id::AtomID atomid(pose.residue(j).atom_index("CA"),j);
			core::Vector const &xyz = pose.xyz( atomid );
			using namespace core::scoring::func;
			FuncOP fx( new ScalarWeightedFunc( 1.0, FuncOP( new HarmonicFunc( 0.0, 1.0 ) ) ) );
			pose.add_constraint(
				core::scoring::constraints::ConstraintCOP( scoring::constraints::ConstraintOP(
				new core::scoring::constraints::CoordinateConstraint
				( atomid, anchoratomid, xyz, fx ) ) ) );
			addedCsts++;
		}
		TR << "Added " << addedCsts << " coord constraints to the pose." << std::endl;
	}
	if ( scfxn_cartmin->get_weight( core::scoring::cart_bonded ) == 0 ) {
		scfxn_cartmin->set_weight( core::scoring::cart_bonded, 0.5 );
		scfxn_cartmin->set_weight( core::scoring::pro_close, 0.0 );
		TR << "scfxn_relax is not properly set for cartmin! setting cart_bonded=0.5 pro_close=0.0." << std::endl;
	}

	// ligand backbone is turned on in movemap, so we need to check it the ligand is a macrocyle and add proper constraints for it.
	if ( macrocycle_ligand_ ) {
		scfxn_cartmin->set_weight( core::scoring::atom_pair_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::angle_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::dihedral_constraint, 1.0 );
		add_macrocycle_constraints( pose, gene.ligand_ids() );
	}

	protocols::relax::FastRelax relax;
	if ( fast_relax_script_file_ != "" ) {
		TR << "==== Use FastRelax script: " << fast_relax_script_file_ << std::endl;
		relax=protocols::relax::FastRelax( scfxn_cartmin, fast_relax_script_file_ );
	} else if ( fast_relax_lines_.size() > 0 ) {
		lines = fast_relax_lines_;
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_cartmin );
	} else {
		TR << "==== Use FastRelax hardcoded. "<< std::endl;
		lines.push_back( "switch:cartesian" );
		lines.push_back( "repeat 3" );
		lines.push_back( "ramp_repack_min 0.02 0.01 1.0 50" );
		lines.push_back( "ramp_repack_min 1.0  0.00001 0.0 50" );
		lines.push_back( "accept_to_best" );
		lines.push_back( "endrepeat" );
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_cartmin );
	}

	relax.set_movemap( mm );
	relax.set_movemap_disables_packing_of_fixed_chi_positions( true );
	relax.apply( pose );
	// delete added root virtual residue
	if ( pose.residue( pose.fold_tree().root() ).aa() == core::chemical::aa_vrt ) {
		pose.delete_residue_slow( pose.fold_tree().root() );
	}
	TR << "final_cartmin: score after relax: ";
	TR << (*scfxn_cartmin)(pose) <<std::endl;
	//should we keep all the constraints in the pose?
	pose.remove_constraints();
}

// final optimziation cycle with sc flexibility only
void
GALigandDock::final_exact_scmin(
	LigandConformer const & gene,
	core::pose::Pose &pose
) {

	utility::vector1< core::Size > contact_scs;
	if ( turnon_flexscs_at_relax_ ) {
		contact_scs = get_atomic_contacting_sidechains( pose, gene.ligand_ids(), contact_distance_ );
		TR << "Redefined flexible sidechains: ";
		for ( core::Size ires = 1; ires < contact_scs.size(); ++ires ) TR << contact_scs[ires] << "+";
		if ( contact_scs.size() > 0 ) TR << contact_scs[contact_scs.size()];
		TR << std::endl;
	} else {
		contact_scs = gene.moving_scs();
	}

	std::set< core::Size > frozen_residues;
	if ( frozen_residues_ != nullptr ) {
		frozen_residues = core::select::get_residue_set_from_subset( frozen_residues_->apply(pose) );
	}

	// movemap for repacking
	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );
	if ( pose.size() == 1 ) {
		mm->set_chi( true ); // ligand-only; virtual root
	} else {
		core::Size lig_jump = gene.get_jumpid();
		mm->set_jump( lig_jump, true );
		for ( auto resid : gene.ligand_ids() ) {
			mm->set_chi( resid, true );
		}
		for ( auto resid : contact_scs ) {
			if ( frozen_residues.count(resid) == 0 ) {
				mm->set_chi( resid, true );
			}
		}
	}

	// waters, hetmol
	for ( core::Size j=1; j<=pose.total_residue(); ++j ) {
		if ( (pose.residue_type(j).is_water() && move_water_) ) {
			if ( frozen_residues.count(j) == 0 ) {
				mm->set_chi( j, true );
			}
		}
	}

	// quick relax
	protocols::relax::FastRelax relax;
	std::vector< std::string > lines;
	if ( pre_optH_relax_ ) {
		TR << "==== quick relax after optH, Use FastRelax hardcoded. " << std::endl;
		lines.push_back( "switch:torsion" );
		lines.push_back( "repeat 1" );
		lines.push_back( "ramp_repack_min 0.02 0.01 1.0 50" );
		lines.push_back( "ramp_repack_min 1.0  0.00001 0.0 50" );
		lines.push_back( "accept_to_best" );
		lines.push_back( "endrepeat" );
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_relax_ );

		relax.set_movemap( mm );
		relax.set_movemap_disables_packing_of_fixed_chi_positions(true);
		relax.apply( pose );

		TR << "final_scmin: score after relax1: " << (*scfxn_relax_)(pose) <<std::endl;
	}

	// opt-H: re-optimize full pose!
	//if( optimize_input_H_ ){
	TR << "Re-optimizing hydrogens in whole structure." << std::endl;
	core::pack::task::PackerTaskOP task = core::pack::task::TaskFactory::create_packer_task( pose );
	task->initialize_from_command_line();
	task->or_optimize_h_mode( true );
	task->or_include_current( true );
	task->or_flip_HNQ( true );
	task->or_multi_cool_annealer( true );
	core::pack::pack_rotamers( pose, *scfxn_relax_, task );
	//}

	// main relax after optH
	lines.clear();
	if ( fast_relax_script_file_ != "" ) {
		TR << "==== Use FastRelax script: " << fast_relax_script_file_ << std::endl;
		relax = protocols::relax::FastRelax( scfxn_relax_, fast_relax_script_file_ );
	} else if ( fast_relax_lines_.size() > 0 ) {
		lines = fast_relax_lines_;
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_relax_ );
	} else {
		TR << "==== Use FastRelax hardcoded. " << std::endl;
		lines.push_back( "switch:torsion" );
		lines.push_back( "repeat 3" );
		lines.push_back( "ramp_repack_min 0.02 0.01 1.0 50" );
		lines.push_back( "ramp_repack_min 1.0  0.00001 0.0 50" );
		lines.push_back( "accept_to_best" );
		lines.push_back( "endrepeat" );
		relax.set_script_from_lines( lines );
		relax.set_scorefxn( scfxn_relax_ );
	}
	relax.set_movemap( mm );
	relax.set_movemap_disables_packing_of_fixed_chi_positions(true);
	relax.apply( pose );


	TR << "final_scmin: score after relax2: " << (*scfxn_relax_)(pose) <<std::endl;

}

// final optimziation cycle with ligand flexibility only
void
GALigandDock::final_exact_ligmin(
	LigandConformer const & gene,
	core::pose::Pose &pose
) {
	if ( redefine_flexscs_at_relax_ ) {
		utility_exit_with_message("Redefining flexible sidechain doesn't work with final_exact_ligmin");
	}

	core::kinematics::MoveMapOP mmlig( new core::kinematics::MoveMap );
	mmlig->set_bb( false ); mmlig->set_chi( false ); mmlig->set_jump( false );
	core::Size lig_jump = gene.get_jumpid();
	mmlig->set_jump( lig_jump, true );
	if ( pose.size() == 1 ) {
		mmlig->set_chi( true ); // ligand-only; virtual root
	} else {
		for ( auto resid : gene.ligand_ids() ) {
			mmlig->set_chi( resid, true );
			mmlig->set_bb( resid, true );
		}
	}

	core::optimization::CartesianMinimizer minimizer;
	core::optimization::MinimizerOptions options( "lbfgs_armijo", 0.0001, true , false );
	options.max_iter(50);

	core::scoring::ScoreFunctionOP scfxn_cartmin = scfxn_relax_->clone();

	if ( macrocycle_ligand_ ) {
		scfxn_cartmin->set_weight( core::scoring::atom_pair_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::angle_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::dihedral_constraint, 1.0 );
		add_macrocycle_constraints( pose, gene.ligand_ids() );
	}

	minimizer.run( pose, *mmlig, *scfxn_cartmin, options );
	// This is a final cart ligand minimization, so I remove the constraints in the end.
	// But should we keep all the constraints in the pose?
	pose.remove_constraints();
}

// final optimziation cycle with ligand flexibility only
void
GALigandDock::final_cartligmin(
	LigandConformer const & gene,
	core::pose::Pose &pose
) {
	// Nov08!! (turn this off for pre-Nov08)
	utility::vector1< core::Size > contact_scs;
	if ( redefine_flexscs_at_relax_ ) {
		contact_scs = get_atomic_contacting_sidechains( pose, gene.ligand_ids(), contact_distance_ );
		TR << "Redefined flexible sidechains: ";
		for ( core::Size ires = 1; ires < contact_scs.size(); ++ires ) TR << contact_scs[ires] << "+";
		if ( contact_scs.size() > 0 ) TR << contact_scs[contact_scs.size()];
		TR << std::endl;
	} else {
		contact_scs = gene.moving_scs();
	}

	// setup for lig-only cartmin
	core::Real w_cart = (*scfxn_relax_)[ core::scoring::cart_bonded ];

	core::kinematics::MoveMapOP mmlig( new core::kinematics::MoveMap );
	mmlig->set_bb( false ); mmlig->set_chi( false ); mmlig->set_jump( false );
	core::Size lig_jump = gene.get_jumpid();
	mmlig->set_jump( lig_jump, true );
	if ( pose.size() == 1 ) {
		mmlig->set_chi( true ); // ligand-only; virtual root
	} else {
		for ( auto resid : gene.ligand_ids() ) {
			mmlig->set_chi( resid, true );
			mmlig->set_bb( resid, true );
		}
		if ( min_neighbor_ ) {
			std::set< core::Size > frozen_residues;
			if ( frozen_residues_ != nullptr ) {
				frozen_residues = core::select::get_residue_set_from_subset( frozen_residues_->apply(pose) );
			}
			for ( auto resid : contact_scs ) {
				if ( frozen_residues.count(resid) == 0 ) {
					mmlig->set_chi( resid, true );
				}
			}
		}
	}

	core::optimization::CartesianMinimizer minimizer;
	core::optimization::MinimizerOptions options( "lbfgs_armijo", 0.0001, true , false );
	options.max_iter(50);

	core::scoring::ScoreFunctionOP scfxn_cartmin = scfxn_relax_->clone();
	if ( w_cart < 1.0e-6 ) {
		scfxn_cartmin->set_weight( core::scoring::cart_bonded, 0.5 );
		scfxn_cartmin->set_weight( core::scoring::cart_bonded_ring, -0.5 );
		scfxn_cartmin->set_weight( core::scoring::pro_close, 0.0 );
	}

	if ( macrocycle_ligand_ ) {
		scfxn_cartmin->set_weight( core::scoring::atom_pair_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::angle_constraint, 1.0 );
		scfxn_cartmin->set_weight( core::scoring::dihedral_constraint, 1.0 );
		add_macrocycle_constraints( pose, gene.ligand_ids() );
	}

	minimizer.run( pose, *mmlig, *scfxn_cartmin, options );
	// This is a final cart ligand minimization, so I remove the constraints in the end.
	// But should we keep all the constraints in the pose?
	pose.remove_constraints();
}

// use ExplicitWaterMover to solvate ligand
void
GALigandDock::final_solvate(
	LigandConformer & gene,
	core::pose::Pose & pose
) {
	using namespace pack;
	using namespace pack::task;
	using namespace pack::task::operation;

	core::pack::task::PackerTaskOP task_new = core::pack::task::TaskFactory::create_packer_task( pose );
	task_new->or_include_current(true);

	for ( core::Size i(1); i <= pose.total_residue(); ++i ) {
		auto i_it = std::find( gene.moving_scs().begin(), gene.moving_scs().end(), i);
		auto j_it = std::find( gene.ligand_ids().begin(), gene.ligand_ids().end(), i);
		if ( j_it != gene.ligand_ids().end() || i_it != gene.moving_scs().end() ) {
			task_new->nonconst_residue_task(i).restrict_to_repacking();
		} else {
			task_new->nonconst_residue_task(i).prevent_repacking();
		}
	}

	core::scoring::ScoreFunctionOP sfwater = scfxn_->clone();
	sfwater->set_weight( core::scoring::pointwater, 1.0 );

	protocols::simple_moves::ExplicitWaterMover wb(sfwater);
	wb.set_taskop( task_new );
	wb.apply( pose );
}


// for multi-outputting, get the next pose
core::pose::PoseOP
GALigandDock::get_additional_output() {
	core::pose::PoseOP retval = remaining_outputs_.pop();
	if ( retval == nullptr ) return retval;

	(*scfxn_relax_)(*retval);
	return retval;
}

// load the initial inputs specified by initial_pool_
//   - reads all files ending in .pdb as PDBs
//   - reads special tag input as the input pose
//   - reads everything else as a silent file
void
GALigandDock::load_initial_pool(
	LigandConformer const &gene_initial,
	LigandConformers &genes_sel
) const {
	utility::vector1<std::string> input_pdbs = utility::string_split( initial_pool_, ',' );

	for ( core::Size ipdb = 1; ipdb <= input_pdbs.size(); ++ipdb ) {
		std::string tag = input_pdbs[ipdb];

		if ( (tag.length() >= 3) && (tag.substr( tag.length()-3 )=="pdb") ) {
			LigandConformer gene=gene_initial;
			gene.score( 0.0 );
			core::pose::PoseOP pose = core::import_pose::pose_from_file( tag, false, core::import_pose::PDB_file );
			if ( TR.Debug.visible() ) {
				pose->dump_pdb("initial_pool."+std::to_string(ipdb)+".pdb");
			}
			if ( pose->size() == gene.ligand_ids().size() ) {
				gene.update_ligand_conf( pose );
			} else {
				gene.update_conf( pose );
			}
			if ( TR.Debug.visible() ) {
				pose->dump_pdb("initial_pool."+std::to_string(ipdb)+".pdb");
				gene.dump_pose( "initial_pool.gene."+std::to_string(ipdb)+".pdb" );
			}

			genes_sel.push_back( gene );

		} else if ( tag != "INPUT" && tag != "Input" && tag != "input" ) {
			// silent file
			core::io::silent::SilentFileOptions opts; // initialized from the command line
			core::io::silent::SilentFileData sfd( opts );
			sfd.read_file( tag );

			//fd: temporary hack for terminal type mismatches
			//core::pose::PoseOP pose_ref(new core::pose::Pose);
			//gene_initial.to_pose( pose_ref );

			for ( auto iter = sfd.begin(), end = sfd.end(); iter != end; ++iter ) {
				LigandConformer gene=gene_initial;
				gene.score( 0.0 );
				core::pose::PoseOP pose(new core::pose::Pose);
				iter->fill_pose( *pose );

				//fd: temporary hack for terminal type mismatches
				//pose->replace_residue( 1, pose_ref->residue(1), false );
				gene.update_conf( pose );

				genes_sel.push_back( gene );
			}
		} else {
			// tag == input
			LigandConformer gene=gene_initial;
			gene.score( 0.0 );
			genes_sel.push_back( gene );
		}
	}
}

// load the initial inputs specified by reference_pool_
//   - reads all files ending in .pdb as PDBs
//   - reads everything else as a silent file
//  we assume the reference pose ligand is the last residue (or should we use _all_ ligands?)
void
GALigandDock::load_reference_pool(
	LigandConformer const &gene_initial,
	utility::vector1< ConstraintInfo > & ref_ligs
) const {
	utility::vector1<std::string> ref_pdbs = utility::string_split( reference_pool_, ',' );

	for ( core::Size ipdb = 1; ipdb <= ref_pdbs.size(); ++ipdb ) {
		std::string tag = ref_pdbs[ipdb];

		if ( tag == "input" || tag == "Input" || tag == "INPUT" ) {
			if ( ! use_pharmacophore_ ) {
				core::pose::PoseOP pose( new core::pose::Pose );
				gene_initial.to_pose( pose );
				ref_ligs.push_back(
					ConstraintInfo(*pose, gene_initial.ligand_ids(), false, false ) // report only at first case
				);
			}
		} else if ( (tag.length() >= 3) && (tag.substr( tag.length()-3 )=="pdb") ) {
			core::pose::PoseOP pose = core::import_pose::pose_from_file( tag, false, core::import_pose::PDB_file );
			utility::vector1 < core::Size > lig_resids;
			get_ligand_resids(*pose, lig_resids);
			ref_ligs.push_back(
				ConstraintInfo(*pose, lig_resids, use_pharmacophore_, (ipdb==1) )
			);

		} else {
			// silent file
			core::io::silent::SilentFileOptions opts; // initialized from the command line
			core::io::silent::SilentFileData sfd( opts );
			sfd.read_file( tag );

			for ( auto iter = sfd.begin(), end = sfd.end(); iter != end; ++iter ) {
				core::pose::PoseOP pose(new core::pose::Pose);
				iter->fill_pose( *pose );
				utility::vector1 < core::Size > lig_resids;
				get_ligand_resids(*pose, lig_resids);
				ref_ligs.push_back(
					ConstraintInfo(*pose, lig_resids, use_pharmacophore_, (ipdb==1) )
				);
			}
		}
	}
}

// LigandAligner
LigandAligner
GALigandDock::setup_ligand_aligner( core::pose::Pose const & pose,
	utility::vector1< core::Size > const &lig_resnos,
	utility::vector1< core::Size > movable_scs_in_ref, // call by value
	utility::vector1< core::conformation::Residue > const &rsds_to_build_grids,
	utility::vector1< bool > const &use_sc_only_in_grid
) const {
	GridScorerOP gridscore_ref(new GridScorer( scfxn_ ));
	gridscore_ref->set_voxel_spacing( grid_*2.0 ); // coarse-grained

	// In dockPH mode, 0.02 prefers collapsing inside receptor
	if ( use_pharmacophore_ ) {
		gridscore_ref->set_w_rep( 0.1 );
		movable_scs_in_ref.clear(); // drop these from grid construction
	} else {
		gridscore_ref->set_w_rep( 0.02 ); // softer for reference-docking mode
	}
	gridscore_ref->set_smoothing( 0.75 ); // hard-coded
	gridscore_ref->set_bbox_padding( padding_ );
	gridscore_ref->set_hash_gridding( hashsize_ );
	gridscore_ref->set_hash_subgridding( subhash_ );
	gridscore_ref->set_exact( false );
	gridscore_ref->set_debug( false );
	gridscore_ref->set_out_of_bound_e( grid_bound_penalty_ );
	gridscore_ref->prepare_grid( pose, lig_resnos );
	if ( use_mean_maxRad_ ) {
		gridscore_ref->set_grid_dim_with_maxRad(multi_ligands_maxRad_);
	}

	gridscore_ref->get_grid_atomtypes( rsds_to_build_grids, use_sc_only_in_grid );
	gridscore_ref->calculate_grid( pose, lig_resnos, movable_scs_in_ref );

	bool fastmode = (runmode_ == "VSX");
	LigandAligner aligner( use_pharmacophore_, movable_scs_in_ref, fastmode);
	aligner.set_sf( gridscore_ref );
	aligner.refine_input( (runmode_ == "refine") );
	aligner.set_sample_ring_conformers( sample_ring_conformers_ );

	if ( use_pharmacophore_ ) {
		// make sure reference_pool is not set to something else
		if ( !(reference_pool_ == "input" || reference_pool_ == "Input" ||
				reference_pool_ == "INPUT" ) ) {
			utility_exit_with_message("error!  pharmacophore docking requires reference_pool to be as 'INPUT'!" );
		}
		core::pose::PoseOP receptor( new core::pose::Pose( pose ) );
		core::Size startid, endid;
		startid = ( lig_resnos[1] <= lig_resnos.back())? lig_resnos[1] : lig_resnos.back() ;
		endid = ( lig_resnos[1] > lig_resnos.back())? lig_resnos[1] : lig_resnos.back() ;
		receptor->delete_residue_range_slow(startid, endid);
		scfxn_->score( *receptor ); // for energygraph!!
		aligner.set_pharmacophore_reference( *receptor );
	}

	return aligner;
}

// initial perturbation
LigandConformers
GALigandDock::generate_perturbed_structures(
	LigandConformer const &gene_initial,
	GridScorerOP gridscorer,
	core::Size npool,
	LigandAligner aligner //call by value
) const {
	LigandConformers genes_sel, genes_ref, genes_rand;

	core::Real rmscut = protocol_[1].rmsthreshold;

	auto start = std::chrono::steady_clock::now();

	/////
	// 1: (optionally) load inputs and add to pool
	if ( initial_pool_ != "" ) {
		load_initial_pool(gene_initial, genes_sel);
	}
	core::Size nstruct_input = genes_sel.size(), nstruct_ref=0;
	int nleft = (int)npool - (int)nstruct_input;
	if ( nleft <= 0 ) {
		if ( reference_pool_ != "none" ) {
			TR << "WARN! Reference pool provided but will not be used.  Increase pool size!" << std::endl;
		}
		return genes_sel;
	}

	// set up grid scorer for steps 2 & 3
	// if there is a smoothing schedule, use iter 1 smoothness to generate structures
	Real const w_rep_org( gridscorer->get_w_rep() );
	core::Real smoothing = protocol_[1].smoothing;
	gridscorer->set_smoothing( smoothing );
	gridscorer->set_w_rep( 0.02 ); // using softer repulsion for input generation

	/////
	// 2: (optionally) load reference structures and randomly generate conformers
	if ( reference_pool_ != "none" ) {
		// make a temporary gridscorer for align docking; keep movable scs fixed in grid
		TR << "Construct a separate grid for LigandAligner, with 0.5 grid step and no movable scs." << std::endl;
		core::pose::PoseOP pose( new core::pose::Pose );
		gene_initial.to_pose( pose );

		utility::vector1< ConstraintInfo > ref_poses;
		load_reference_pool(gene_initial, ref_poses);

		// assign num structures generating from reference
		core::Size nstruct_ref = std::lround( nleft*reference_frac_ );
		core::Size nrefgen = debug_? nstruct_ref : int(reference_oversample_)*nstruct_ref;

		// re-assign numbers based on Nmatches if using pharmacophore
		if ( reference_frac_auto_ && use_pharmacophore_ ) {
			//nrefgen = aligner.estimate_nstruct_sample( pose->residue( gene_initial.ligand_id() ), nrefgen );
			nrefgen = aligner.estimate_nstruct_sample( *pose, gene_initial.ligand_ids(), nrefgen );
			nstruct_ref = (core::Size)(nrefgen/reference_oversample_);

			TR << "Automatically set nstruct-from-reference to " << nstruct_ref
				<< " (from " << nrefgen << " trials) of total " << nleft << " left to sample." << std::endl;
			TR << "Est. time for matching: " << nrefgen << "~" << nrefgen*2 << " seconds..." << std::endl;
		}
		core::Size nrefgen_sampler(0);
		nrefgen_sampler = (int)(nrefgen*torsion_sampler_percentage_);
		nrefgen = nrefgen - nrefgen_sampler;

		for ( core::Size i=1; i<=nrefgen; ++i ) {
			LigandConformer gene( gene_initial );

			if ( ref_poses.size() > 0 ) {
				ConstraintInfo const & selected_ref =
					ref_poses[ numeric::random::rg().random_range( 1, ref_poses.size() ) ];
				aligner.set_target( selected_ref ); // random reference from pool
			}

			gene.randomize( 0.0 );  // for torsion&ring; fix trans
			aligner.apply( gene );

			// rescore with orignal gridscorer to match scale
			Real score_soft = gridscorer->score( gene, true );
			gene.score( score_soft );
			genes_ref.push_back( gene );
		}

		for ( core::Size i=1; i<=nrefgen_sampler; ++i ) {
			LigandConformer gene( gene_initial );

			if ( ref_poses.size() > 0 ) {
				ConstraintInfo const & selected_ref =
					ref_poses[ numeric::random::rg().random_range( 1, ref_poses.size() ) ];
				aligner.set_target( selected_ref ); // random reference from pool
			}

			gene.sample_conformation( 0.0, torsion_sampler_ );  // for torsion&ring; fix trans
			aligner.apply( gene );

			// rescore with orignal gridscorer to match scale
			Real score_soft = gridscorer->score( gene, true );
			gene.score( score_soft );
			genes_ref.push_back( gene );
		}

		TR << "Generate " << nrefgen_sampler << " structures using TorsionSampler for ligand aligner." << std::endl;

		if ( nrefgen > 0 ) {
			// take best scoring subset
			std::sort(genes_ref.begin(), genes_ref.end(),
				[&](LigandConformer const &lig_i, LigandConformer const &lig_j){ return lig_i.score() < lig_j.score(); } );
			for ( core::Size i=1; i<=nstruct_ref; ++i ) {
				genes_sel.push_back( genes_ref[i] );
			}
			TR << "Ref scorecut = " << genes_ref[nstruct_ref].score() << std::endl;
			nleft -= nstruct_ref;
		}
	} // if


	if ( nleft <= 0 ) {
		return genes_sel;
	}

	/////
	// 3: random structures
	core::Size nrand = debug_? nleft : (int)(nleft*random_oversample_);
	core::Size nrand_sampler = (int)(nrand*torsion_sampler_percentage_);
	nrand = nrand - nrand_sampler;
	for ( core::Size i=1; i<=nrand; ++i ) {
		LigandConformer gene( gene_initial );
		if ( runmode_ == "refine" ) {
			gene = mutate( gene_initial ); //mutation parameters are set as small in advance
		} else {
			gene.randomize( gridscorer->get_padding() - 1.0 );  // radius of search
		}

		Real score_soft = gridscorer->score( gene, true ); // score with soft repulsive
		gene.score( score_soft );
		genes_rand.push_back( gene );
	}

	for ( core::Size i=1; i<=nrand_sampler; ++i ) {
		LigandConformer gene( gene_initial );
		if ( runmode_ == "refine" ) {
			gene = mutate( gene_initial ); //mutation parameters are set as small in advance
		} else {
			gene.sample_conformation( gridscorer->get_padding() - 1.0, torsion_sampler_ );  // radius of search
		}

		Real score_soft = gridscorer->score( gene, true ); // score with soft repulsive
		gene.score( score_soft );
		genes_rand.push_back( gene );
	}

	// 4 (optional): align structures to refrence
	if ( !align_reference_atom_ids_.empty() ) {
		for ( auto gene : genes_rand ) {
			gene.superimpose_to_ref_pose( align_reference_atom_ids_ );
			Real score_soft = gridscorer->score( gene, true ); // score with soft repulsive
			gene.score( score_soft );
		}
	}

	TR << "Generate " << nrand_sampler << " structures using TorsionSampler for random structures." << std::endl;

	// select lowest random structures by energy while ensuring diversity
	std::sort(genes_rand.begin(), genes_rand.end(),
		[&](LigandConformer const &lig_i, LigandConformer const &lig_j){ return lig_i.score() < lig_j.score(); } );
	utility::vector1< bool > selected( genes_rand.size(), false );
	for ( core::Size ii = 1; ii <= genes_rand.size(); ++ii ) {
		LigandConformer &gene_gen = genes_rand[ii];
		bool is_similar( false );
		for ( core::Size jj = 1; jj <= genes_sel.size() && !is_similar; ++jj ) {
			core::Real d = distance_fast( gene_gen, genes_sel[jj] );
			is_similar = ( d < rmscut );
		}
		if ( !is_similar ) {
			genes_sel.push_back( gene_gen );
			selected[ii] = true;
		}

		if ( genes_sel.size() == npool ) break;
	}
	// no random structures left that are unique.  Fill with lowest-scoring
	for ( core::Size ii = 1; ii <= genes_rand.size() && genes_sel.size()<npool; ++ii ) {
		if ( !selected[ii] ) {
			genes_sel.push_back( genes_rand[ii] );
			selected[ii] = true;
		}
	}
	//std::chrono::duration<double> pack_time, min_time;
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> gen_time = end-start;

	TR << "Finished generating initial pool in " << gen_time.count() << " seconds." << std::endl;
	TR << "   # from inputs: " << nstruct_input << std::endl;
	TR << "   # aligned to ref: " << nstruct_ref << std::endl;
	TR << "   # random placement: " << nleft << std::endl;

	// recover original weight
	gridscorer->set_w_rep( w_rep_org );

	return genes_sel;
}

void
GALigandDock::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap
) {

	scfxn_ = protocols::rosetta_scripts::parse_score_function( tag, datamap );
	if ( tag->hasOption("scorefxn_relax") ) {
		std::string const scorefxn_name( tag->getOption<std::string>( "scorefxn_relax" ) );
		scfxn_relax_ = (datamap.get< core::scoring::ScoreFunction * >( "scorefxns", scorefxn_name ))->clone();
	} else {
		scfxn_relax_ = protocols::rosetta_scripts::parse_score_function( tag, datamap );
	}

	// First read runmode; then allow options to change by user-specified options
	if ( tag->hasOption("runmode") ) {
		runmode_ = tag->getOption<std::string>("runmode");
		setup_params_for_runmode( runmode_ );
	}

	// Below are detailed controls
	if ( tag->hasOption("ngen") ) { ngen_ = tag->getOption<int>("ngen"); }

	// allowed movement
	if ( tag->hasOption("ligand") ) { ligid_ = tag->getOption<std::string>("ligand"); }
	if ( tag->hasOption("sidechains") ) { sidechains_ = tag->getOption<std::string>("sidechains"); }
	if ( tag->hasOption("sc_edge_buffer") ) { sc_edge_buffer_ = tag->getOption<core::Real>("sc_edge_buffer"); }
	if ( tag->hasOption("fa_rep_grid") ) { fa_rep_grid_ = tag->getOption<core::Real>("fa_rep_grid"); }
	if ( tag->hasOption("grid_bound_penalty") ) { grid_bound_penalty_ = tag->getOption<core::Real>("grid_bound_penalty"); }
	if ( tag->hasOption("rotprob") ) {  max_rot_cumulative_prob_ = tag->getOption<core::Real>("rotprob"); }
	if ( tag->hasOption("rotEcut") ) {  rot_energy_cutoff_ = tag->getOption<core::Real>("rotEcut"); }
	if ( tag->hasOption("favor_native") ) { favor_native_ = tag->getOption<core::Real>("favor_native"); }
	if ( tag->hasOption("optimize_input_H") ) { optimize_input_H_ = tag->getOption<bool>("optimize_input_H"); }
	if ( tag->hasOption("pre_optH_relax") ) { pre_optH_relax_ = tag->getOption<bool>("pre_optH_relax"); }

	if ( tag->hasOption("sample_ring_conformers") ) { sample_ring_conformers_ = tag->getOption<bool>("sample_ring_conformers"); }

	// input params
	if ( tag->hasOption("use_pharmacophore") ) {
		use_pharmacophore_ = tag->getOption<bool>("use_pharmacophore");
		if ( !use_pharmacophore_ ) reference_pool_ = "none";
	}
	if ( tag->hasOption("initial_pool") ) { initial_pool_ = tag->getOption<std::string>("initial_pool"); }
	if ( tag->hasOption("reference_oversample") ) { reference_oversample_ = tag->getOption<core::Real>("reference_oversample"); }
	if ( tag->hasOption("reference_pool") ) {
		reference_pool_ = tag->getOption<std::string>("reference_pool");
		if ( !use_pharmacophore_ && reference_pool_!="none" ) {
			TR.Warning << "Warning: pharmacophore is off but reference_pool is used, this will trigger a slower aligning algorithm!" << std::endl;
		}
	}
	if ( tag->hasOption("reference_frac") ) { reference_frac_ = tag->getOption<core::Real>("reference_frac"); }
	if ( tag->hasOption("reference_frac_auto") ) { reference_frac_auto_ = tag->getOption<bool>("reference_frac_auto"); }
	if ( tag->hasOption("random_oversample") ) { random_oversample_ = tag->getOption<core::Real>("random_oversample"); }
	if ( tag->hasOption("premin_ligand") ) { premin_ligand_ = tag->getOption<bool>("premin_ligand"); }
	if ( tag->hasOption("use_mean_maxRad") ) { use_mean_maxRad_ = tag->getOption<bool>("use_mean_maxRad"); }
	if ( tag->hasOption("stdev_multiplier") ) { stdev_multiplier_ = tag->getOption<core::Real>("stdev_multiplier"); }
	if ( tag->hasOption("torsion_sampler_percentage") ) { torsion_sampler_percentage_ = tag->getOption<core::Real>("torsion_sampler_percentage"); }
	if ( tag->hasOption("contact_distance") ) { contact_distance_ = tag->getOption<core::Real>("contact_distance"); }
	if ( tag->hasOption("freeze_ligand_backbone") ) { freeze_ligand_backbone_ = tag->getOption<bool>("freeze_ligand_backbone"); }
	if ( tag->hasOption("freeze_ligand") ) { freeze_ligand_ = tag->getOption<bool>("freeze_ligand"); }
	if ( tag->hasOption("macrocycle_ligand") ) { macrocycle_ligand_ = tag->getOption<bool>("macrocycle_ligand"); }

	if ( tag->hasOption("frozen_scs") ) {
		std::string frozen_scs = tag->getOption<std::string>("frozen_scs");
		if ( frozen_scs != "none" ) {
			// parse as residue numbers
			frozen_residues_ = core::pose::get_resnum_selector( tag, "frozen_scs" );
		}
	}

	if ( tag->hasOption("multiple_ligands") ) {
		std::string ligands_string = tag->getOption<std::string>("multiple_ligands");
		multiple_ligands_ = utility::string_split( ligands_string, ',' );
		if ( multiple_ligands_.size() > 1000 ) {
			TR.Error << "multiple_ligands arguments cannot be more than 1000! " << std::endl;
			utility_exit();
		}
	}

	if ( tag->hasOption("multiple_ligands_file") ) {
		std::string ligands_file = tag->getOption<std::string>("multiple_ligands_file");
		std::string line;
		std::ifstream myfile(ligands_file);
		core::chemical::ResidueTypeSetCOP residue_set( core::chemical::ChemicalManager::get_instance()
			->residue_type_set( "fa_standard" ) );
		if ( myfile.is_open() ) {
			while ( myfile.good() ) {
				std::getline(myfile, line);
				if ( line.empty() or line.find('#')==0 ) continue;
				if ( ! residue_set->has_name( line ) ) {
					TR.Warning << "No residue info found, skip "<< line << std::endl;
					continue;
				}
				multiple_ligands_.push_back(line);
			}
			myfile.close();
		} else {
			TR.Error << "Cannot open " << ligands_file << std::endl;
			utility_exit();
		}
	}

	if ( tag->hasOption("ligand_structure_file") ) {
		std::string ligands_string = tag->getOption<std::string>("ligand_structure_file");
		ligand_file_list_ = utility::string_split( ligands_string, ',' );
	}

	if ( tag->hasOption("ligand_structure_filelist") ) {
		std::string ligands_file = tag->getOption<std::string>("ligand_structure_filelist");
		std::string line;
		std::ifstream myfile(ligands_file);
		if ( myfile.is_open() ) {
			while ( myfile.good() ) {
				std::getline(myfile, line);
				if ( line.empty() or line.find('#')==0 ) continue;
				ligand_file_list_.push_back(line);
			}
			myfile.close();
		} else {
			TR.Error << "Cannot open " << ligands_file << std::endl;
			utility_exit();
		}
	}

	// reporting
	if ( tag->hasOption("nativepdb") ) {
		std::string nativepdb = tag->getOption<std::string>("nativepdb");
		pose_native_ = core::import_pose::pose_from_file( nativepdb, false, core::import_pose::PDB_file );
	}


	// post-processing
	if ( tag->hasOption("final_exact_minimize") ) {
		final_exact_minimize_ = tag->getOption<std::string>("final_exact_minimize");
		if ( final_exact_minimize_.substr(0,2) != "sc" && final_exact_minimize_.substr(0,4) != "bbsc"
				&& final_exact_minimize_ != "none" && final_exact_minimize_.substr(0,10) != "ligandonly" ) {
			TR.Error << "The tag 'final_exact_minimize' must be one of: sc, bbscX, ligandonly, none" << std::endl;
			utility_exit();
		}
	}

	if ( tag->hasOption("cartmin_lig") ) { cartmin_lig_ = tag->getOption<bool>("cartmin_lig"); }
	if ( tag->hasOption("min_neighbor") ) { min_neighbor_ = tag->getOption<bool>("min_neighbor"); }
	if ( min_neighbor_ && !cartmin_lig_ ) {
		TR.Error << "min_neighbor may only be specified if cartmin_lig is enabled!" << std::endl;
		utility_exit();
	}

	if ( tag->hasOption("estimate_dG") ) { estimate_dG_ = tag->getOption<bool>("estimate_dG"); }

	if ( tag->hasOption("final_solvate") ) {
		final_solvate_ = tag->getOption<bool>("final_solvate");
		if ( final_solvate_ && final_exact_minimize_ == "none" ) {
			TR.Error << "The option 'final_solvate' requires a final minimize set!" << std::endl;
			utility_exit();
		}
	}

	if ( tag->hasOption("full_repack_before_finalmin") ) { full_repack_before_finalmin_ = tag->getOption<bool>("full_repack_before_finalmin"); }

	//protocols
	if ( tag->hasOption("fastrelax_script") ) {
		fast_relax_script_file_ = tag->getOption<std::string>("fastrelax_script");
	}
	if ( tag->hasOption("move_water") ) { move_water_ = tag->getOption<bool>("move_water"); }
	if ( tag->hasOption("turnon_flexscs_at_relax") ) { turnon_flexscs_at_relax_ = tag->getOption<bool>("turnon_flexscs_at_relax"); }
	if ( tag->hasOption("redefine_flexscs_at_relax") ) { redefine_flexscs_at_relax_ = tag->getOption<bool>("redefine_flexscs_at_relax"); }

	// grid params
	if ( tag->hasOption("exact") ) { exact_ = tag->getOption<bool>("exact"); }
	if ( tag->hasOption("debug") ) { debug_ = tag->getOption<bool>("debug"); }
	if ( tag->hasOption("grid_step") ) { grid_ = tag->getOption<core::Real>("grid_step"); }
	if ( tag->hasOption("padding") ) { padding_ = tag->getOption<core::Real>("padding"); }
	if ( tag->hasOption("hashsize") ) { hashsize_ = tag->getOption<core::Real>("hashsize"); }
	if ( tag->hasOption("subhash") ) { subhash_ = tag->getOption<core::Real>("subhash"); }

	// per-cycle defaults
	if ( tag->hasOption("npool") ) { npool_ = tag->getOption<core::Size>("npool"); }

	if ( tag->hasOption("nrelax") ) nrelax_ = tag->getOption<core::Size>("nrelax");
	if ( tag->hasOption("nreport") ) nreport_ = tag->getOption<core::Size>("nreport");

	if ( tag->hasOption("pmut") ) { pmut_ = tag->getOption<core::Real>("pmut"); }
	if ( tag->hasOption("smoothing") ) { smoothing_ = tag->getOption<core::Real>("smoothing"); }
	if ( tag->hasOption("rmsdthreshold") ) { rmsdthreshold_ = tag->getOption<core::Real>("rmsdthreshold"); }
	if ( tag->hasOption("maxiter") ) { maxiter_ = tag->getOption<core::Size>("maxiter"); }
	if ( tag->hasOption("pack_cycles") ) { packer_cycles_ = tag->getOption<core::Size>("pack_cycles"); }
	if ( tag->hasOption("ramp_schedule") ) {
		std::string ramp_schedule_string = tag->getOption<std::string>("ramp_schedule");
		utility::vector1<std::string> ramp_schedule_stringV( utility::string_split( ramp_schedule_string , ',' ) );
		for ( std::string & scale : ramp_schedule_stringV ) {
			ramp_schedule_.push_back( atof( scale.c_str() ) );
		}
	}

	if ( tag->hasOption("align_reference_atom_ids") ) {
		std::string align_reference_atom_ids_string = tag->getOption<std::string> ( "align_reference_atom_ids" );
		utility::vector1< std::string > align_reference_atom_ids_stringV ( utility::string_split( align_reference_atom_ids_string , ',' ) );
		for ( auto & atom_id_string : align_reference_atom_ids_stringV ) {
			utility::vector1< std::string > atom_id_string_split = utility::string_split( atom_id_string , '-' );
			core::Size atom_index = stoi(atom_id_string_split.at(1));
			core::Size residue_index = stoi(atom_id_string_split.at(2));
			align_reference_atom_ids_.push_back( core::id::AtomID( atom_index, residue_index ) );
		}
	}

	// detailed per-cycle controls
	utility::vector1< utility::tag::TagCOP > const stage_tags( tag->getTags() );

	utility::vector1< utility::tag::TagCOP >::const_iterator tag_it;
	bool stage_specified( false );
	for ( tag_it = stage_tags.begin(); tag_it != stage_tags.end(); ++tag_it ) {
		if ( (*tag_it)->getName() == "Stage" ) stage_specified = true;
	}

	if ( stage_specified ) {
		// remove info in case specified from runmode
		if ( protocol_.size() > 0 ) protocol_.resize( 0 );

		utility::vector1< utility::tag::TagCOP >::const_iterator tag_it;
		for ( tag_it = stage_tags.begin(); tag_it != stage_tags.end(); ++tag_it ) {
			if ( (*tag_it)->getName() == "Stage" ) {
				GADockStageParams stage_i( 1, npool_, rmsdthreshold_, pmut_, maxiter_, packer_cycles_, smoothing_, ramp_schedule_ );
				if ( (*tag_it)->hasOption("repeats") ) { stage_i.repeats = (*tag_it)->getOption<core::Size>("repeats"); }
				if ( (*tag_it)->hasOption("npool") ) { stage_i.pool = (*tag_it)->getOption<core::Size>("npool"); }
				if ( (*tag_it)->hasOption("pmut") ) { stage_i.pmut = (*tag_it)->getOption<core::Real>("pmut"); }
				if ( (*tag_it)->hasOption("smoothing") ) { stage_i.smoothing = (*tag_it)->getOption<core::Real>("smoothing"); }
				if ( (*tag_it)->hasOption("elec_scale") ) { stage_i.elec_scale = (*tag_it)->getOption<core::Real>("elec_scale"); }
				if ( (*tag_it)->hasOption("rmsdthreshold") ) { stage_i.rmsthreshold = (*tag_it)->getOption<core::Real>("rmsdthreshold"); }
				if ( (*tag_it)->hasOption("maxiter") ) { stage_i.maxiter = (*tag_it)->getOption<core::Size>("maxiter"); }
				if ( (*tag_it)->hasOption("pack_cycles") ) { stage_i.packcycles = (*tag_it)->getOption<core::Size>("pack_cycles"); }
				if ( (*tag_it)->hasOption("ramp_schedule") ) {
					std::string ramp_schedule_string = (*tag_it)->getOption<std::string>("ramp_schedule");
					utility::vector1<std::string> ramp_schedule_stringV( utility::string_split( ramp_schedule_string , ',' ) );
					stage_i.ramp_schedule.clear();
					for ( std::string & scale : ramp_schedule_stringV ) {
						stage_i.ramp_schedule.push_back( atof( scale.c_str() ) );
					}
				}
				protocol_.push_back( stage_i );
			}
		}
	}

	// sanity checks
	if ( tag->hasOption("ngen") && protocol_.size() > 0 ) {
		TR.Error << "ngen specified but detailed schedule also given.  Aborting!" << std::endl;
		utility_exit();
	}

	// default protocol
	if ( protocol_.size() == 0 ) {
		TR << "Using default protocol." << std::endl;
		GADockStageParams stage_i( ngen_, npool_, rmsdthreshold_, pmut_, maxiter_, packer_cycles_, smoothing_, ramp_schedule_ ); // elec scale SHOULD be controled by per-stage schedule
		protocol_.push_back( stage_i );
	} else {
		TR << "Using custom " << protocol_.size() << "-stage protocol." << std::endl;
	}
}

// note that this only works through rosetta_scripts
void
GALigandDock::setup_params_for_runmode( std::string runmode )
{
	utility::vector1< core::Real > ramp_schedule;
	ramp_schedule.push_back( 0.1 ); ramp_schedule.push_back( 1.0 );
	protocol_.resize( 0 );
	fast_relax_lines_.resize( 0 );
	bool die( false );

	if ( runmode == "dockrigid" ) {
		GADockStageParams stage1( 10, 100, 1.0, // repeats, npool, rmsdthreshold
			0.2, 100, 100, 0.375, ramp_schedule ); // pmut, maxiter, packcycles, smoothing, ramp_schedule
		protocol_.push_back( stage1 );

		sidechains_ = "none";
		final_exact_minimize_ = "sc"; //default
		reference_pool_ = "input";
		nrelax_ = nreport_ = 20;
		// use default fast_relax schedule

	} else if ( runmode == "dockflex" ) {
		// hi-res cross docking scenario
		GADockStageParams stage1( 10, 100, 2.0, // repeats, npool, rmsdthreshold
			0.2, 100, 100, 0.375, ramp_schedule ); // pmut, maxiter, packcycles, smoothing, ramp_schedule
		protocol_.push_back( stage1 );

		sidechains_ = "aniso";
		final_exact_minimize_ = "bbsc1";
		sc_edge_buffer_ = 0.0; // in A
		reference_pool_ = "input";
		reference_frac_auto_ = true;
		nrelax_ = nreport_ = 20;

		fast_relax_lines_.push_back("switch:cartesian");
		fast_relax_lines_.push_back("repeat 1");
		fast_relax_lines_.push_back("ramp_repack_min 1.0   0.00001  1.0 200");
		fast_relax_lines_.push_back("accept_to_best");
		fast_relax_lines_.push_back("endrepeat");

	} else if ( runmode.substr(0,2) == "VS" ) {
		// virtual screening
		final_exact_minimize_ = "sc"; //default
		reference_pool_ = "input";
		estimate_dG_ = true;
		reference_frac_auto_ = true;
		premin_ligand_ = true;

		if ( runmode.substr(2) == "H" ) { // VS-Hires
			sidechains_ = "aniso";
			nrelax_ = nreport_ = 20;
			//1-round minimization
			min_neighbor_ = true;

			// repeats, npool, rmsdthreshold, pmut, maxiter, packcycles, smoothing, ramp_schedule
			GADockStageParams stage1( 5, 100, 2.0, 0.2, 100, 100, 0.75, ramp_schedule );
			stage1.elec_scale = 3.0; // upweight at early stages
			stage1.maxiter = 25;
			protocol_.push_back( stage1 );
			GADockStageParams stage2( 5, 100, 2.0, 0.2, 100, 100, 0.375, ramp_schedule );
			stage2.maxiter = 25;
			protocol_.push_back( stage2 );

		} else if ( runmode.substr(2) == "S" ) {
			// GZ: Reserved for VS-Standard
			sidechains_ = "none";
			nrelax_ = 20;
			nreport_ = 1;

			//1-round minimization
			min_neighbor_ = false;
			fast_relax_lines_.push_back("switch:torsion");
			fast_relax_lines_.push_back("repeat 2");
			fast_relax_lines_.push_back("ramp_repack_min 0.02  0.01   1.0 50");
			fast_relax_lines_.push_back("ramp_repack_min 1.0   0.001  1.0 50");
			fast_relax_lines_.push_back("accept_to_best");
			fast_relax_lines_.push_back("endrepeat");

			// repeats, npool, rmsdthreshold, pmut, maxiter, packcycles, smoothing, ramp_schedule
			GADockStageParams stage1( 3, 100, 2.0, 0.2, 50, 25, 0.375, ramp_schedule );
			protocol_.push_back( stage1 );

		} else if ( runmode.substr(2) == "X" ) { // VS-eXpress
			sidechains_ = "none";
			nrelax_ = 5;
			nreport_ = 1;

			//1-round minimization
			min_neighbor_ = false;
			fast_relax_lines_.push_back("switch:torsion");
			fast_relax_lines_.push_back("repeat 2");
			fast_relax_lines_.push_back("ramp_repack_min 0.02  0.01   1.0 50");
			fast_relax_lines_.push_back("ramp_repack_min 1.0   0.001  1.0 50");
			fast_relax_lines_.push_back("accept_to_best");
			fast_relax_lines_.push_back("endrepeat");

			//ramp_schedule.push_back( 0.1 ); ramp_schedule.push_back( 1.0 );
			// repeats, npool, rmsdthreshold, pmut, maxiter, packcycles, smoothing, ramp_schedule
			GADockStageParams stage1( 5, 50, 2.0, 0.2, 50, 25, 0.375, ramp_schedule );
			protocol_.push_back( stage1 );

		} else {
			die = true;
		}
	} else if ( runmode == "refine" ) {
		//
	} else if ( runmode == "eval" ) {
		sidechains_ = "none";
		estimate_dG_ = true;
	} else {
		die = true;
	}

	if ( die ) utility_exit_with_message("error!  No runmode defined for "+runmode);
}


GAOptimizerOP
GALigandDock::get_optimizer(
	LigandConformer const &gene_initial,
	GridScorerOP gridscorer
) const {
	GAOptimizerOP optimizer(new GAOptimizer(gridscorer));
	if ( pose_native_ ) {
		LigandConformer gene_native( pose_native_, gene_initial.ligand_ids(), gene_initial.moving_scs(), freeze_ligand_backbone_ );
		optimizer->set_native( gene_native );
	}

	optimizer->set_protocol( protocol_ );
	optimizer->set_max_rot_cumulative_prob( max_rot_cumulative_prob_ );
	optimizer->set_rot_energy_cutoff( rot_energy_cutoff_ );  // at some point make this a parameter?
	optimizer->set_favor_native( favor_native_ );
	optimizer->set_align_reference_atom_ids( align_reference_atom_ids_ );
	return optimizer;
}

std::string GALigandDock::get_name() const {
	return mover_name();
}

std::string GALigandDock::mover_name() {
	return "GALigandDock";
}

// xml stuff
std::string gadock_subelement_ct_name( std::string const & name ) {
	return "GALigandDock_subelement_" + name + "Type";
}

void GALigandDock::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) {
	using namespace utility::tag;
	AttributeList attlist;
	attlist + XMLSchemaAttribute( "scorefxn", xs_string, "weights file");
	attlist + XMLSchemaAttribute( "scorefxn_relax", xs_string, "weights file");

	attlist + XMLSchemaAttribute( "runmode", xs_string, "run mode [dock/dockPH/refine/optligand]");

	attlist + XMLSchemaAttribute( "sample_ring_conformers", xsct_rosetta_bool, "Allow ring conformer sampling if defined in params.");
	attlist + XMLSchemaAttribute( "rotprob", xsct_real, "max cumulative rotamer probability");
	attlist + XMLSchemaAttribute( "rotEcut", xsct_real, "rotamer 1b energy");
	attlist + XMLSchemaAttribute( "ligand", xs_string, "ligand residue ids (if not specified default to last residue)");
	attlist + XMLSchemaAttribute( "nativepdb", xs_string, "name of native pdb");
	attlist + XMLSchemaAttribute( "favor_native", xsct_real, "give a bonus score to the input rotamer");
	attlist + XMLSchemaAttribute( "optimize_input_H", xsct_rosetta_bool, "do not optimize H at the begining (which is used for grid construction)");
	attlist + XMLSchemaAttribute( "pre_optH_relax", xsct_rosetta_bool, "relax structure before optimize hydrogen in final exact minimization");
	attlist + XMLSchemaAttribute( "grid_step", xsct_real, "Grid step (A) for grid-based scoring");
	attlist + XMLSchemaAttribute( "padding", xsct_real, "Padding (A) step for grid-based scoring");
	attlist + XMLSchemaAttribute( "hashsize", xsct_real, "Width of hash bins (A)");
	attlist + XMLSchemaAttribute( "subhash", xsct_non_negative_integer, "When scanning gridspace, subhash to this level");
	attlist + XMLSchemaAttribute( "nrelax", xsct_non_negative_integer, "Num. structs to run final minimize");
	attlist + XMLSchemaAttribute( "nreport", xsct_non_negative_integer, "Num. structs to report");
	attlist + XMLSchemaAttribute( "final_exact_minimize", xs_string, "Minimize the Genes by exact score after GA.");
	attlist + XMLSchemaAttribute( "cartmin_lig", xsct_rosetta_bool, "Cartmin ligand-only before and after final relax");
	attlist + XMLSchemaAttribute( "premin_ligand", xsct_rosetta_bool, "Cartmin ligand-only at the beginning");
	attlist + XMLSchemaAttribute( "min_neighbor", xsct_rosetta_bool, "If cartmin is enabled, also cartmin SCs before and after final relax.");
	attlist + XMLSchemaAttribute( "full_repack_before_finalmin", xsct_rosetta_bool, "Full repack before final relax.");
	attlist + XMLSchemaAttribute( "final_solvate", xsct_rosetta_bool, "Solvate pose (via ExplicitWaterMover) in final optimize. Default: false");
	attlist + XMLSchemaAttribute( "fastrelax_script", xs_string, "FastRelax script file for exact minimize.");
	attlist + XMLSchemaAttribute( "move_water", xsct_rosetta_bool, "Move water at final relaxation.");
	attlist + XMLSchemaAttribute( "turnon_flexscs_at_relax", xsct_rosetta_bool, "Turn on movable residues at final relaxation.");
	attlist + XMLSchemaAttribute( "redefine_flexscs_at_relax", xsct_rosetta_bool, "Redefine movable residues at final relaxation.");
	attlist + XMLSchemaAttribute( "exact", xsct_rosetta_bool, "Use exact scoring.");
	attlist + XMLSchemaAttribute( "debug", xsct_rosetta_bool, "Debug grid scoring: report both exact and grid scores.");
	attlist + XMLSchemaAttribute( "use_pharmacophore", xsct_rosetta_bool, "Use pharmacophore info at initial pool generation.");
	attlist + XMLSchemaAttribute( "initial_pool", xs_string, "Include these structures in the initial pool.");
	attlist + XMLSchemaAttribute( "multiple_ligands", xs_string, "Scan ligands with these residue types.");
	attlist + XMLSchemaAttribute( "multiple_ligands_file", xs_string, "Scan ligands with these residue types in a text file.");
	attlist + XMLSchemaAttribute( "ligand_structure_file", xs_string, "Scan ligands with these ligand structure files (pdb or silent file).");
	attlist + XMLSchemaAttribute( "ligand_structure_filelist", xs_string, "Scan ligands with ligand structure files (pdb or silent file) in a text file.");
	attlist + XMLSchemaAttribute( "random_oversample", xsct_real, "scale factor to ntrial of initial random pool generation");
	attlist + XMLSchemaAttribute( "reference_oversample", xsct_real, "scale factor to ntrial of initial reference pool generation");
	attlist + XMLSchemaAttribute( "reference_pool", xs_string, "Use this structures as _references_ to generate the initial pool.");
	attlist + XMLSchemaAttribute( "reference_frac", xsct_real, "If reference pool is provided, the fraction of structures from the reference pool.");
	attlist + XMLSchemaAttribute( "reference_frac_auto", xsct_rosetta_bool, "Select Nstruct to sample by reference automatically");
	attlist + XMLSchemaAttribute( "sidechains", xs_string, "Sidechains to move: none, auto, or residue IDs.");
	attlist + XMLSchemaAttribute( "frozen_scs", xs_string, "Sidechains to freeze: list of residue IDs.");
	attlist + XMLSchemaAttribute( "sc_edge_buffer", xsct_real, "Scaling factor of maxdistance when deciding to include sc as movable");
	attlist + XMLSchemaAttribute( "fa_rep_grid", xsct_real, "Repulsion weight at grid scoring stage");
	attlist + XMLSchemaAttribute( "grid_bound_penalty", xsct_real, "Penalty factor when ligand atm gets out of boundary");
	attlist + XMLSchemaAttribute( "estimate_dG", xsct_rosetta_bool, "Estimate dG of binding on lowest-energy docked pose. Default: false");
	attlist + XMLSchemaAttribute( "use_mean_maxRad", xsct_rosetta_bool, "Use mean maxRad for multi ligands? Default: false");
	attlist + XMLSchemaAttribute( "stdev_multiplier", xsct_real, "Standard deviation multiplier for mean_maxRad. Default: 1.0");
	attlist + XMLSchemaAttribute( "torsion_sampler_percentage", xsct_real, "The percentage of the initial gene sampled by torsion sampler.");
	attlist + XMLSchemaAttribute( "contact_distance", xsct_real, "Distance cutoff for determining if ligand is in contact with a residue sidechain. Default: 4.5" );
	attlist + XMLSchemaAttribute( "freeze_ligand_backbone", xsct_rosetta_bool, "Freeze peptide ligand backbone torsion, only works on peptide ligand. Default: false." );
	attlist + XMLSchemaAttribute( "freeze_ligand", xsct_rosetta_bool, "Freeze ligand internal torsions. Default: false." );
	attlist + XMLSchemaAttribute( "macrocycle_ligand", xsct_rosetta_bool, "If the ligand is macrocyle or cyclic peptide, if true, constraints will be added to ensure the ring closure. Default: false." );
	attlist + XMLSchemaAttribute( "align_reference_atom_ids", xs_string, "Atom ids to align after each cycle 'atom_num-residue_num,atom_num-residue_num,atom_num-residue_num...'");

	// per-cycle parameters (defaults)
	attlist + XMLSchemaAttribute( "ngen", xs_integer, "number of generations");
	attlist + XMLSchemaAttribute( "npool", xsct_non_negative_integer, "(default) pool size");
	attlist + XMLSchemaAttribute( "pmut", xsct_real, "(default) probability of mutation");
	attlist + XMLSchemaAttribute( "smoothing", xsct_real, "(default) grid smoothing");
	attlist + XMLSchemaAttribute( "rmsdthreshold", xsct_real, "(default) RMSD threshold between pool structures");
	attlist + XMLSchemaAttribute( "ramp_schedule", xs_string, "(default) During minimization, ramp fa_rep according to this schedule");
	attlist + XMLSchemaAttribute( "maxiter", xsct_non_negative_integer, "(default) maxiter for minimizer");
	attlist + XMLSchemaAttribute( "pack_cycles", xsct_non_negative_integer, "(default) pack for (N x #res) cycles");

	// attributes for "Stage" subelement
	AttributeList stage_subelement_attributes;
	stage_subelement_attributes
		+ XMLSchemaAttribute( "repeats", xs_integer, "number of generations in this stage")
		+ XMLSchemaAttribute( "npool", xsct_non_negative_integer, "pool size in this stage" )
		+ XMLSchemaAttribute( "smoothing", xsct_real, "Grid smoothing in this stage" )
		+ XMLSchemaAttribute( "elec_scale", xsct_real, "Scale of elec and hbond terms at this stage")
		+ XMLSchemaAttribute( "pmut", xsct_real, "Sampling frequency weight for this template" )
		+ XMLSchemaAttribute( "rmsdthreshold", xsct_real, "symmdef file associated with this template (only if using symmetry)" )
		+ XMLSchemaAttribute( "ramp_schedule", xs_string, "comma-seprated list of chains to randomize - not documented" )
		+ XMLSchemaAttribute( "maxiter", xsct_non_negative_integer, "maxiter for minimizer" )
		+ XMLSchemaAttribute( "pack_cycles", xsct_non_negative_integer, "pack for (N x #res) cycles");

	XMLSchemaSimpleSubelementList subelements;
	subelements.complex_type_naming_func( & gadock_subelement_ct_name );
	subelements.add_simple_subelement( "Stage", stage_subelement_attributes, "Per-stage parameters");

	protocols::moves::xsd_type_definition_w_attributes_and_repeatable_subelements(
		xsd, mover_name(), "This mover runs ligand docking using a GA (with gridded scoring) to optimize ligand-protein interaction energies.",
		attlist, subelements );
}

protocols::moves::MoverOP
GALigandDock::clone() const {
	return( protocols::moves::MoverOP( new GALigandDock( *this ) ) );
}

protocols::moves::MoverOP
GALigandDock::fresh_instance() const {
	return protocols::moves::MoverOP( new GALigandDock );
}

std::string GALigandDockCreator::keyname() const {
	return GALigandDock::mover_name();
}

protocols::moves::MoverOP
GALigandDockCreator::create_mover() const {
	return protocols::moves::MoverOP( new GALigandDock );
}

void GALigandDockCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const {
	GALigandDock::provide_xml_schema( xsd );
}

} // ga_dock
} // ligand_docking
} // protocols
