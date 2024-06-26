// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file       protocols/membrane/SpinAroundPartnerMover.hh
/// @brief      Spins the downstream partner around the upstream partner
/// @details Spins the downstream partner around the upstream partner in the
///    membrane to probe all kinds of interfaces. Both embedding normals
///    are approximately conserved, i.e. the partners aren't flipped
///    in the membrane.
/// @author     JKLeman (julia.koehler1982@gmail.com)

#ifndef INCLUDED_protocols_membrane_SpinAroundPartnerMover_hh
#define INCLUDED_protocols_membrane_SpinAroundPartnerMover_hh

// Unit Headers
#include <protocols/membrane/SpinAroundPartnerMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Project Headers

// Package Headers
#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>

// Utility Headers
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace membrane {

class SpinAroundPartnerMover : public protocols::moves::Mover {

public:

	/////////////////////
	/// Constructors  ///
	/////////////////////

	/// @brief Default Constructor
	/// @details Defaults: jump = 1, sampling range = 100
	///    Sampling range of 100 means that both x and y are sampled from
	///    -100 to +100
	SpinAroundPartnerMover();

	/// @brief Custom Constructor
	/// @details User can specify jump number
	SpinAroundPartnerMover( core::Size jump_num );

	/// @brief Custom constructor
	/// @details User can specify jump number and sampling range
	SpinAroundPartnerMover( core::Size jump_num, core::Size range );

	/// @brief Copy Constructor
	SpinAroundPartnerMover( SpinAroundPartnerMover const & src );

	/// @brief Assignment Operator
	SpinAroundPartnerMover &
	operator = ( SpinAroundPartnerMover const & src );

	/// @brief Destructor
	~SpinAroundPartnerMover() override;

	///////////////////////////////
	/// Rosetta Scripts Methods ///
	///////////////////////////////

	/// @brief Create a Clone of this mover
	protocols::moves::MoverOP clone() const override;

	/// @brief Create a Fresh Instance of this Mover
	protocols::moves::MoverOP fresh_instance() const override;

	/// @brief Pase Rosetta Scripts Options for this Mover
	void parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap &
	) override;

	/// @brief Provide xml schema for RosettaScripts compatibility
	static void provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

	/////////////////////
	/// Mover Methods ///
	/////////////////////

	/// @brief Get the name of this Mover (SpinAroundPartnerMover)
	std::string get_name() const override;

	/// @brief Get the jump number for this Mover
	core::Size get_jump() const;

	/// @brief Get the random range for this Mover
	bool get_rand_range() const;

	/// @brief Get the sampling range for this Mover
	core::Size get_range() const;

	/// @brief Get the x position for this Mover
	core::Real get_x() const;

	/// @brief Get the y position for this Mover
	core::Real get_y() const;

	/// @brief Flip the downstream partner in the membrane
	void apply( core::pose::Pose & pose ) override;

	/// @brief Set random range
	void random_range( bool yesno );

	/// @brief Set x position
	void set_x( core::Real x );

	/// @brief Set y position
	void set_y( core::Real y );

private: // methods

	/////////////////////
	/// Setup Methods ///
	/////////////////////

	/// @brief Register Options from Command Line
	void register_options();

	/// @brief Set default values
	void set_defaults();

private: // data

	/// @brief Jump number
	core::Size jump_;

	// @brief Random range
	bool rand_range_;

	/// @brief Sampling range
	core::Size range_;

	/// @brief Set x and y position
	core::Real x_;
	core::Real y_;

	// Tell the compiler that we are not hiding the base
	// function with the parse_my_tag written above
	using protocols::moves::Mover::parse_my_tag;

};

} // membrane
} // protocols

#endif // INCLUDED_protocols_membrane_SpinAroundPartnerMover_hh
