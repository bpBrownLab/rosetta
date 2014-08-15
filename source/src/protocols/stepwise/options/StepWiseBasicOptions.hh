// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/options/StepWiseBasicOptions.hh
/// @brief
/// @detailed
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_options_StepWiseBasicOptions_HH
#define INCLUDED_protocols_stepwise_options_StepWiseBasicOptions_HH

#include <basic/resource_manager/ResourceOptions.hh>
#include <protocols/stepwise/options/StepWiseBasicOptions.fwd.hh>
#include <core/types.hh>

#include <utility/tag/Tag.fwd.hh>

#if defined(WIN32) || defined(PYROSETTA)
	#include <utility/tag/Tag.hh>
#endif

namespace protocols {
namespace stepwise {
namespace options {

	class StepWiseBasicOptions: public virtual basic::resource_manager::ResourceOptions {

	public:

		//constructor
		StepWiseBasicOptions();

		StepWiseBasicOptions( StepWiseBasicOptions const & src );

		//destructor
		~StepWiseBasicOptions();

	public:

		StepWiseBasicOptionsOP clone() const;

		StepWiseBasicOptions &
		operator = ( StepWiseBasicOptions const & src );

		/// @brief Initialize from the recursive "tag" structure.
		virtual
		void
		parse_my_tag( utility::tag::TagCOP ){}

		/// @brief The class name (its type) for a particular ResourceOptions instance.
		/// This function allows for better error message delivery.
		virtual
		std::string
		type() const{ return "StepWiseBasicOptions";}

		bool const & output_minimized_pose_list() const { return output_minimized_pose_list_; }
		void set_output_minimized_pose_list( bool const & setting ){ output_minimized_pose_list_ = setting; }

		std::string const & silent_file() const { return silent_file_; }
		void set_silent_file( std::string const & setting ){ silent_file_ = setting; }

		std::string const & sampler_silent_file() const { return sampler_silent_file_; }
		void set_sampler_silent_file( std::string const & setting ){ sampler_silent_file_ = setting; }

		core::Size const & num_pose_minimize() const { return num_pose_minimize_; }
		void set_num_pose_minimize( core::Size const & setting ){ num_pose_minimize_ = setting; }

		core::Size const & num_random_samples() const { return num_random_samples_; };
		void set_num_random_samples( core::Size const & setting ){ num_random_samples_ = setting; }

		void set_atr_rep_screen( bool const & setting ){ atr_rep_screen_ = setting; }
		bool atr_rep_screen() const{ return atr_rep_screen_; }

		void set_rmsd_screen( core::Real const & setting ){ rmsd_screen_ = setting; }
		core::Real rmsd_screen() const{ return rmsd_screen_; }

	protected:

		void
		initialize_from_command_line();

    void
		initialize_variables();

	private:

		std::string silent_file_;
		core::Size num_pose_minimize_;
		core::Size num_random_samples_;
		bool atr_rep_screen_;
		core::Real rmsd_screen_;
		bool output_minimized_pose_list_;
		std::string sampler_silent_file_;

	};

} //options
} //stepwise
} //protocols

#endif