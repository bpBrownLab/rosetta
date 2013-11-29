// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/swa/rna/screener/ChainBreakScreener.cc
/// @brief
/// @detailed
/// @author Rhiju Das, rhiju@stanford.edu

#include <protocols/swa/rna/screener/ChainBreakScreener.hh>
#include <protocols/swa/rna/StepWiseRNA_Util.hh>
#include <protocols/rna/RNA_LoopCloser.hh>
#include <core/chemical/VariantType.hh>
#include <core/conformation/Residue.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/Energies.hh>
#include <core/id/TorsionID.hh>

#include <basic/Tracer.hh>
#include <utility/exit.hh>

using namespace core;

static basic::Tracer TR( "protocols.swa.rna.screener.ChainBreakScreener" ) ;

namespace protocols {
namespace swa {
namespace rna {
namespace screener {

	//Constructor
	ChainBreakScreener::ChainBreakScreener( pose::Pose const & pose, Size const five_prime_res ):
		chain_break_screening_pose_( pose ),
		five_prime_res_( five_prime_res ),
		reinitialize_CCD_torsions_( false ),
		verbose_( false )
	{
		chain_break_scorefxn_ =  new core::scoring::ScoreFunction;
		chain_break_scorefxn_->set_weight( core::scoring::angle_constraint, 1.0 );
		chain_break_scorefxn_->set_weight( core::scoring::atom_pair_constraint, 1.0 );
	}

	//Destructor
	ChainBreakScreener::~ChainBreakScreener()
	{}


	////////////////////////////////////////////////////////////////////////////////////////
	void
	ChainBreakScreener::copy_CCD_torsions( pose::Pose & pose ) const {

		using namespace core::chemical;
		using namespace core::conformation;
		using namespace core::id;

		Size const three_prime_res = five_prime_res_ + 1;
		//Even through there is the chain_break, alpha of 3' and epl and gamma of 5' should be defined due to the existence of the upper and lower variant type atoms.
 		copy_CCD_torsions_general( pose, five_prime_res_, three_prime_res );

	}


	////////////////////////////////////////////////////////////////////////////////////////
	void
	ChainBreakScreener::copy_CCD_torsions_general( pose::Pose & pose, Size const five_prime_res, Size const three_prime_res ) const {

		using namespace core::chemical;
		using namespace core::conformation;
		using namespace core::id;

		if ( ( five_prime_res ) != ( three_prime_res - 1 ) ) utility_exit_with_message( "( five_prime_res ) != ( three_prime_res - 1 )" );

		conformation::Residue const & lower_res = chain_break_screening_pose_.residue( five_prime_res );
		conformation::Residue const & upper_res = chain_break_screening_pose_.residue( three_prime_res );

		for ( Size n = 1; n <= 3; n++ ){ //alpha, beta, gamma of 3' res
			pose.set_torsion( TorsionID( three_prime_res, id::BB,  n ), upper_res.mainchain_torsion( n ) );
		}

		for ( Size n = 5; n <= 6; n++ ){ //epsilon and zeta of 5' res
			pose.set_torsion( TorsionID( five_prime_res, id::BB,  n ), lower_res.mainchain_torsion( n ) );
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////
	bool
	ChainBreakScreener::check_loop_closed( pose::Pose const & pose ){
		static protocols::rna::RNA_LoopCloser rna_loop_closer;
		return ( rna_loop_closer.check_closure( pose, five_prime_res_ ) );
	}

	////////////////////////////////////////////////////////////////////////////////////////
	bool
	ChainBreakScreener::chain_break_screening_general( pose::Pose & chain_break_screening_pose,
																										 core::scoring::ScoreFunctionOP const & chain_break_scorefxn,
																										 Size const five_prime_res ){

		using namespace core::scoring;

		static protocols::rna::RNA_LoopCloser rna_loop_closer;

		if ( !chain_break_screening_pose.residue( five_prime_res ).has_variant_type( chemical::CUTPOINT_LOWER ) ) {
			utility_exit_with_message( "chain_break_screening_pose.residue( five_prime_chain_break_res ).has_variant_type(  chemical::CUTPOINT_LOWER ) == false" );
		}

		if ( !chain_break_screening_pose.residue( five_prime_res + 1 ).has_variant_type( chemical::CUTPOINT_UPPER ) ) {
			utility_exit_with_message( "chain_break_screening_pose.residue( five_prime_chain_break_res + 1 ).has_variant_type( chemical::CUTPOINT_UPPER ) == false" );
		}

		if ( reinitialize_CCD_torsions_ ) set_CCD_torsions_to_zero( chain_break_screening_pose, five_prime_res );

		//		Real const mean_dist_err=rna_loop_closer.apply( chain_break_screening_pose, five_prime_res);
		rna_loop_closer.apply( chain_break_screening_pose, five_prime_res );

		( *chain_break_scorefxn )( chain_break_screening_pose );

		scoring::EMapVector & energy_map = chain_break_screening_pose.energies().total_energies();
		Real const angle_score = energy_map[scoring::angle_constraint];
		Real const distance_score = energy_map[scoring::atom_pair_constraint];

		if ( angle_score < 5 ) count_data_.good_angle_count++;
		if ( distance_score < 5 ) count_data_.good_distance_count++;
		if ( ( angle_score < 5 ) && ( distance_score < 5 ) ){
			count_data_.chain_break_screening_count++;
			if ( verbose_ ){
				//				TR.Debug << " C5_O3= " << C5_O3_distance << " C5_O3_n= " << count_data_.C5_O3_distance_count;
				TR.Debug << "  chain_closable_count = " << count_data_.chain_closable_count;
				TR.Debug << " angle = " << angle_score << " dist = " << distance_score;
				TR.Debug << " angle_n = " << count_data_.good_angle_count;
				TR.Debug << " dist_n = " << count_data_.good_distance_count;
				TR.Debug << " chain_break_screening = " << count_data_.chain_break_screening_count;
				TR.Debug << " tot = " << count_data_.tot_rotamer_count << std::endl;
			}
			return true;
		} else {
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool
	ChainBreakScreener::check_screen(){
		chain_break_screening_general( chain_break_screening_pose_, chain_break_scorefxn_, five_prime_res_ );
	}


} //screener
} //rna
} //swa
} //protocols
