// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/fold_from_loops/ResidueLabelsToPymolSelectionMover.hh
/// @brief Prints a Pymol selection command for each label in a pose
/// @author Jaume Bonet (jaume.bonet@gmail.com)

#ifndef INCLUDED_fold_from_loops_movers_ResidueLabelsToPymolSelectionMover_hh
#define INCLUDED_fold_from_loops_movers_ResidueLabelsToPymolSelectionMover_hh

// Unit headers
#include <protocols/fold_from_loops/movers/ResidueLabelsToPymolSelectionMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Protocol headers

// Core headers
#include <core/pose/Pose.fwd.hh>
#include <core/select/residue_selector/ResidueSelector.fwd.hh>

// Basic/Utility headers
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace fold_from_loops {
namespace movers {

///@brief Adds constraints generated by ConstraintGenerators to a pose
class ResidueLabelsToPymolSelectionMover : public protocols::moves::Mover {

public:
	ResidueLabelsToPymolSelectionMover();

	// destructor (important for properly forward-declaring smart-pointer members)
	~ResidueLabelsToPymolSelectionMover() override;

	void
	apply( core::pose::Pose & pose ) override;

	/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data
	) override;

	void pdb_count( bool action ){ pdb_count_ = action; };

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	fresh_instance() const override;

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	clone() const override;

	std::string
	get_name() const override;

	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

private:
	static bool default_pdb_count(){ return false; };

private:
	bool pdb_count_ = false;

};

}
} //protocols
} //fold_from_loops

#endif //INCLUDED_fold_from_loops_ResidueLabelsToPymolSelectionMover_hh
