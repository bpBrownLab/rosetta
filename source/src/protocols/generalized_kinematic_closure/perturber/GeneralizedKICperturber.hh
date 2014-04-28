// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/protocols/generalized_kinematic_closure/perturber/GeneralizedKICperturber.hh
/// @brief  Headers for GeneralizedKICperturber class (helper class for the GeneralizedKIC mover).
/// @author Vikram K. Mulligan (vmullig@uw.edu)

#ifndef INCLUDED_protocols_generalized_kinematic_closure_perturber_GeneralizedKICperturber_hh
#define INCLUDED_protocols_generalized_kinematic_closure_perturber_GeneralizedKICperturber_hh

// Unit Headers
//#include <protocols/moves/Mover.hh>
#include <protocols/generalized_kinematic_closure/GeneralizedKIC.fwd.hh>
#include <protocols/generalized_kinematic_closure/perturber/GeneralizedKICperturber.fwd.hh>

// Scripter Headers
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>

//// Project Headers
// AUTO-REMOVED #include <core/pose/Pose.hh>
// AUTO-REMOVED #include <core/grid/CartGrid.hh>
#include <utility/vector1.hh>
#include <numeric/xyzVector.hh>
#include <core/id/AtomID.hh>
#include <core/id/NamedAtomID.hh>
#include <core/conformation/Residue.hh>
#include <core/pose/Pose.hh>

#include <set>

#include <core/grid/CartGrid.fwd.hh>

// How to add new perturber effects:
// 1. Add a new entry in the perturber_effect enum list.
// 2. Add the perturber effect name to the get_perturber_effect_name function.
// 3. Add the perturber effect to the switch() statement in apply().
// 4. Create an apply_<perturber_effect_name>() function as a private method of the GeneralizedKICperturber class.

///////////////////////////////////////////////////////////////////////

namespace protocols {
namespace generalized_kinematic_closure {
namespace perturber {


/// @brief The effect of the perturber (e.g. randomizing alpha-amino acid phi/psi angles, altering side-chains, etc.)
/// @details  Perturbation effects starting with "set" set parameters to defined values (e.g. set the third phi angle
///           to 45 degrees).  Perturbation effects starting with "randomize" set parameters based on random
///           distributions, with no consideration of the initial value.  Perturbation effects starting with "jitter"
///           set parameters to some new value based on a random distribution about the initial value. 
enum perturber_effect {
	//When adding an effect to this enum, add its name to the get_perturber_effect_name() function.

	no_effect = 1,

	set_dihedral,
	set_bondangle,
	set_bondlength,

	randomize_dihedral,
	randomize_alpha_backbone_by_rama,
	//randomize_chi,
	//randomize_rotamer

	sample_cis_peptide_bond,

	unknown_effect, //Keep this second-to-last.
	end_of_effect_list = unknown_effect //Keep this last.
};


///////////////////////////////////////////////////////////////////////
//   GENERALIZED KIC PERTURBER CLASS                                 //
///////////////////////////////////////////////////////////////////////

class GeneralizedKICperturber : public utility::pointer::ReferenceCount
{
public:
	GeneralizedKICperturber();
	~GeneralizedKICperturber();

	std::string get_name() const;

  ///
	/// @brief Returns the enum type for the effect of this perturber. 
	perturber_effect get_perturber_effect() const { return effect_; }

  ///
	/// @brief Returns the enum type for the effect of a pertuber based on a perturber name.
	///        Returns unknown_effect if can't find a match for the name.
	perturber_effect get_perturber_effect_from_name( std::string const &name ) const;

  ///
	/// @brief Returns the name of a perturber given the enum type.
	///        Returns "unknown_effect" if no such effect exists.
	std::string get_perturber_effect_name( core::Size &effect ) const;

  ///
	/// @brief Sets the effect of this perturber. 
	void set_perturber_effect( perturber_effect const &effect );

  ///
	/// @brief Sets the effect of this perturber using the perturber effect name.
	///        Exits with an error message if the name is unknown.
	void set_perturber_effect( std::string const &effectname );

  ///
	/// @brief Adds a real value to the list of real input values.
	void add_inputvalue (core::Real const &inputvalue) { inputvalues_real_.push_back(inputvalue); return; }

	/// @brief Adds a residue to the list of residues on which this perturber should act.
	/// @details These are indices based on the original structure, NOT based on
	/// the loop to be closed.  Some effects act on a list of residues, while
	/// others act on a list of AtomIDs.
	void add_residue (core::Size const residue_index) { residues_.push_back(residue_index); return; }

	/// @brief Adds a set of atoms defining a bond length, bond angle, or dihedral
	/// angle to the list of sets of atoms on which this perturber should act.
	/// @details These are indices based on the original structure, NOT based on
	/// the loop to be closed.  Some effects act on a list of residues, while
	/// others act on a list of atoms.
	void add_atom_set ( utility::vector1< core::id::NamedAtomID > const &atomset ) { atoms_.push_back(atomset); return; }


	/// @brief Applies the perturbation to the vectors of desired torsions, desired angles, and desired bond lengths.
	///
	/// @detailed
	///
	/// @param[in] original_pose - The original pose.
	/// @param[in] loop_pose - A pose consisting of just the loop to be perturbed, plus one residue on each side establishing the frame.
	/// @param[in] residue_map - Mapping of (loop residue, original pose residue).
	/// @param[in] atomlist - List of atoms (residue indices are based on the loop_pose).
	/// @param[in,out] torsions - Desired torsions for each atom; can be set or altered by the apply() function.
	/// @param[in,out] bondangles - Desired bond angles for each atom; can be set or altered by the apply() function.
	/// @param[in,out] bondlengths - Desired bond lengths for each atom; can be set or altered by the apply() function.
	void apply (
		core::pose::Pose const &original_pose, //The original pose
		core::pose::Pose const &loop_pose, //A pose consisting of just the loop to be perturbed, plus one residue on each side establishing the frame
		utility::vector1< std::pair< core::Size, core::Size > > const &residue_map, //mapping of (loop residue, original pose residue)
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1< core::Real > &torsions, //desired torsions for each atom (input/output)
		utility::vector1< core::Real > &bondangles, //desired bond angle for each atom (input/output)
		utility::vector1< core::Real > &bondlengths //desired bond length for each atom (input/output)
	) const;


private:
////////////////////////////////////////////////////////////////////////////////
//          PRIVATE VARIABLES                                                 //
////////////////////////////////////////////////////////////////////////////////

	/// @brief The effect of this perturber.  See the perturber_effect enum type
	/// for more information.
	perturber_effect effect_;

	/// @brief Real input values for the operation.  For example, a set_dihedral
	/// operation would take a single real value (the value that the dihedral
	/// will be set to).
	utility::vector1 <core::Real> inputvalues_real_;

	/// @brief The list of residues on which this operation should be carried out.
	/// @details These are indices based on the original structure, NOT based on
	/// the loop to be closed.  Some effects act on a list of residues, while
	/// others act on a list of AtomIDs.
	utility::vector1 <core::Size> residues_;

	/// @brief A list of sets of atoms defining bond lenghts, bond angles, or
	/// dihedral angles on which this perturber will work.
	/// @details These are NamedAtomIDs (residue_index, atom_name).  The residue
	/// indices are based on the original structure, NOT on the loop to be
	/// closed.  Some effects act on a list of residues, while others act on a
	/// list of atoms.
	utility::vector1 < utility::vector1 < core::id::NamedAtomID > > atoms_;


////////////////////////////////////////////////////////////////////////////////
//          PRIVATE FUNCTIONS                                                 //
////////////////////////////////////////////////////////////////////////////////


	/// @brief Given an index in the original pose and a mapping from loop to pose,
	/// return the index in the loop.
	core::Size get_loop_index (
		core::Size const original_pose_index,
		utility::vector1 < std::pair < core::Size, core::Size > > const &residue_map
	) const;

	/// @brief Given a list of lists of AtomIDs, where residue indices are based on
	/// the original pose, and a mapping of original pose residue ID values to
	/// loop residue ID values, generate a new list of lists of AtomIDs, where the
	/// residue indices are based on the loop pose.
	void reindex_AtomIDs (
		utility::vector1 < std::pair < core::Size, core::Size > > const &residue_map, //input
		utility::vector1 < utility::vector1 < core::id::AtomID > > &AtomIDs_reindexed, //output
		core::pose::Pose const &original_pose //input -- for reference
	) const;

////////////////////////////////////////////////////////////////////////////////
//          APPLY FUNCTIONS FOR SPECIFIC EFFECTS                              //
////////////////////////////////////////////////////////////////////////////////

	/// @brief Applies a set_dihedral perturbation to a list of torsions.
	/// @details  Can also be used to randomize dihedral values.
	/// @param[in] dihedrallist - List of sets of atoms defining dihedrals, indexed based on the loop_pose.
	/// @param[in] atomlist - List of atoms (residue indices are based on the loop_pose).
	/// @param[in,out] torsions - Desired torsions for each atom; set by this function.
	/// @param[in] randomize - Should the specified torsions be randomized?
	void apply_set_dihedral (
		utility::vector1 < utility::vector1 < core::id::AtomID > > const &dihedrallist, //List of sets of atoms defining dihedrals, indexed based on the loop_pose.
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1< core::Real > &torsions, //desired torsions for each atom (input/output)
		bool const randomize
	) const;

	/// @brief Applies a set_bondangle perturbation to a list of bond angles.
	///
	/// @detailed
	///
	/// @param[in] bondanglelist - List of sets of atoms defining bond angles, indexed based on the loop_pose.
	/// @param[in] atomlist - List of atoms (residue indices are based on the loop_pose).
	/// @param[in,out] bondangles - Desired bond angles for each atom; set by this function.
	void apply_set_bondangle (
		utility::vector1 < utility::vector1 < core::id::AtomID > > const &bondanglelist, //List of sets of atoms defining bond angles, indexed based on the loop_pose.
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1< core::Real > &bondangles //desired bond angles for each atom (input/output)
	) const;

	/// @brief Applies a set_bondlength perturbation to a list of bond lengths.
	///
	/// @detailed
	///
	/// @param[in] bondlengthlist - List of sets of atoms defining bond lengths, indexed based on the loop_pose.
	/// @param[in] atomlist - List of atoms (residue indices are based on the loop_pose).
	/// @param[in,out] bondlengths - Desired bond lengths for each atom; set by this function.
	void apply_set_bondlength (
		utility::vector1 < utility::vector1 < core::id::AtomID > > const &bondlengthlist, //List of sets of atoms defining bond lengths, indexed based on the loop_pose.
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1< core::Real > &bondlengths //desired bond lengths for each atom (input/output)
	) const;

	/// @brief Applies a randomize_alpha_backbone_by_rama perturbation to the list of torsions.
	/// @details This checks whether each residue is an alpha-amino acid.
	/// @param[in] original_pose - The input pose.
	/// @param[in] loop_pose - A pose that is just the loop to be closed (possibly with other things hanging off of it).
	/// @param[in] residues - A vector of the indices of residues affected by this perturber.  Note that 
	/// @param[in] atomlist - A vector of pairs of AtomID, xyz coordinate.  Residue indices are based on the loop pose, NOT the original pose.
	/// @param[in] residue_map - A vector of pairs of (loop pose index, original pose index).
	/// @param[in,out] torsions - A vector of desired torsions, some of which are randomized by this function.
	void apply_randomize_alpha_backbone_by_rama(
		core::pose::Pose const &original_pose,
		core::pose::Pose const &loop_pose,
		utility::vector1 <core::Size> const &residues,
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1 < std::pair < core::Size, core::Size > > const &residue_map, //Mapping of (loop_pose, original_pose).
		utility::vector1< core::Real > &torsions //desired torsions for each atom (input/output)
	) const;

	/// @brief Applies a sample_cis_peptide_bond perturbation to the list of torsions.
	/// @details This checks whether each residue specified is an alpha- or beta-amino acid.  If it is, it samples the cis version of the omega angle (if omega is in the chain of atoms).
	/// @param[in] loop_pose - A pose that is just the loop to be closed (possibly with other things hanging off of it).
	/// @param[in] atomlist - A vector of pairs of AtomID, xyz coordinate.  Residue indices are based on the loop pose, NOT the original pose.
	/// @param[in] residues - A vector of the indices of residues affected by this perturber.  Note that 
	/// @param[in] residue_map - A vector of pairs of (loop pose index, original pose index).
	/// @param[in,out] torsions - A vector of desired torsions, some of which are randomized by this function.
	void apply_sample_cis_peptide_bond(
		core::pose::Pose const &loop_pose,
		utility::vector1 < std::pair < core::id::AtomID, numeric::xyzVector<core::Real> > > const &atomlist, //list of atoms (residue indices are based on the loop_pose)
		utility::vector1 <core::Size> const &residues,
		utility::vector1 < std::pair < core::Size, core::Size > > const &residue_map, //Mapping of (loop_pose, original_pose).
		utility::vector1< core::Real > &torsions //desired torsions for each atom (input/output)
	) const;

};

} //namespace perturber
} //namespace generalized_kinematic_closure
} //namespace protocols

#endif //INCLUDED_protocols_generalized_kinematic_closure_perturber_GeneralizedKICperturber_hh
