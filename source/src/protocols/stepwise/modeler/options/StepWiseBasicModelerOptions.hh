// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/modeler/options/StepWiseBasicModelerOptions.hh
/// @brief
/// @detailed
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_modeler_StepWiseBasicModelerOptions_HH
#define INCLUDED_protocols_stepwise_modeler_StepWiseBasicModelerOptions_HH

#include <protocols/stepwise/modeler/options/StepWiseBasicModelerOptions.fwd.hh>
#include <protocols/stepwise/options/StepWiseBasicOptions.hh>
#include <core/types.hh>

namespace protocols {
namespace stepwise {
namespace modeler {
namespace options {

	class StepWiseBasicModelerOptions: public protocols::stepwise::options::StepWiseBasicOptions {

	public:

		//constructor
		StepWiseBasicModelerOptions();

		StepWiseBasicModelerOptions( StepWiseBasicModelerOptions const & src );

		//destructor
		~StepWiseBasicModelerOptions();

	public:

		StepWiseBasicModelerOptionsOP clone() const;

		StepWiseBasicModelerOptions &
		operator = ( StepWiseBasicModelerOptions const & src );

		/// @brief Initialize from the recursive "tag" structure.
		virtual
		void
		parse_my_tag( utility::tag::TagCOP ){}

		/// @brief The class name (its type) for a particular ResourceOptions instance.
		/// This function allows for better error message delivery.
		virtual
		std::string
		type() const{ return "StepWiseBasicModelerOptions";}

		core::Size const & sampler_num_pose_kept() const { return sampler_num_pose_kept_; }
		void set_sampler_num_pose_kept( core::Size const & setting ){ sampler_num_pose_kept_ = setting; }

		bool const & use_green_packer() const { return use_green_packer_; }
		void set_use_green_packer( bool const & setting ){ use_green_packer_ = setting; }

		bool const & verbose() const { return verbose_; }
		void set_verbose( bool const & setting ){ verbose_ = setting; }

		bool const & choose_random() const { return choose_random_; }
		void set_choose_random( bool const & setting ){ choose_random_ = setting; }

		void set_dump( bool const & setting ){ dump_ = setting; }
		bool dump() const{ return dump_; }

		core::Real const & cluster_rmsd() const { return cluster_rmsd_; }
		void set_cluster_rmsd( core::Real const & setting ){ cluster_rmsd_ = setting; }

		void set_skip_minimize( bool const & setting ){ skip_minimize_ = setting; }
		bool skip_minimize() const{ return skip_minimize_; }

		bool const & disallow_realign() const { return disallow_realign_; }
		void set_disallow_realign( bool const & setting ){ disallow_realign_ = setting; }

protected:

		void
		initialize_from_command_line();

    void
		initialize_variables();

protected:

		core::Size sampler_num_pose_kept_;
		bool use_green_packer_;
		bool verbose_;
		bool choose_random_;
		bool dump_;
		core::Real cluster_rmsd_;
		bool skip_minimize_;
		bool disallow_realign_;

	};

} //options
} //modeler
} //stepwise
} //protocols

#endif