// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/import_pose/import_pose_options.hh
///
/// @brief
/// @author Brian D. Weitzner brian.weitzner@gmail.com

#ifndef INCLUDED_core_import_pose_import_pose_options_HH
#define INCLUDED_core_import_pose_import_pose_options_HH

//Core headers
#include <core/types.hh>

// Unit headers
#include <core/import_pose/import_pose_options.fwd.hh>
#include <core/io/pdb/pdb_dynamic_reader_options.hh>

// C++ headers
#include <string>


namespace core {
namespace import_pose {

/// @brief This class contains all of the data which is used in
/// the process of reading a PDB into a Pose.  There is actually
/// a substantial amount of data!
class ImportPoseOptions : public io::pdb::PDB_DReaderOptions
{
public:
	ImportPoseOptions();

	virtual ~ImportPoseOptions();

	virtual
	void parse_my_tag( utility::tag::TagCOP tag );

	virtual
	std::string type() const;

	// accessors
	bool centroid() const;
	bool fold_tree_io() const;
	bool membrane() const;
	bool no_optH() const;
	bool pack_missing_sidechains() const;
	bool read_fold_tree() const;
	bool rna() const;
	bool skip_set_reasonable_fold_tree() const;
	std::string const & residue_type_set() const;
	bool set_up_metal_bonds() const;
	bool set_up_metal_constraints() const;
	core::Real metal_bond_LJ_multiplier() const;
	core::Real metal_bond_dist_constraint_multiplier() const;
	core::Real metal_bond_angle_constraint_multiplier() const;

	// mutators
	void set_centroid( bool centroid );
	void set_fold_tree_io( bool fold_tree_io );
	void set_membrane( bool membrane );
	void set_no_optH( bool no_optH );
	void set_pack_missing_sidechains( bool pack_missing_sidechains );
	void set_read_fold_tree( bool read_fold_tree );
	void set_rna( bool rna );
	void set_skip_set_reasonable_fold_tree( bool skip_set_reasonable_fold_tree );
	void set_residue_type_set( std::string const & residue_type_set );
	void set_set_up_metal_bonds( bool invalue );
	void set_metal_bond_LJ_multiplier(core::Real invalue );
	void set_metal_bond_dist_constraint_multiplier(core::Real invalue );
	void set_metal_bond_angle_constraint_multiplier(core::Real invalue );

private:
	/// @brief Assigns user specified values to primitive members using command line options
	void init_from_options();

private:
	bool centroid_;
	bool fold_tree_io_;
	bool membrane_;
	bool no_optH_;
	bool pack_missing_sidechains_;
	bool read_fold_tree_;
	bool rna_;
	bool skip_set_reasonable_fold_tree_;
	bool set_up_metal_bonds_;
	core::Real metal_bond_LJ_multiplier_;
	core::Real metal_bond_dist_constraint_multiplier_;
	core::Real metal_bond_angle_constraint_multiplier_;

	std::string residue_type_set_;

};

} // namespace import_pose
} // namespace core

#endif // INCLUDED_core_import_pose_import_pose_options_HH
