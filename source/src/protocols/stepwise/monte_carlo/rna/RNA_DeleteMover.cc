// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file RNA_DeleteMover
/// @brief Deletes an RNA residue from a chain terminus.
/// @detailed
/// @author Rhiju Das

#include <protocols/stepwise/monte_carlo/rna/RNA_DeleteMover.hh>
#include <protocols/stepwise/monte_carlo/SWA_MoveSelector.hh>
#include <protocols/stepwise/monte_carlo/rna/StepWiseRNA_MonteCarloOptions.hh>
#include <protocols/stepwise/enumerate/rna/StepWiseRNA_Modeler.hh>
#include <protocols/stepwise/enumerate/rna/StepWiseRNA_ModelerOptions.hh>
#include <protocols/stepwise/StepWiseUtil.hh>

// libRosetta headers
#include <core/types.hh>
#include <core/chemical/VariantType.hh>
#include <core/chemical/ResidueTypeSet.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/ResidueFactory.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>
#include <core/pose/full_model_info/FullModelInfo.hh>
#include <core/pose/full_model_info/FullModelInfo.hh>
#include <core/pose/full_model_info/FullModelInfoUtil.hh>
#include <core/scoring/ScoreFunction.hh>

#include <utility/tools/make_vector1.hh>
#include <utility/string_util.hh>

#include <basic/Tracer.hh>


using namespace core;
using namespace core::pose::full_model_info;
using core::Real;
using utility::make_tag_with_dashes;

//////////////////////////////////////////////////////////////////////////
// Removes one residue from a 5' or 3' chain terminus, and appropriately
//  updates the pose full_model_info object.
//
// Now updated to slice off more than one residue, or even an entire
//  fixed domain, and save that slice in an 'other_pose' (cached in the
//  main pose inside full_model_info).
//
//////////////////////////////////////////////////////////////////////////

static basic::Tracer TR( "protocols.stepwise.monte_carlo.RNA_DeleteMover" ) ;

namespace protocols {
namespace stepwise {
namespace monte_carlo {
namespace rna {


  //////////////////////////////////////////////////////////////////////////
  //constructor!
	RNA_DeleteMover::RNA_DeleteMover( ):
		minimize_after_delete_( true ),
		options_( new StepWiseRNA_MonteCarloOptions )
	{}

  //////////////////////////////////////////////////////////////////////////
  //destructor
  RNA_DeleteMover::~RNA_DeleteMover()
  {}

  //////////////////////////////////////////////////////////////////////////
  void
  RNA_DeleteMover::apply( core::pose::Pose &  )
	{
		std::cout << "not defined" << std::endl;
	}


	//////////////////////////////////////////////////////////////////////
  void
  RNA_DeleteMover::apply( core::pose::Pose & pose, Size const res_to_delete_in_full_model_numbering ) const
	{
		apply( pose, utility::tools::make_vector1( res_to_delete_in_full_model_numbering ) );
	}


	//////////////////////////////////////////////////////////////////////
  void
  RNA_DeleteMover::apply( core::pose::Pose & pose, utility::vector1< Size > const & residues_to_delete_in_full_model_numbering ) const
	{
		using namespace core::pose;

		FullModelInfo & full_model_info = nonconst_full_model_info( pose );
		utility::vector1< Size > const residues_to_delete = full_model_info.full_to_sub( residues_to_delete_in_full_model_numbering );

		// do the slice.
		PoseOP sliced_out_pose_op = new Pose;
		slice_out_pose( pose, *sliced_out_pose_op, residues_to_delete );

		// get rid of pieces that are single residues. no need to hold on to those.
		bool keep_remainder_pose( true ), keep_sliced_out_pose( true );
		remove_singletons_and_update_pose_focus( pose, sliced_out_pose_op, keep_remainder_pose, keep_sliced_out_pose );

		if ( keep_remainder_pose  ) fix_up_residue_type_variants( pose );
		if ( keep_sliced_out_pose ) fix_up_residue_type_variants( *sliced_out_pose_op );

		clear_constraints_recursively( pose );
		if ( get_native_pose() ) {
			superimpose_recursively_and_add_constraints( pose, *get_native_pose(),
																									 options_->constraint_x0(), options_->constraint_tol() );
		}

		if ( minimize_after_delete_ ) {
			if ( keep_remainder_pose  ) minimize_after_delete( pose );
			if ( keep_sliced_out_pose ) minimize_after_delete( *sliced_out_pose_op );
		}

	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool
  RNA_DeleteMover::decide_to_keep_pose( pose::Pose const & pose ) const {
		return ( check_for_fixed_domain( pose ) ||
						 ( ( options_->allow_from_scratch() || options_->allow_split_off() ) && pose.total_residue() > 1 ) );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
  void
  RNA_DeleteMover::remove_singletons_and_update_pose_focus( core::pose::Pose & pose,
																														core::pose::PoseOP sliced_out_pose_op,
																														bool & keep_remainder_pose,
																														bool & keep_sliced_out_pose ) const {

		// make decision on which poses to keep.
		keep_remainder_pose  =  decide_to_keep_pose( pose );
		keep_sliced_out_pose =  decide_to_keep_pose( *sliced_out_pose_op );

		FullModelInfo & full_model_info = nonconst_full_model_info( pose );
		if ( keep_sliced_out_pose ) full_model_info.add_other_pose( sliced_out_pose_op );

		if ( !keep_remainder_pose ) { // remainder pose is a single nucleotide. no need to keep track of it anymore.

			runtime_assert( full_model_info.res_list().size() > 0 );
			Size const res_in_remainder_pose = full_model_info.res_list()[ 1 ];
			bool pose_is_alone( false );
			if ( keep_sliced_out_pose ){ // go to the sliced out pose.
				Size const sliced_out_pose_idx = full_model_info.get_idx_for_other_pose( *sliced_out_pose_op );
				switch_focus_to_other_pose( pose, sliced_out_pose_idx );
			} else if ( full_model_info.other_pose_list().size() > 0 ){ // switch focus randomly.
				switch_focus_among_poses_randomly( pose );
			} else {
				pose_is_alone = true;
			}

			if ( pose_is_alone ){
				TR << TR.Red << "EMPTY POSE! Keeping the pose with number of residues " << pose.total_residue() << TR.Reset << std::endl;

				FullModelInfoOP new_full_model_info = nonconst_full_model_info( pose ).clone_info();
				// Rosetta's Pose object craps out if it is blank.
				// Still want to have blank poses represent fully random chain in stepwise monte carlo,
				// and would be a good state to have, or at least transit through.
				// Creating a single virtual residue, with res_list cleared, acts as a very
				// special case, and rest of the SWA_MonteCarlo code checks for res_list.size().
				core::chemical::ResidueTypeSet const & residue_set = pose.residue_type( 1 ).residue_type_set();
				core::chemical::ResidueTypeCOPs const & rsd_type_list( residue_set.name3_map( "VRT" ) );
				core::conformation::ResidueOP new_res( core::conformation::ResidueFactory::create_residue( *rsd_type_list[1] ) );
				pose.clear();
				pose.append_residue_by_bond( *new_res );

				new_full_model_info->clear_res_list();
				new_full_model_info->clear_other_pose_list();
				set_full_model_info( pose, new_full_model_info );
			} else {
				FullModelInfo & new_full_model_info = nonconst_full_model_info( pose );
				Size const remainder_pose_idx = new_full_model_info.get_idx_for_other_pose_with_residue( res_in_remainder_pose );
				new_full_model_info.remove_other_pose_at_idx( remainder_pose_idx );
			}

		}

	}


	//////////////////////////////////////////////////////////////////////
	void
	RNA_DeleteMover::wipe_out_moving_residues( pose::Pose & pose ) {

		// don't do any minimizing -- just get rid of everything...
		bool const minimize_after_delete_save( minimize_after_delete_ );
		minimize_after_delete_ = false;

		utility::vector1< SWA_Move > swa_moves;
		SWA_MoveSelector swa_move_selector;
		swa_move_selector.get_delete_move_elements( pose, swa_moves);

		if ( swa_moves.size() > 0 ){ // recursively delete all residues.
			apply( pose, swa_moves[1].move_element() );
			wipe_out_moving_residues( pose );
		}

		minimize_after_delete_ = minimize_after_delete_save;

	}

	////////////////////////////////////////////////////////////////////
	void
	RNA_DeleteMover::minimize_after_delete( pose::Pose & pose ) const{

		using namespace core::pose::full_model_info;

		stepwise_rna_modeler_->set_skip_sampling( true );
		stepwise_rna_modeler_->set_moving_res_and_reset( 0 );
		stepwise_rna_modeler_->set_minimize_res( get_moving_res_from_full_model_info( pose ) );
		stepwise_rna_modeler_->apply( pose );

	}

	///////////////////////////////////////////////////////////////////
	void
	RNA_DeleteMover::set_stepwise_rna_modeler( protocols::stepwise::enumerate::rna::StepWiseRNA_ModelerOP stepwise_rna_modeler ){
		stepwise_rna_modeler_ = stepwise_rna_modeler;
	}

	//////////////////////////////////////////////////////////////////////
	std::string
	RNA_DeleteMover::get_name() const {
		return "RNA_DeleteMover";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void
	RNA_DeleteMover::set_options( StepWiseRNA_MonteCarloOptionsCOP options ){
		options_ = options;
	}

} //rna
} //monte_carlo
} //stepwise
} //protocols
