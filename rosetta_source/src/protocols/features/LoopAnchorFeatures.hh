// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/features/LoopAnchorFeatures.hh
/// @brief  report comments stored with each pose
/// @author Matthew O'Meara

#ifndef INCLUDED_protocols_features_LoopAnchorFeatures_hh
#define INCLUDED_protocols_features_LoopAnchorFeatures_hh

// Unit Headers
#include <protocols/features/FeaturesReporter.hh>
#include <protocols/features/LoopAnchorFeatures.fwd.hh>

// Project Headers
#include <core/types.hh>
#include <protocols/moves/DataMap.fwd.hh>
#include <numeric/HomogeneousTransform.fwd.hh>
#include <utility/sql_database/DatabaseSessionManager.fwd.hh>
#include <utility/vector1.fwd.hh>
#include <utility/tag/Tag.fwd.hh>

// C++ Headers
#include <string>

#include <utility/vector1.hh>


namespace protocols{
namespace features{

class LoopAnchorFeatures : public FeaturesReporter {
public:
	LoopAnchorFeatures();

	LoopAnchorFeatures(LoopAnchorFeatures const & );

	virtual ~LoopAnchorFeatures();

	///@brief return string with class name
	std::string
	type_name() const;

	///@brief return sql statements that setup the right tables
	std::string
	schema() const;

	///@brief return the set of features reporters that are required to
	///also already be extracted by the time this one is used.
	utility::vector1<std::string>
	features_reporter_dependencies() const;

	void
	parse_my_tag(
		utility::tag::TagPtr const tag,
		protocols::moves::DataMap & /*data*/,
		protocols::filters::Filters_map const & /*filters*/,
		protocols::moves::Movers_map const & /*movers*/,
		core::pose::Pose const & /*pose*/);

	///@brief collect all the feature data for the pose
	core::Size
	report_features(
		core::pose::Pose const & pose,
		utility::vector1< bool > const & /*relevant_residues*/,
		core::Size struct_id,
		utility::sql_database::sessionOP db_session);
private:

	numeric::HomogeneousTransform<core::Real>
	compute_anchor_transform(
		core::pose::Pose const & pose,
		core::Size residue_begin,
		core::Size residue_end);

private:

	core::Size min_loop_length_;
	core::Size max_loop_length_;

};

} // features namespace
} // protocols namespace

#endif //INCLUDED_protocols_features_LoopAnchorFeatures_hh
