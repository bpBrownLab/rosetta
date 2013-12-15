// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief

// Unit headers
#include <protocols/constraints_additional/COMCoordinateConstraint.hh>

// Package headers
#include <core/scoring/constraints/Constraint.hh>
#include <core/scoring/func/Func.hh>
#include <core/scoring/func/XYZ_Func.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/EnergyMap.hh>
#include <core/pose/util.hh>

// Project headers
#include <core/pose/Pose.hh>
#include <core/id/AtomID.hh>
#include <core/id/NamedAtomID.hh>

#include <core/chemical/ResidueType.hh>
#include <core/chemical/ResidueTypeSet.hh>
#include <numeric/model_quality/rms.hh>

#include <utility/string_util.hh>

// Utility Headers
#include <basic/Tracer.hh>
#include <math.h>

#include <core/chemical/ChemicalManager.fwd.hh>
#include <core/id/SequenceMapping.hh>
#include <core/util/SwitchResidueTypeSet.hh>
#include <utility/vector1.hh>
#include <numeric/xyz.functions.hh>

//Auto Headers
#include <core/conformation/Residue.hh>
#include <core/kinematics/Jump.hh>

static basic::Tracer tr("core.io.constraints");

namespace protocols {
namespace constraints_additional {

using namespace core;

///c-tor
COMCoordinateConstraint::COMCoordinateConstraint(
	utility::vector1< AtomID > const & atms,
	Vector const & COM_target,
	Real stdv,
	Real interval,
	scoring::ScoreType scoretype   /// ? TO DO -- give own scoretype
):
Constraint( scoretype ),
atms_(atms),
COM_target_(COM_target),
stdv_(stdv),
interval_(interval)
{}

COMCoordinateConstraint::COMCoordinateConstraint(
	utility::vector1< AtomID > const & atms,
	Vector const & COM_target,
	Real stdv,
	scoring::ScoreType scoretype   /// ? TO DO -- give own scoretype
):
Constraint( scoretype ),
atms_(atms),
COM_target_(COM_target),
stdv_(stdv),
interval_(0.0)
{}

COMCoordinateConstraint::COMCoordinateConstraint(
	utility::vector1< AtomID > const & atms,
	Vector const & COM_target,
	scoring::ScoreType scoretype   /// ? TO DO -- give own scoretype
):
Constraint( scoretype ),
atms_(atms),
COM_target_(COM_target),
stdv_(1.0),
interval_(0.0)
{}

void
COMCoordinateConstraint::score( scoring::func::XYZ_Func const& xyz,
																scoring::EnergyMap const &,
																scoring::EnergyMap & emap ) const
{
	Vector COM_pose( 0.0, 0.0, 0.0 );
	for (Size i=1; i<=(Size)atms_.size(); ++i) {
		COM_pose += xyz(atms_[i]);
	}

	COM_pose /= atms_.size();
	dCOM_ = COM_pose - COM_target_ ; // Save this for derivative evaluation
	Real dist2 = dCOM_.dot_product( dCOM_ );
	Real dist = sqrt( dist2 );
	Real score_val = -1;
	if( dist < interval_ ) score_val = 0;
	else score_val = pow( (dist - interval_)/stdv_ , 2 ) ;

	tr.Debug << "HARMONIC-func std("<<stdv_<<") interval("<<interval_<<") dist: "<<dist<<" constaint_score: "<<score_val<<std::endl;
	//std::cout << "HARMONIC-func std("<<stdv_<<") interval("<<interval_<<") dist^2: "<<dist2<<" constaint_score: "<<score_val<<std::endl;

	emap[ this->score_type() ] += score_val;
}

void
COMCoordinateConstraint::setup_for_scoring( scoring::func::XYZ_Func const & xyz,
																						scoring::ScoreFunction const & ) const
{
	// Take care of Centroid models
	for (Size i=1; i<=(Size)atms_.size(); ++i) {
		chemical::ResidueType const & rsd_type = xyz.residue( atms_[i].rsd()  ).type();
		id::AtomID atm_i;

		// is pose centroid?? then we need to remap atmids
		if ( rsd_type.residue_type_set().name() == chemical::CENTROID && atms_[i].atomno() > 5 ) {
			atm_i = id::AtomID( xyz.residue( atms_[i].rsd()  ).natoms() , atms_[i].rsd() );
		} else {
			atm_i = atms_[i];
		}
	}
}

// call the setup_for_derivatives for each constraint
void
COMCoordinateConstraint::setup_for_derivatives( scoring::func::XYZ_Func const & xyz, scoring::ScoreFunction const &scfxn ) const {
	setup_for_scoring( xyz, scfxn );
}

// atom deriv
void
COMCoordinateConstraint::fill_f1_f2(
	AtomID const & atom,
	scoring::func::XYZ_Func const & xyz,
	Vector & F1,
	Vector & F2,
	scoring::EnergyMap const & weights
) const	{

	if ( std::find( atms_.begin() , atms_.end() , atom ) == atms_.end()) return;

	Vector Ivec( 1.0, 1.0, 1.0 );

	Vector atom_x = -xyz( atom );
	Vector f2( 2.0*dCOM_/natoms() );
	//f2[0] /= xyz( atom )[0];
	//f2[1] /= xyz( atom )[1];
	//f2[2] /= xyz( atom )[2];

	Vector atom_y = f2 + atom_x;   // a "fake" atom in the direcion of the gradient
	Vector const f1( atom_x.cross( atom_y ) );

	F1 += f1*weights[ this->score_type() ];
	F2 += f2*weights[ this->score_type() ];
}

std::string
COMCoordinateConstraint::type() const {
	return "COMCoordinate";
}

///
Size
COMCoordinateConstraint::natoms() const
{
	return atms_.size();
}

id::AtomID const &
COMCoordinateConstraint::atom( Size const n ) const
{
	if ( n >= 1 && n <= atms_.size() )
		return atms_[n];
	else
		utility_exit_with_message( "COMCoordinateConstraint::atom() bad argument" );

	return atms_[1]; // stop the compiler from complaining
}

void
COMCoordinateConstraint::show( std::ostream& out ) const {
	out << "COMCoordinateConstraint::show() " << std::endl;
}

void
COMCoordinateConstraint::show_def( std::ostream&  out , pose::Pose const& /* pose */ ) const {
	out << "COMCoordinateConstraint::show_def() " << std::endl;
}

Size
COMCoordinateConstraint::show_violations( std::ostream& /*out*/, pose::Pose const& /*pose*/, Size /*verbose_level*/, Real /*threshold*/ ) const {
	return 0;
}

/// @brief Format should look like:
/// Dunbrack seqpos_ rot_vec_pos_ rot_bin_ bonus_
void
COMCoordinateConstraint::read_def(
	std::istream & line_stream,
	pose::Pose const & pose,
	scoring::func::FuncFactory const & /* func_factory */
) {

	Size res;
	std::string name;

	// to do? atm name -> centroid conversion
	utility::vector1< id::AtomID > atms;
	tr.Debug << "read: ";
	while ( line_stream >> name >> res ) {
		tr.Debug << "   " << name << " " << res ;
		atms.push_back( id::AtomID( pose.residue_type( res ).atom_index( name ), res ) );
		if ( res > pose.total_residue() ) {
			tr.Debug << "** ignored **";
			continue;
		}
	}
	tr.Debug << std::endl;
	atms_ = atms;
	//init( pose );
}

scoring::constraints::ConstraintOP COMCoordinateConstraint::remapped_clone(
			pose::Pose const& src,
			pose::Pose const& dest,
			id::SequenceMappingCOP smap
) const {

	using namespace core;
	using namespace core::pose;

	utility::vector1< AtomID > map_atom;

	bool validity = true;

	for (utility::vector1< AtomID >::const_iterator it = atms_.begin(), ite = atms_.end(); it != ite; ++it){
		id::NamedAtomID atom_temp( atom_id_to_named_atom_id( *it, src ) );
		if ( smap ){
			atom_temp.rsd() = (*smap)[ atom_temp.rsd() ];
		}

		id::AtomID new_id( named_atom_id_to_atom_id( atom_temp, dest, false /*exception*/) );

		if (new_id.valid()){
		} else {
			validity = false;
			break;
		}
		map_atom.push_back( new_id );
	}

	if (!validity) {
		return NULL;
	} else {
		return new COMCoordinateConstraint( map_atom, COM_target_, stdv_, interval_ );
	}
}


}
}
