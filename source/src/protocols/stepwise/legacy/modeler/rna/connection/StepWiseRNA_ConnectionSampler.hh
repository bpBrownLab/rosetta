// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/modeler/rna/rigid_body/StepWiseRNA_ConnectionSampler.hh
/// @brief
/// @detailed
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_rna_StepWiseRNA_ConnectionSampler_HH
#define INCLUDED_protocols_stepwise_rna_StepWiseRNA_ConnectionSampler_HH

#include <protocols/moves/MoverForPoseList.hh>
#include <protocols/stepwise/modeler/options/StepWiseModelerOptions.fwd.hh>
#include <protocols/stepwise/modeler/rna/StepWiseRNA_Classes.hh>
#include <protocols/stepwise/modeler/rna/rigid_body/FloatingBaseClasses.hh>
#include <protocols/stepwise/modeler/working_parameters/StepWiseWorkingParameters.fwd.hh>
#include <protocols/stepwise/legacy/modeler/rna/StepWiseRNA_PoseSelection.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_BaseCentroidChecker.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_VDW_BinChecker.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_AtrRepChecker.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_ChainClosureChecker.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_ChainClosableGeometryChecker.fwd.hh>
#include <protocols/stepwise/modeler/rna/o2prime/O2PrimePacker.fwd.hh>
#include <protocols/stepwise/modeler/rna/phosphate/MultiPhosphateSampler.fwd.hh>
#include <protocols/stepwise/screener/StepWiseScreener.hh>
#include <protocols/stepwise/screener/TagDefinition.hh>
#include <protocols/stepwise/sampler/copy_dofs/ResidueAlternativeStepWiseSamplerComb.fwd.hh>
#include <protocols/stepwise/sampler/copy_dofs/ResidueAlternativeSet.hh>
#include <protocols/stepwise/sampler/rigid_body/RigidBodyStepWiseSampler.fwd.hh>
#include <protocols/stepwise/sampler/rigid_body/RigidBodyStepWiseSamplerWithResidueList.fwd.hh>
#include <protocols/stepwise/sampler/rigid_body/RigidBodyStepWiseSamplerWithResidueAlternatives.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/io/silent/SilentFileData.fwd.hh>
#include <core/kinematics/Stub.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/conformation/Residue.fwd.hh>
#include <core/pack/task/PackerTask.fwd.hh>

using namespace core;

namespace protocols {
namespace stepwise {
namespace legacy {
namespace modeler {
namespace rna {
namespace connection {

	class StepWiseRNA_ConnectionSampler: public protocols::moves::MoverForPoseList {

	private:

		StepWiseRNA_ConnectionSampler(); // Can't use a default constructor

	public:

		//constructor
		StepWiseRNA_ConnectionSampler( stepwise::modeler::working_parameters::StepWiseWorkingParametersCOP & working_parameters_ );

		//destructor
		~StepWiseRNA_ConnectionSampler();

		virtual void apply( pose::Pose & pose_to_visualize );

		virtual std::string get_name() const;

		using MoverForPoseList::apply;

	public:

		void set_silent_file( std::string const & setting ){ silent_file_ = setting; }

		void set_scorefxn( scoring::ScoreFunctionCOP const & scorefxn );

		void set_pose_list( utility::vector1< pose::PoseOP > &	pose_list );

		utility::vector1< pose::PoseOP > &		get_pose_list();

		void
		set_user_input_VDW_bin_checker( checker::RNA_VDW_BinCheckerOP const & user_input_VDW_bin_checker );

		void
		add_residue_alternative_set( sampler::copy_dofs::ResidueAlternativeSet const & residue_alternative_set );

		void
		set_options( options::StepWiseModelerOptionsCOP options );

	private:

		void
		figure_out_reference_res( pose::Pose const & pose );

		void
		figure_out_reference_res_with_rigid_body_rotamer( pose::Pose const & pose );

		bool
		initialize_poses_and_checkers( pose::Pose & pose  );

		Size
		get_max_ntries();

		Size
		get_num_pose_kept();

		void
		reinstantiate_backbone_and_add_constraint_at_moving_res(	pose::Pose & pose, Size const & five_prime_chain_break_res );

		void
		initialize_euler_angle_grid_parameters();

		void
		initialize_xyz_grid_parameters();

		void
		initialize_sampler();

		void
		initialize_full_rigid_body_sampler();

		sampler::StepWiseSamplerBaseOP
		get_full_bond_sampler();

		sampler::copy_dofs::ResidueAlternativeStepWiseSamplerCombOP
		get_rsd_alternatives_rotamer();

		void
		initialize_rigid_body_rotamer();

		void
		initialize_screeners( pose::Pose & pose );

		void
		initialize_residue_level_screeners( pose::Pose & pose );

		void
		initialize_pose_level_screeners( pose::Pose & pose );

		void
		update_base_bin_map( rigid_body::BaseBin const & base_bin );

		void
		initialize_moving_residue_pose_list( pose::Pose const & pose );

		Size
		which_residue_alternative_set_is_moving_residue() const;

		utility::vector1< core::conformation::ResidueOP >	get_moving_rsd_list() const;

		Size truly_floating_base();

		void check_working_parameters( pose::Pose const & pose );

		bool
		presample_virtual_sugars( pose::Pose & pose );

	private:

		Size const moving_res_; // Might not corresponds to user input.
		Size reference_res_; //the last stationary residue that this attach to the moving residues
		utility::vector1< Size > moving_partition_res_;

		utility::vector1< pose::PoseOP > pose_list_;

		scoring::ScoreFunctionCOP scorefxn_;

		sampler::rigid_body::RigidBodyStepWiseSamplerOP rigid_body_rotamer_;
		protocols::stepwise::sampler::StepWiseSamplerBaseOP sampler_;
		utility::vector1< screener::StepWiseScreenerOP > screeners_;
		screener::TagDefinitionOP tag_definition_;

		StepWiseRNA_CountStruct count_data_;
		std::string silent_file_;
		bool kic_modeler_;
		bool rebuild_bulge_mode_;

		options::StepWiseModelerOptionsCOP options_;

		utility::vector1< sampler::copy_dofs::ResidueAlternativeSet > residue_alternative_sets_;

		checker::RNA_AtrRepCheckerOP atr_rep_checker_;
		checker::RNA_AtrRepCheckerOP virt_sugar_atr_rep_checker_;
		checker::RNA_VDW_BinCheckerOP VDW_bin_checker_;
		checker::RNA_VDW_BinCheckerOP user_input_VDW_bin_checker_;

		utility::vector1< Size > five_prime_chain_breaks_;
		utility::vector1< Size > three_prime_chain_breaks_;
		utility::vector1< Size > chain_break_gap_sizes_;
		utility::vector1< checker::RNA_ChainClosableGeometryCheckerOP > chain_closable_geometry_checkers_;

		utility::vector1< Size > cutpoints_closed_;
		utility::vector1< checker::RNA_ChainClosureCheckerOP > chain_closure_checkers_;

		checker::RNA_BaseCentroidCheckerOP base_centroid_checker_;

		pose::PoseOP screening_pose_, virt_sugar_screening_pose_;
		StepWiseRNA_PoseSelectionOP pose_selection_;

		o2prime::O2PrimePackerOP o2prime_packer_;
		phosphate::MultiPhosphateSamplerOP phosphate_sampler_;

		kinematics::Stub moving_res_base_stub_;
		rigid_body::BaseBinMap base_bin_map_;

		Real max_distance_squared_;

		bool rigid_body_modeler_;
		bool try_sugar_instantiation_;
		Distance o2prime_instantiation_distance_cutoff_;
		std::string const extra_tag_;
		bool const virt_sugar_atr_rep_screen_;
		bool const build_pose_from_scratch_;

		stepwise::modeler::working_parameters::StepWiseWorkingParametersCOP working_parameters_; //need to use the full_to_sub map...should convert to const style.. Parin Feb 28, 2010
	};

} //connection
} //rna
} //modeler
} //legacy
} //stepwise
} //protocols

#endif