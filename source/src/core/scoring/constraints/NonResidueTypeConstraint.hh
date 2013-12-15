// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/scoring/constraints/ResidueConstraint.hh
///
/// @brief implements favor_non_native_residue constraints, assigning an energy bonus
/// to a residue if it matches the identity within the constraint object
/// @author Sarel Fleishman


#ifndef INCLUDED_core_scoring_constraints_NonResidueTypeConstraint_HH
#define INCLUDED_core_scoring_constraints_NonResidueTypeConstraint_HH

#include <core/scoring/constraints/NonResidueTypeConstraint.fwd.hh>
#include <core/scoring/constraints/Constraint.hh>

// AUTO-REMOVED #include <core/pose/Pose.hh>
#include <core/conformation/Conformation.fwd.hh>
#include <core/id/AtomID.hh>

// AUTO-REMOVED #include <utility/vector1.hh>

#include <utility/vector1.hh>


namespace core {
namespace scoring {
namespace constraints {

///@brief This class favors a particular residue identity at a particular position by reducing its res_type energy.
class NonResidueTypeConstraint : public Constraint
{
public:
	typedef core::Real Real;
public:

	NonResidueTypeConstraint(
		pose::Pose const & pose,
		Size seqpos,
		Real non_native_residue_bonus
	);


	NonResidueTypeConstraint(
	  pose::Pose const & pose,
	  Size seqpos,
	  std::string AAname,
	  Real favor_non_native_bonus
	);

	NonResidueTypeConstraint(
		Size seqpos,
		std::string aa_in,
		std::string name3_in,
		core::Real bonus_in
	);

	virtual ~NonResidueTypeConstraint();

	virtual
	Size
	natoms() const { return 0; }

	virtual
	AtomID const &
	atom( Size const ) const {
		utility_exit_with_message("NonResidueTypeConstraint is not atom-based!.");
		return core::id::BOGUS_ATOM_ID;  // required for compilation on Windows
	}

	virtual
	utility::vector1< core::Size >
	residues() const;

	void
	show( std::ostream & out ) const;

	virtual
	ConstraintOP
	remap_resid( core::id::SequenceMapping const &seqmap ) const;

	virtual
	void
	score( func::XYZ_Func const & xyz_func, EnergyMap const & weights, EnergyMap & emap ) const;

	virtual
	void
	fill_f1_f2(
		AtomID const & atom,
		func::XYZ_Func const & xyz,
		Vector & F1,
		Vector & F2,
		EnergyMap const & weights
	) const;

	virtual ConstraintOP
	clone() const;
private:
	Size seqpos_;
	std::string AAname;
	std::string rsd_type_name3_;
	core::Real favor_non_native_bonus_;
}; // RotamerConstraint


} // namespace constraints
} // namespace scoring
} // namespace core

#endif // INCLUDED_core_scoring_constraints_NonResidueTypeConstraint_HH
