// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/util.cc
/// @brief
/// @author Jianqing Xu (xubest@gmail.com)

// Project Headers
#include <protocols/antibody/util.hh>

// Core Headers
#include <core/kinematics/FoldTree.hh>
#include <core/pose/PDBInfo.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>
#include <core/pose/util.tmpl.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/rms_util.hh>
#include <core/types.hh>
#include <core/id/AtomID_Map.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/constraints/ConstraintSet.hh>
#include <core/pack/rotamer_set/UnboundRotamersOperation.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/task/operation/NoRepackDisulfides.hh>
#include <core/pack/task/operation/OperateOnCertainResidues.hh>
#include <core/pack/task/operation/OptH.hh>
#include <core/pack/task/operation/ResFilters.hh>
#include <core/pack/task/operation/ResLvlTaskOperations.hh>
#include <core/scoring/constraints/ConstraintIO.hh>
#include <core/scoring/constraints/DihedralConstraint.hh>
#include <core/pack/task/operation/TaskOperations.hh>
#include <core/pack/dunbrack/RotamerConstraint.hh>
#include <core/scoring/rms_util.tmpl.hh>

// Protocol Headers
#include <protocols/toolbox/task_operations/RestrictToInterface.hh>
#include <protocols/simple_moves/ConstraintSetMover.hh>
#include <protocols/loops/Loop.hh>
#include <protocols/loops/Loops.hh>

// Numeric Headers
#include <numeric/xyz.functions.hh>
#include <numeric/xyzVector.io.hh>
#include <numeric/numeric.functions.hh>
#include <numeric/random/random.hh>
#include <numeric/PCA.hh>


// Basic Headers
#include <iostream>
#include <fstream>
#include <basic/Tracer.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <utility/file/FileName.hh>
#include <utility/file/file_sys_util.hh>

static basic::Tracer TR("antibody.util");


using namespace core;
namespace protocols {
namespace antibody {


//JQX:
// Description (Jason contributed)
// assuming a loop 10,11,12,13,14,15, cut_point is 13/14
//                  or loop = Loop (10, 15, 13)
// 1. (Rosetta Default)
//           set_single_loop_fold_tree()
//           jump from 8->17 (-2 amd +2)
// 2. (Aroop's)
//            simple_one_loop_fold_tree()
//            jump from 9->16 (-1 and +1)
// 3. (Aroop's)
//            simple_fold_tree()
//            jump from 10->15, but you need manuly input
// 4. (Aroop's)
//            setup_simple_fold_tree()
//            jump from 10->15, but you need manuly input

void simple_one_loop_fold_tree(
    pose::Pose & pose_in,
    loops::Loop const & loop	) {
	using namespace kinematics;

	TR.Debug <<  "Setting up simple one loop fold tree" << std::endl;

	//setup fold tree for this loop
	FoldTree f;
	f.clear();
	Size nres = pose_in.total_residue();
	Size jumppoint1 = loop.start() - 1;
	Size jumppoint2 = loop.stop() + 1;

	if( jumppoint1 < 1 )   jumppoint1 = 1;
	if( jumppoint2 > nres) jumppoint2 = nres;

	f.add_edge( 1, jumppoint1, Edge::PEPTIDE );
	f.add_edge( jumppoint1, loop.cut(), Edge::PEPTIDE );
	f.add_edge( loop.cut() + 1, jumppoint2, Edge::PEPTIDE );
	f.add_edge( jumppoint2, nres, Edge::PEPTIDE );
	f.add_edge( jumppoint1, jumppoint2, 1 );
	f.reorder( 1 );

	pose_in.fold_tree( f );

	TR.Debug <<  "Finished setting up simple one loop fold tree" << std::endl;

	return;
} // simple_one_loop_fold_tree

void simple_fold_tree(
    pose::Pose & pose_in,
    Size jumppoint1,
    Size cutpoint,
    Size jumppoint2 ) {
	using namespace kinematics;

	TR.Debug <<  "Setting up simple fold tree" << std::endl;

	//setup fold tree for this loop
	FoldTree f;
	f.clear();
	Size nres = pose_in.total_residue();

	if( jumppoint1 < 1 )   jumppoint1 = 1;
	if( jumppoint2 > nres) jumppoint2 = nres;

	f.add_edge( 1, jumppoint1, Edge::PEPTIDE );
	f.add_edge( jumppoint1, cutpoint, Edge::PEPTIDE );
	f.add_edge( cutpoint + 1, jumppoint2, Edge::PEPTIDE );
	f.add_edge( jumppoint2, nres, Edge::PEPTIDE );
	f.add_edge( jumppoint1, jumppoint2, 1 );
	f.reorder( 1 );

	pose_in.fold_tree( f );

	TR.Debug <<  "Finished setting up simple fold tree" << std::endl;

	return;
} // simple_fold_tree


///////////////////////////////////////////////////////////////////////////
/// @begin CDR_H3_filter
///
/// @brief tests if a loop has H3 like base charachteristics
///
/// @detailed Uses the Shirai rules to find out if the dihedral angle
///           formed by CA atoms of residues n-2,n-1,n and n+1 conform to a
///           kinked/extended structure in accordance with the sequence. If
///           there is a match, a true value is returned
///
/// @param[in] pose: full actual protein
///            loop_begin: seq numbered loop begin corresponding to pose
///            size: size of loop to compute loop_end
///
/// @global_read reads -command line flag -base stored in dle_ns
///              to determine to do the complete H3 filter check or just do
///              a prediction of the H3 base type based on the
///              aforementioned dihedral angle
///
/// @references Structural classification of CDR-H3 in antibodies
///             Hiroki Shirai, Akinori Kidera, Haruki Nakamura
///             FEBS Letters 399 (1996) 1-8
///
/// @authors Aroop 02/04/2010
///
/// @last_modified 02/04/2010
///////////////////////////////////////////////////////////////////////////

//TODO:
//JQX:
//work with Daisuke to put the L89 creteria into the code

bool CDR_H3_filter_legacy_code_with_old_rule(const pose::Pose & pose_in, loops::Loop & input_loop, bool is_camelid) {


	TR.Debug <<  "Checking Kink/Extended CDR H3 Base Angle" << std::endl;


	char const light_chain = 'L';

	if(is_camelid )
		return( true );

	// Values read from plot in reference paper. Fig 1 on Page 3
	// Values adjusted to match data from antibody training set
	Real const kink_lower_bound = -10.00; // Shirai: 0
	Real const kink_upper_bound = 70.00; // Shirai: 70
	Real const extended_lower_bound = 125.00; // Shirai: ~180
	Real const extended_upper_bound = 185.00; // Shirai: ~180

	// Hydrogen Bond maximum value is 3.9 Angstroms - not used
	//	Real const h_bond(3.9);
	// Salt Bridge maximum value is 2.0 Angstroms - not used
	//	Real const s_bridge(4.0);

	// chop out the loop:
	//JQX: 2 residues before h3, one residue after h3. Matched Rosetta2!
	Size start(input_loop.start()-2);
	Size stop(input_loop.stop()+1);


	//bool is_kinked( false );  // unused ~Labonte
	//bool is_extended( false );  // unused ~Labonte
	bool is_H3( false );

	// extract 3 letter residue codes for the chopped loop
	std::vector <std::string> aa_name; // loop residue 3 letter codes
	//JQX: pay attention here!! It is vector, not vector1! too painful to compare to R2 code
	//     just make vector, so it can match R2 code easily
	for(Size ii=start; ii<=stop; ii++) {
		aa_name.push_back(pose_in.residue(ii).name3() );
		//            TR<<pose_in.residue(ii).name1()<<std::endl;
	}

	Size const CA(2);   // CA atom position in full_coord array
	// base dihedral angle to determine kinked/extended conformation

	Real base_dihedral( numeric::dihedral_degrees(
	                        pose_in.residue( stop ).xyz( CA ),
	                        pose_in.residue( stop - 1).xyz( CA ),
	                        pose_in.residue( stop - 2).xyz( CA ),
	                        pose_in.residue( stop - 3).xyz( CA ) ) );

	//pose_in.dump_pdb("check_cter_dihedral.pdb");


	TR << "Base Dihedral: " << base_dihedral << std::endl;



	// JQX: the code in the below if statement was in Rosetta 2, but Aroop did not port it into R3
	//      Maybe he has already tested that, the performance was better if using extra
	//      sequence creteria. But for now, I still port the code in here. Maybe for some reason
	//      one still decides not to use the sequence rules.

	bool H3_base_only=false;

	if( H3_base_only) {
		std::string base;
		if((base_dihedral > kink_lower_bound) && (base_dihedral < kink_upper_bound)) {
			base = "KINK";
			TR << "              " << base << std::endl;
		} else if((base_dihedral > extended_lower_bound) && (base_dihedral < extended_upper_bound)) {
			base = "EXTENDED";
			TR << "              " << base << std::endl;
		} else {
			base = "NEUTRAL";
			TR << "              " << base << std::endl;
		}

		return( true );
	}


	// setting up pseudo-periodic range used in extended base computation
	if( base_dihedral < kink_lower_bound ) {
		base_dihedral = base_dihedral + 360.00;
	}

	// Rule 1a for standard kink
	if ((aa_name[aa_name.size()-3] != "ASP") && (aa_name[aa_name.size()-1] == "TRP")) { //aa_name.size()-3 = n-1
		//aa_name.size()-1 = n+1
		if( (base_dihedral > kink_lower_bound) && (base_dihedral < kink_upper_bound)) {
			// std::cout << "KINK Found" << std::endl; // aroop_temp remove
			//is_kinked = true;  // unused ~Labonte
			is_H3 = true;
		}
	}

	// Rule 1b for standard extended form
	if (  ( aa_name[ aa_name.size() - 3 ] == "ASP" ) &&
	        ( ( aa_name[1] != "LYS" ) && ( aa_name[1] != "ARG" ) ) &&
	        ( is_H3 != true )    ) {   //aa_name[1] = 0 position

		if( ( base_dihedral>extended_lower_bound) && (base_dihedral<extended_upper_bound) ) {
			// std::cout << "EXTENDED Found" << std::endl; // aroop_temp remove
			//is_extended = true;  // unused ~Labonte
			is_H3 = true;
		}

		if(!is_H3) {
			// Rule 1b extension for special kinked form
			bool is_basic(false); // Special basic residue exception flag
			for(Size ii = 2; ii <= Size(aa_name.size() - 5); ii++) {      //aa_name.size() - 5 = n-3
				if( aa_name[ii] == "ARG" || aa_name[ii] == "LYS" ) {      //aa_name[2] =  0 position
					is_basic = true;
					break;
				}
			}

			if(!is_basic) {
				Size rosetta_number_of_L49 = pose_in.pdb_info()->pdb2pose(light_chain, 49 );
				std::string let3_code_L49 = pose_in.residue( rosetta_number_of_L49 ).name3();
				if( let3_code_L49 == "ARG" || let3_code_L49 == "LYS") {
					is_basic = true;
				}
			}
			if( is_basic && ( base_dihedral > kink_lower_bound ) &&
			        ( base_dihedral < kink_upper_bound ) ) {
				// aroop_temp remove
				// std::cout << "KINK (special 1b) Found" << std::endl;
				//is_kinked = true;  // unused ~Labonte
				is_H3 = true;
			}
		}
	}

	// Rule 1c for kinked form with salt bridge
	if ( ( aa_name[ aa_name.size() - 3 ] == "ASP") &&
	        ( ( aa_name[1] == "LYS") || ( aa_name[1] == "ARG" ) ) &&
	        ( (aa_name[0] != "LYS" ) && ( aa_name[0] != "ARG" ) ) &&
	        ( is_H3 != true) ) {
		if( (base_dihedral > kink_lower_bound ) &&
		        (base_dihedral < kink_upper_bound ) ) {
			// aroop_temp remove
			// std::cout << "KINK (w sb) Found" << std::endl;
			//is_kinked = true;  // unused ~Labonte
			is_H3 = true;
		}
		if(!is_H3) {
			bool is_basic(false); // Special basic residue exception flag
			Size rosetta_number_of_L46 = pose_in.pdb_info()->pdb2pose( light_chain, 46 );
			std::string let3_code_L46 = pose_in.residue( rosetta_number_of_L46 ).name3();
			if( let3_code_L46 == "ARG" || let3_code_L46 == "LYS") is_basic = true;
			if( is_basic && (base_dihedral > extended_lower_bound ) &&
			        ( base_dihedral < extended_upper_bound ) ) {
				// aroop_temp remove
				// std::cout << "EXTENDED (special 1c) Found" << std::endl;
				//is_extended = true;  // unused ~Labonte
				is_H3 = true;
			}
		}
	}

	// Rule 1d for extened form with salt bridge
	if (  (  aa_name[ aa_name.size() - 3 ] == "ASP") &&
	        ( ( aa_name[1] == "LYS") || ( aa_name[1] == "ARG" ) ) &&
	        ( ( aa_name[0] == "LYS") || ( aa_name[0] == "ARG") ) &&
	        ( is_H3 != true )       ) {
		if( ( base_dihedral > extended_lower_bound ) &&
		        ( base_dihedral < extended_upper_bound ) ) {
			// aroop_temp remove
			// std::cout << "EXTENDED (w sb) Found" << std::endl;
			//is_extended = true;  // unused ~Labonte
			is_H3 = true;
		}
	}

	TR.Debug <<  "Finished Checking Kink/Extended CDR H3 Base Angle: " << is_H3 << std::endl;

	return is_H3;
} // CDR_H3_filter

bool CDR_H3_cter_filter(const pose::Pose & pose_in, AntibodyInfoOP ab_info) {

	TR.Debug <<  "Checking Kink/Extended CDR H3 Base Angle" << std::endl;

	if(ab_info->is_camelid() ) {
		return( true );
	}

	// Values read from plot in reference paper. Fig 1 on Page 3
	// Values adjusted to match data from antibody training set
	Real const kink_lower_bound = -10.00; // Shirai: 0
	Real const kink_upper_bound = 70.00; // Shirai: 70
	Real const extended_lower_bound = 125.00; // Shirai: ~180
	Real const extended_upper_bound = 185.00; // Shirai: ~180

	// Hydrogen Bond maximum value is 3.9 Angstroms - not used
	//	Real const h_bond(3.9);
	// Salt Bridge maximum value is 2.0 Angstroms - not used
	//	Real const s_bridge(4.0);

	// chop out the loop:
	//JQX: 2 residues before h3, one residue after h3. Matched Rosetta2!
	Size start(  ab_info->get_CDR_loop(h3).start()  -  2 );
	Size stop(  ab_info->get_CDR_loop(h3).stop()  +  1  );


	bool matched_kinked( false );
	bool matched_extended( false );


	// extract 3 letter residue codes for the chopped loop
	std::vector <std::string> aa_name; // loop residue 3 letter codes
	//JQX: pay attention here!! It is vector, not vector1! too painful to compare to R2 code
	//     just make vector, so it can match R2 code easily
	for(Size ii=start; ii<=stop; ii++) {
		aa_name.push_back(pose_in.residue(ii).name3() );
		//            TR<<pose_in.residue(ii).name1()<<std::endl;
	}

	Size const CA(2);   // CA atom position in full_coord array
	// base dihedral angle to determine kinked/extended conformation

	Real base_dihedral( numeric::dihedral_degrees(
	                        pose_in.residue( stop ).xyz( CA ),
	                        pose_in.residue( stop - 1).xyz( CA ),
	                        pose_in.residue( stop - 2).xyz( CA ),
	                        pose_in.residue( stop - 3).xyz( CA ) ) );

	//pose_in.dump_pdb("check_cter_dihedral.pdb");

	TR << "Base Dihedral: " << base_dihedral << std::endl;

	// setting up pseudo-periodic range used in extended base computation
	if( base_dihedral < kink_lower_bound ) {
		base_dihedral = base_dihedral + 360.00;
	}


	if( (base_dihedral > kink_lower_bound ) && (base_dihedral < kink_upper_bound ) ) {
		if(ab_info->get_Predicted_H3BaseType()==Kinked) {
			matched_kinked = true;
		}
	}
	if( (base_dihedral > extended_lower_bound ) && (base_dihedral < extended_upper_bound ) ) {
		if(ab_info->get_Predicted_H3BaseType()==Extended) {
			matched_extended = true;
		}
	}


	bool passed;
	if (matched_kinked || matched_extended) {
		passed=true;
	} else {
		passed=false;
	}

	TR.Debug <<  "Finished Checking Kink/Extended CDR H3 Base Angle: " << passed << std::endl;

	return passed;
}

core::pack::task::TaskFactoryOP setup_packer_task(pose::Pose & pose_in ) {
	using namespace pack::task;
	using namespace pack::task::operation;

	TR.Debug << "Setting Up Packer Task" << std::endl;

	core::pack::task::TaskFactoryOP tf = new TaskFactory;
	tf->clear();

	tf->push_back(new OperateOnCertainResidues(new PreventRepackingRLT, new ResidueLacksProperty("PROTEIN") ));
	tf->push_back(new InitializeFromCommandline );
	tf->push_back(new IncludeCurrent );
	tf->push_back(new RestrictToRepacking );
	tf->push_back(new NoRepackDisulfides );

	// incorporating Ian's UnboundRotamer operation.
	// note that nothing happens if unboundrot option is inactive!
	pack::rotamer_set::UnboundRotamersOperationOP
	unboundrot = new pack::rotamer_set::UnboundRotamersOperation();
	unboundrot->initialize_from_command_line();

	operation::AppendRotamerSetOP unboundrot_operation = new operation::AppendRotamerSet( unboundrot );
	tf->push_back( unboundrot_operation );

	// adds scoring bonuses for the "unbound" rotamers, if any
	core::pack::dunbrack::load_unboundrot( pose_in ); ///FIXME: this is dangerous here..... JQX
	//TODO:
	//JQX: need to understand this pose_in!!!!

	TR.Debug << "Done: Setting Up Packer Task" << std::endl;

	return tf;

} // setup_packer_task

/*    void
 dle_extreme_repack(
 pose::Pose & pose_in,
 int repack_cycles,
 ObjexxFCL::FArray1D_bool & allow_repack,
 bool rt_min,
 bool rotamer_trials,
 bool force_one_repack,
 bool use_unbounds
 )
 {
 using namespace pose;

 // Exit if not fullatom
 if( !pose_in.fullatom() ) {
 std::cout << "Repack called in centroid mode" << std::endl;
 std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
 std::cout << "------NOT REPACKING-----------" << std::endl;
 return;
 }
 // Saving parameters
 bool initial_rot_trial = score_get_try_rotamers();
 bool initial_min_rot = get_minimize_rot_flag();
 score_set_minimize_rot( rt_min );
 score_set_try_rotamers( rotamer_trials );
 // initial allowed chi movement
 FArray1D_bool old_chi_move( pose_in.total_residue(), false );
 for( int i = 1; i <= pose_in.total_residue(); i++ ) {
 // storing old
 old_chi_move(i) = pose_in.get_allow_chi_move(i);
 // setting new
 pose_in.set_allow_chi_move( i, allow_repack(i) || old_chi_move(i) );
 }
 Score_weight_map weight_map( score12 );
 Monte_carlo mc( pose_in, weight_map, 2.0 );
 // repack idealized native
 Pose start_native_pose;
 start_native_pose = pose_in;
 for(int i=1; i <= repack_cycles; i++) {
 pose_in = start_native_pose;
 if( use_unbounds )
 dle_pack_with_unbound( pose_in, allow_repack, true  ); // include_current = true
 else
 pose_in.repack( allow_repack, true ); //include_current =  true
 pose_in.score( weight_map );
 score_set_minimize_rot( false );
 score_set_try_rotamers( false );
 if( force_one_repack && (i == 1) ) mc.reset( pose_in );
 mc.boltzmann( pose_in );
 score_set_minimize_rot( rt_min );
 score_set_try_rotamers( rotamer_trials );
 }
 pose_in = mc.low_pose();
 pose_in.score( weight_map );

 // Restoring Globals
 score_set_minimize_rot( initial_rot_trial );
 score_set_try_rotamers( initial_min_rot );
 pose_in.set_allow_chi_move( old_chi_move );

 return;
 }
 */


bool cutpoints_separation( core::pose::Pose & pose, AntibodyInfoOP & antibody_info ) {

	bool closed_cutpoints = true;

	for( loops::Loops::const_iterator it=antibody_info->get_AllCDRs_in_loopsop()->begin(),
	        it_end=antibody_info->get_AllCDRs_in_loopsop()->end(),
	        it_next; it != it_end; ++it ) {
		Size cutpoint   = it->cut();
		Real separation = 10.00; // an unlikely high number
		separation = cutpoint_separation( pose, cutpoint );

		if( separation > 1.9 ) {
			closed_cutpoints = false;
			break;
		}
	}
	return( closed_cutpoints );
} // cutpoints_separation

Real cutpoint_separation(pose::Pose & pose_in, Size cutpoint) {

	Size const N ( 1 ); // N atom
	Size const C ( 3 ); // C atom

	// Coordinates of the C atom of cutpoint res and N atom of res cutpoint+1
	numeric::xyzVector_float peptide_C(pose_in.residue( cutpoint ).xyz( C )),
	        peptide_N( pose_in.residue( cutpoint + 1 ).xyz( N ) );
	//			Real cutpoint_separation=distance(peptide_C, peptide_N);
	Real cutpoint_separation=peptide_C.distance(peptide_N);

	return( cutpoint_separation );
} // cutpoint_separation

Real global_loop_rmsd (const pose::Pose & pose_in, const pose::Pose & native_pose,loops::LoopsOP current_loop ) {
	if (pose_in.total_residue() != native_pose.total_residue() ) {
		throw excn::EXCN_BadInput("The pose sequence length does not match that of native_pose");
	}

	using namespace scoring;

	Size loop_start = (*current_loop)[1].start();
	Size loop_end = (*current_loop)[1].stop();

	using ObjexxFCL::FArray1D_bool;
	FArray1D_bool superpos_partner ( pose_in.total_residue(), false );

	for ( Size i = loop_start; i <= loop_end; ++i ) superpos_partner(i) = true;

	using namespace core::scoring;
	Real rmsG = rmsd_no_super_subset( native_pose, pose_in, superpos_partner, is_protein_backbone_including_O );
	return ( rmsG );
}

void align_to_native( core::pose::Pose & pose,
                      core::pose::Pose const & native_pose,
                      AntibodyInfoOP const ab_info,
                      AntibodyInfoOP const native_ab_info,
                      std::string const & reqeust_chain) {


	std::string pose_seq        = pose.sequence();
	std::string native_pose_seq = native_pose.sequence();
	if(pose_seq != native_pose_seq   ) {
		throw excn::EXCN_BadInput(" the pose sequence does not match native_pose sequence ");
	}


	core::id::AtomID_Map< core::id::AtomID > atom_map;
	core::pose::initialize_atomid_map( atom_map, pose, core::id::BOGUS_ATOM_ID );

	// loop over the L and H chains
	for (Size i_chain=1; i_chain<=ab_info->get_AntibodyFrameworkInfo().size(); i_chain++) {
		vector1<FrameWork>        chain_frmwk =        ab_info->get_AntibodyFrameworkInfo()[i_chain];
		vector1<FrameWork> native_chain_frmwk = native_ab_info->get_AntibodyFrameworkInfo()[i_chain];

		if(  (chain_frmwk[1].chain_name == reqeust_chain ) || (reqeust_chain =="LH")  ) {

			// loop over the segments on the framework of one chain
			for (Size j_seg=1; j_seg<=chain_frmwk.size(); j_seg++) { // for loop of the framework segments
				Size count=0;

				// loop over the residues on one segment on one framework of one chain
				for (Size k_res=chain_frmwk[j_seg].start; k_res<= chain_frmwk[j_seg].stop; k_res++) {
					count++;
					Size res_counter = k_res;
					Size nat_counter = native_chain_frmwk[j_seg].start+count-1;
					//TR<<"Matching Residue "<< res_counter<<" with  "<<nat_counter<<std::endl;

					// loop over the backbone atoms including Oxygen
					for( core::Size latm=1; latm <= 4; latm++ ) {
						core::id::AtomID const id1( latm, res_counter );
						core::id::AtomID const id2( latm, nat_counter );
						atom_map[ id1 ] = id2;
					}
				}

			}// end of the for loop

		}//end of the if statement
	}

	core::scoring::superimpose_pose( pose, native_pose, atom_map );

} // align_to_native()

std::map< CDRNameEnum, bool>
add_harmonic_cluster_constraints(AntibodyInfoOP ab_info, core::pose::Pose & pose){
	std::map< CDRNameEnum, bool> result;
	if (!ab_info->clusters_setup()){
		ab_info->setup_CDR_clusters(pose);
	}

	for (core::Size i=1; i<=ab_info->get_total_num_CDRs(); ++i){
		CDRNameEnum cdr_name = static_cast<CDRNameEnum>(i);
		result[cdr_name] = add_harmonic_cluster_constraint(ab_info, pose, ab_info->get_CDR_cluster(cdr_name).first);
	}
	return result;
}

std::map< CDRNameEnum, bool>
add_harmonic_cluster_constraints(AntibodyInfoOP ab_info, core::pose::Pose & pose, utility::vector1< core::scoring::constraints::ConstraintCOP > constraints){
	
	std::map< CDRNameEnum, bool> result;
	if (!ab_info->clusters_setup()){
		ab_info->setup_CDR_clusters(pose);
	}

	for (core::Size i=1; i<=ab_info->get_total_num_CDRs(); ++i){
		CDRNameEnum cdr_name = static_cast<CDRNameEnum>(i);
		result[cdr_name] = add_harmonic_cluster_constraint(ab_info, pose, ab_info->get_CDR_cluster(cdr_name).first, constraints);
	}
	return result;
}



bool
add_harmonic_cluster_constraint(AntibodyInfoCOP ab_info, core::pose::Pose & pose, CDRClusterEnum const cluster){

	using namespace core::scoring::constraints;

	
	std::string fname = get_harmonic_cluster_constraint_filename(ab_info, cluster);
	if (fname=="NA"){return false;}
	ConstraintSetOP cst = ConstraintIO::get_instance()->read_constraints(fname, new ConstraintSet, pose);

	pose.add_constraints(cst->get_all_constraints());
	return true;
}

bool
add_harmonic_cluster_constraint(AntibodyInfoCOP ab_info, core::pose::Pose & pose, CDRClusterEnum const cluster, utility::vector1< core::scoring::constraints::ConstraintCOP > constraints){

	using namespace core::scoring::constraints;
	
	
	std::string fname = get_harmonic_cluster_constraint_filename(ab_info, cluster);
	if (fname=="NA"){return false;}
	
	ConstraintSetOP cst = ConstraintIO::get_instance()->read_constraints(fname, new ConstraintSet, pose);

	vector1< ConstraintCOP > local_csts = cst->get_all_constraints();
	pose.add_constraints(local_csts);

	constraints.insert(constraints.end(), local_csts.begin(), local_csts.end());
	return true;
}

bool
set_harmonic_cluster_constraint(AntibodyInfoCOP ab_info, core::pose::Pose & pose, CDRClusterEnum const cluster) {

	using namespace protocols::simple_moves;


	std::string fname = get_harmonic_cluster_constraint_filename(ab_info, cluster);
	if (fname=="NA"){return false;}
	ConstraintSetMoverOP cst_mover = new ConstraintSetMover();
	cst_mover->constraint_file(fname);
	cst_mover->apply(pose);
	return true;
}

std::string
get_harmonic_cluster_constraint_filename(AntibodyInfoCOP ab_info, CDRClusterEnum const cluster ){
	
	using namespace basic::options;
	
	std::string fname;
	std::string cluster_type = ab_info->get_cluster_name(cluster);
	if (cluster_type=="NA") {
		TR<< "Cannot add constraint to cluster of type NA.  Skipping."<<std::endl;
		fname = "NA";
	}
	std::string path = "sampling/antibodies/cluster_based_constraints/CircularHarmonic/";
	std::string extension = ".txt";
	std::string specific_path = path + cluster_type + extension;
	fname = option[ OptionKeys::in::path::database ](1).name() + specific_path;
	if( !utility::file::file_exists(fname)) {
		TR<< "Fname "<<fname<<" Does not exist.  No constraint will be added."<<std::endl;
		fname =  "NA";
	}
	return fname;
}

bool
check_if_pose_renumbered_for_clusters(core::pose::Pose const & pose){
	

	if (core::pose::has_chain("L", pose)){
		//L1
		if (pose.residue(pose.pdb_info()->pdb2pose('L', 23)).name1() != 'C'){
			TR << "Problem with L1 starting anchor residue name" <<std::endl;
			return false;
		}
		if (pose.residue(pose.pdb_info()->pdb2pose('L', 43)).name1() != 'W'){
			TR << "Problem with L1 ending anchor residue name" <<std::endl;
			return false;
		}
		
		//L3
		if (pose.residue(pose.pdb_info()->pdb2pose('L', 106)).name1() != 'C'){
			TR << "Problem with L3 starting anchor residue name" <<std::endl;
			return false;
		}
		if (pose.residue(pose.pdb_info()->pdb2pose('L', 139)).name1() != 'F'){
			TR << "Problem with L3 ending anchor residue name" <<std::endl;
			return false;
		}
	}
	
	if (core::pose::has_chain("H", pose)){
		//H1
		if (pose.residue(pose.pdb_info()->pdb2pose('H', 23)).name1() != 'C'){
			TR << "Problem with H1 starting anchor residue name" <<std::endl;
			return false;
		}
		if (pose.residue(pose.pdb_info()->pdb2pose('H', 43)).name1() != 'W'){
			TR << "Problem with H1 ending anchor residue name" <<std::endl;
			return false;
		}
		
		//H3
		if (pose.residue(pose.pdb_info()->pdb2pose('H', 106)).name1() != 'C'){
			TR << "Problem with H3 starting anchor residue name" <<std::endl;
			return false;
		}
		if (pose.residue(pose.pdb_info()->pdb2pose('H', 139)).name1() != 'W'){
			TR << "Problem with H3 ending anchor residue name" <<std::endl;
			return false;
		}
	}
	
	return true;
}

} // namespace antibody
} // namespace protocols

