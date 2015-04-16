// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   src/protocols/helical_bundle/BackboneGridSamplerHelper.cc
/// @brief  A class that stores the various mainchain torsion values that will be sampled by the
/// BackboneGridSampler mover.
/// @author Vikram K. Mulligan (vmullig@uw.edu)

// Unit header
#include <protocols/helical_bundle/BackboneGridSamplerHelper.hh>

// Package headers

// Project headers

// Basic headers
#include <basic/basic.hh>
#include <basic/Tracer.hh>

// Numeric headers
#include <numeric/random/random.hh>

// Utility Headers
#include <utility/assert.hh>
#include <utility/py/PyAssert.hh>
#include <utility/vector1.hh>

// C++ Headers
#include <iostream>
#include <stdio.h>


namespace protocols {
	namespace helical_bundle {

		static thread_local basic::Tracer TR( "protocols.helical_bundle.BackboneGridSamplerHelper" );

		/// @brief Constructor.
		///
		BackboneGridSamplerHelper::BackboneGridSamplerHelper() :
			n_torsions_(0),
			allowed_torsion_indices_(),
			torsion_samples_(),
			torsion_lower_vals_(),
			torsion_upper_vals_(),
			torsion_sample_vals_(),
			cur_indices_()
		{
		}

		BackboneGridSamplerHelper::BackboneGridSamplerHelper( BackboneGridSamplerHelper const & src ) :
			utility::pointer::ReferenceCount(),
			utility::pointer::enable_shared_from_this< BackboneGridSamplerHelper >(),
			n_torsions_( src.n_torsions_ ),
			allowed_torsion_indices_( src.allowed_torsion_indices_ ),
			torsion_samples_(src.torsion_samples_),
			torsion_lower_vals_(src.torsion_lower_vals_),
			torsion_upper_vals_(src.torsion_upper_vals_),
			torsion_sample_vals_(src.torsion_sample_vals_),
			cur_indices_( src.cur_indices_ )
		{
		}

		BackboneGridSamplerHelper::~BackboneGridSamplerHelper() {}


		///@brief make a copy of this BackboneGridSamplerHelper object (allocate actual memory for it)
		///
		BackboneGridSamplerHelperOP
		BackboneGridSamplerHelper::clone() const
		{
			return BackboneGridSamplerHelperOP( new BackboneGridSamplerHelper( *this ) );
		}

		/// @brief Reset this BackboneGridSamplerHelper object.
		/// @details Clears all internal data.
		void BackboneGridSamplerHelper::reset() {
			n_torsions_=0;
			allowed_torsion_indices_.clear();
			torsion_samples_.clear();
			torsion_lower_vals_.clear();
			torsion_upper_vals_.clear();
			torsion_sample_vals_.clear();
			cur_indices_.clear();
			return;
		}

		/// @brief Perform the pre-calculation that sets up the lists of torsion values to be sampled.
		///
		void BackboneGridSamplerHelper::initialize_samples() {
			if(TR.visible()) TR << "Initializing samples." << std::endl;
			torsion_sample_vals_.clear();
			cur_indices_.clear();

			for(core::Size i=1, imax=n_torsions(); i<=imax; ++i) {

				utility::vector1 < core::Real > curvect;
				
				core::Real lower = torsion_lower_vals_[i];
				core::Real upper = torsion_upper_vals_[i];
				core::Real increment = (upper-lower) / static_cast<core::Real>(torsion_samples_[i] - 1);
				
				//Special case: sampling the whole range (in which case we don't want to re-sample +/- 180).
				if(lower <= -180 && upper >= 180)
				{
					increment = 360 / static_cast<core::Real>(torsion_samples_[i]);
					lower = -180;
					upper = 180-increment;
				}

				if(TR.visible()) {TR << "lower=" << lower << " upper=" << upper << " increment=" << increment << std::endl; TR.flush();} //DELETE ME!

				runtime_assert_string_msg( lower < upper,
					"Internal error in BackboneGridSamplerHelper::initialize_samples().  Lower values in torsion ranges must be less than upper values." );
				runtime_assert_string_msg( torsion_samples_[i] > 1,
					"Internal error in BackboneGridSamplerHelper::initialize_samples().  Sampled torsion values must have at least two samples." );
				core::Real curval = lower;

				for(core::Size j=1; j<=torsion_samples_[i]; ++j) {
					curvect.push_back(curval);
					curval+=increment;
				}

				torsion_sample_vals_.push_back( curvect );
				cur_indices_.push_back(1); //Initialize this vector to (1,1,1,...)
			} //Looping through all torsions.

			if (TR.visible()) {
				TR << "The following torsion values will be sampled:" << std::endl;
				char outstring[1024];
				sprintf(outstring, "BB_Tors_index\tTorsion_values" );
				TR << outstring << std::endl;

				for(core::Size i=1, imax=n_torsions(); i<=imax; ++i) {
					sprintf(outstring, "%lu\t", static_cast<unsigned long>(allowed_torsion_indices_[i]) );
					TR << outstring;
					for(core::Size j=1, jmax=torsion_sample_vals_[i].size(); j<=jmax; ++j) {
						sprintf(outstring, "%.4f\t", static_cast<double>(torsion_sample_vals_[i][j]));
						TR << outstring;
					}
					TR << std::endl;
				}
				TR << std::endl;
				TR.flush();
			}

			return;
		}

		/// @brief RECURSIVE function that increments the current sample index.
		/// @details This function adds 1 to the last mainchain torsion index.  If the last torsion
		/// index value exceeds the number of samples for that torsion, it resets that
		/// torsion index and increments the second-last torsion index by 1 by recursively
		/// calling itself.  This function is overloaded; the public version by
		/// default tries to increment the last index, while the private version
		/// can be called recursively.
		void BackboneGridSamplerHelper::increment_cur_indices()
		{
			increment_cur_indices( n_torsions() ); //Call the private, recursive function.
			return;
		}

		/// @brief RECURSIVE function that increments the current sample index.
		/// @details This function adds 1 to the last torsion index.  If the last torsion
		/// index value exceeds the number of samples for that torsion, it resets that
		/// torsion index and increments the second-last torsion index by 1 by recursively
		/// calling itself.
		void BackboneGridSamplerHelper::increment_cur_indices( core::Size const index_to_increment )
		{
			runtime_assert_string_msg( index_to_increment > 0, "In BackboneGridSamplerHelper::increment_cur_indices(): all possible permutations have already been sampled!" );

			++(cur_indices_[index_to_increment]);
			if(cur_indices_[index_to_increment] > torsion_sample_vals_[index_to_increment].size()) {
				cur_indices_[index_to_increment] = 1;
				increment_cur_indices( index_to_increment - 1 );
			}

			return;
		}
		
		/// @brief Initialize this object from an object passed from the BackboneGridSampler mover.
		/// 
		void BackboneGridSamplerHelper::initialize_data(
			utility::vector1 <std::pair <core::Size /*mainchain torsion index*/, std::pair < std::pair < core::Real /*start of range to sample*/, core::Real /*end of range to sample*/ >, core::Size /*samples*/ > > > const &torsions_to_sample
		) {
			reset();
			n_torsions_ = torsions_to_sample.size();
			
			for(core::Size i=1; i<=n_torsions_; ++i) {
				allowed_torsion_indices_.push_back( torsions_to_sample[i].first );
				torsion_samples_.push_back( torsions_to_sample[i].second.second );
				torsion_lower_vals_.push_back( torsions_to_sample[i].second.first.first );
				torsion_upper_vals_.push_back( torsions_to_sample[i].second.first.second );
			}
			
			initialize_samples();
			
			return;
		}

	} // namespace helical_bundle
} // namespace protocols
