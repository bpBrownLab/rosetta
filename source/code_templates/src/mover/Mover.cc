// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file --path--/--class--.cc
/// @brief --brief--
/// @author --name-- (--email--)

// Unit headers
#include <--path--/--class--.hh>
#include <--path--/--class--Creator.hh>

// Core headers
#include <core/pose/Pose.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>

static THREAD_LOCAL basic::Tracer TR( "--namespace_dot--.--class--" );

--namespace--

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
--class--::--class--():
	protocols::moves::Mover( --class--::class_name() )
{

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Copy constructor
--class--::--class--( --class-- const & src ):
	protocols::moves::Mover( src )
{

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
--class--::~--class--(){}

////////////////////////////////////////////////////////////////////////////////
	/// Mover Methods ///
	/////////////////////

/// @brief Apply the mover
void
--class--::apply( core::pose::Pose& ){

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Show the contents of the Mover
void
--class--::show(std::ostream & output) const
{
	protocols::moves::Mover::show(output);
}

/// @brief Get the name of the Mover
std::string
--class--::get_name() const {
	return "--class--";
}

////////////////////////////////////////////////////////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
void
--class--::parse_my_tag(
	utility::tag::TagCOP ,
	basic::datacache::DataMap& ,
	protocols::filters::Filters_map const & ,
	protocols::moves::Movers_map const & ,
	core::pose::Pose const & )
{

}

////////////////////////////////////////////////////////////////////////////////
/// @brief required in the context of the parser/scripting scheme
moves::MoverOP
--class--::fresh_instance() const
{
	return protocols::moves::MoverOP( new --class-- );
}

/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
--class--::clone() const
{
	return protocols::moves::MoverOP( new --class--( *this ) );
}

protocols::moves::MoverOP
--class--::fresh_instance() const
{
	return protocols::moves::MoverOP( new --class-- );
}

std::string
--class--::get_name() const
{
	return --class--::class_name();
}

std::string
--class--::class_name()
{
	return "--class--";
}

void
--class--::show( std::ostream & output ) const
{
	protocols::moves::Mover::show( output );
}

std::ostream &
operator<<( std::ostream & os, --class-- const & mover )
{
	mover.show(os);
	return os;
}

////////////////////////////////////////////////////////////////////////////////
	/// Creator ///
	///////////////
void
--class--::apply( core::pose::Pose & )
{
}

/////////////// Creator ///////////////

protocols::moves::MoverOP
--class--Creator::create_mover() const
{
	return protocols::moves::MoverOP( new --class-- );
}

std::string
--class--Creator::keyname() const
{
	return --class--::class_name();
}

////////////////////////////////////////////////////////////////////////////////
	/// private methods ///
	///////////////////////

--end_namespace--

