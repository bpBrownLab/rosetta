// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/features/ProteinBondGeometryFeatures.hh
/// @brief  report protein backbone torsion angle features
/// @author Patrick Conway

#ifndef INCLUDED_protocols_features_ProteinBondGeometryFeatures_hh
#define INCLUDED_protocols_features_ProteinBondGeometryFeatures_hh

// Unit Headers
#include <core/scoring/methods/CartesianBondedEnergy.fwd.hh>
#include <protocols/features/FeaturesReporter.hh>
#include <protocols/features/ProteinBondGeometryFeatures.fwd.hh>

//External
#include <boost/uuid/uuid.hpp>

// Project Headers
#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>
#include <utility/vector1.fwd.hh>

// C++ Headers
#include <string>

#include <utility/vector1.hh>


namespace protocols{
namespace features{

class ProteinBondGeometryFeatures : public protocols::features::FeaturesReporter {
public:
	ProteinBondGeometryFeatures();

	ProteinBondGeometryFeatures( ProteinBondGeometryFeatures const & src );

	virtual ~ProteinBondGeometryFeatures();

	///@brief return string with class name
	std::string
	type_name() const;

	///@brief generate the table schemas and write them to the database
	void
	write_schema_to_db(
		utility::sql_database::sessionOP db_session) const;

private:
	///@brief generate the bond_intrares_angles table schema
	void
	write_bond_intrares_angles_table_schema(
		utility::sql_database::sessionOP db_session) const;

	///@brief generate the bond_interres_angles table schema
	void
	write_bond_interres_angles_table_schema(
		utility::sql_database::sessionOP db_session) const;

	///@brief generate the bond_intrares_lengths table schema
	void
	write_bond_intrares_lengths_table_schema(
		utility::sql_database::sessionOP db_session) const;

	///@brief generate the bond_interres_lengths table schema
	void
	write_bond_interres_lengths_table_schema(
		utility::sql_database::sessionOP db_session) const;

	///@brief generate the bond_intrares_torsions table schema
	void
	write_bond_intrares_torsions_table_schema(
		utility::sql_database::sessionOP db_session) const;


public:
	///@brief return the set of features reporters that are required to
	///also already be extracted by the time this one is used.
	utility::vector1<std::string>
	features_reporter_dependencies() const;

	///@brief collect all the feature data for the pose
	core::Size
	report_features(
		core::pose::Pose const & pose,
		utility::vector1< bool > const & relevant_residues,
		boost::uuids::uuid struct_id,
		utility::sql_database::sessionOP db_session);

	void
	report_intrares_angles(
					core::pose::Pose const & pose,
					utility::vector1< bool > const & relevant_residues,
					boost::uuids::uuid struct_id,
					utility::sql_database::sessionOP db_session);

	void
	report_interres_angles(
							 core::pose::Pose const & pose,
							 utility::vector1< bool > const & relevant_residues,
							 boost::uuids::uuid struct_id,
							 utility::sql_database::sessionOP db_session);

	void
	report_intrares_lengths(
							 core::pose::Pose const & pose,
							 utility::vector1< bool > const & relevant_residues,
							 boost::uuids::uuid struct_id,
							 utility::sql_database::sessionOP db_session);

	void
	report_interres_lengths(
							core::pose::Pose const & pose,
							utility::vector1< bool > const & relevant_residues,
							boost::uuids::uuid struct_id,
							utility::sql_database::sessionOP db_session);

	void
	report_intrares_torsions(
							core::pose::Pose const & pose,
							utility::vector1< bool > const & relevant_residues,
							boost::uuids::uuid struct_id,
							utility::sql_database::sessionOP db_session);

	mutable core::scoring::methods::BondAngleDatabaseOP db_angle_;
	mutable core::scoring::methods::BondLengthDatabaseOP db_length_;
	mutable core::scoring::methods::TorsionDatabaseOP db_torsion_;

	// options
	bool linear_bonded_potential_;

};

} // namespace
} // namespace

#endif // include guard
