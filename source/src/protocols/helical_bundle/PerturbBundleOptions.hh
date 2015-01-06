// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/helical_bundle/PerturbBundleOptions.hh
/// @brief  Prototypes and method declarations for the PerturbBundleOptions class.
/// @author Vikram K. Mulligan (vmullig@uw.edu)


#ifndef INCLUDED_protocols_helical_bundle_PerturbBundleOptions_hh
#define INCLUDED_protocols_helical_bundle_PerturbBundleOptions_hh


// Unit headers
#include <protocols/helical_bundle/PerturbBundleOptions.fwd.hh>

// Package headers
#include <core/conformation/Residue.hh>

// Project headers
#include <core/types.hh>

// Utility headers
#include <utility/pointer/access_ptr.hh>
#include <utility/pointer/owning_ptr.hh>
#include <utility/pointer/ReferenceCount.hh>
#include <utility/vector1.hh>

// Numeric headers

// C++ headers


namespace protocols {
	namespace helical_bundle {

		/// @brief The type of random perturbation that will be used.
		///
		enum PertType {
			pt_uniform = 1,
			pt_gaussian,
			pt_undefined_perturbation //keep this last
		};

		///@brief  PerturbBundleOptions class, which stores options for the PerturbBundle mover.
		///
		class PerturbBundleOptions : public utility::pointer::ReferenceCount, public utility::pointer::enable_shared_from_this< PerturbBundleOptions >
		{
			public:

				/// @brief constructors
				///
				PerturbBundleOptions();

				PerturbBundleOptions( PerturbBundleOptions const & src );

				~PerturbBundleOptions();

				/// @brief Copy this residue( allocate actual memory for it )
				///
				virtual
				PerturbBundleOptionsOP clone() const;

				/// self pointers
				inline PerturbBundleOptionsCOP get_self_ptr() const { return shared_from_this(); }
				inline PerturbBundleOptionsOP get_self_ptr() { return shared_from_this(); }
				inline PerturbBundleOptionsCAP get_self_weak_ptr() const { return PerturbBundleOptionsCAP( shared_from_this() ); }
				inline PerturbBundleOptionsAP get_self_weak_ptr() { return PerturbBundleOptionsAP( shared_from_this() ); }

			public: //Calculator

				/// @brief Function to generate a random value, based on the perturbation type and perturbation magnitude.
				/// @details Typically, one would then add this value to the current parameter value to perturb it.
				core::Real delta() const;

			public: //Getters

				/// @brief Returns the index of the helix that these options refer to.
				///
				core::Size helix_index() const { return helix_index_; }

				/// @brief Returns whether this option is set to be peturbable.
				///
				bool is_perturbable() const { return perturbable_; }

				/// @brief Returns the perturbation magnitude.  Returns 0 if not perturbable.
				core::Real perturbation_magnitude() const { return (perturbable_? perturbation_magnitude_ : 0.0); }

				/// @brief Returns the perturbation type.
				PertType perturbation_type() const { return perturbation_type_; }

				/// @brief Returns whether the default values should be used in lieu of whatever is set here.
				///
				bool use_defaults() const { return use_defaults_; }

				/// @brief Returns whether we're copying this value from another helix.
				///
				bool is_copy() const { return use_value_from_other_helix_!=0; }

				/// @brief Returns the other helix index from which we're copying a value.
				///
				core::Size other_helix() const { return use_value_from_other_helix_; }

			public: //Setters

				/// @brief Sets the index of the helix that these options refer to.
				///
				void set_helix_index(core::Size const val) { helix_index_=val; return; }

				/// @brief Sets whether this option is peturbable.
				///
				void set_perturbable(bool const val) { perturbable_=val; return; }

				/// @brief Sets the perturbation magnitude.  Sets perturbable to true in the process.
				///
				void set_perturbation_magnitude(core::Real const &val) {
					perturbation_magnitude_=val;
					perturbable_=true;
					use_defaults_=false;
					return;
				}

				/// @brief Sets the perturbation type.
				///
				void set_perturbation_type( PertType const type ) { perturbation_type_=type; return; }

				/// @brief Sets whether the default values should be used in lieu of whatever is set here.
				///
				void set_use_defaults( bool const val ) { use_defaults_=val; return; }

				/// @brief Sets the helix from which we should copy a parameter value.  If set to zero, no copying occurs.
				///
				void set_helix_to_copy(core::Size helix_index) { use_value_from_other_helix_=helix_index; use_defaults_=false; return; }

			private:

			/********************************************************************************
						PRIVATE DATA
			*********************************************************************************/

			/// @brief The helix index.
			/// @details Helix indices differ from indices of PerturbBundleOptions.  PerturbBundleOptions could be created in any order
			/// (e.g. in a RosettaScript), but refer to helices that are numbered in the bundle in the order of their declaration in the
			/// Conformation object's ParameterSet.  Here, a value of 0 will mean default; 1 or more will refer to specific helices.
			core::Size helix_index_;

			/// @brief The magnitude of the perturbation.
			///
			core::Real perturbation_magnitude_;

			/// @brief The type of perturbation.
			/// @details Options defined by enum.  Currently, "uniform" and "gaussian" are defined.
			PertType perturbation_type_;

			/// @brief Can this parameter be perturbed?
			///
			bool perturbable_;

			/// @brief Should we just use the default options for this parameter?
			///
			bool use_defaults_;

			/// @brief Should we just use the parameter value from another helix?
			/// @details Default value (0) means no; otherwise the helix index must be specified.
			core::Size use_value_from_other_helix_;

		}; //class PerturbBundleOptions

	} // namespace helical_bundle
} // namespace protocols

#endif