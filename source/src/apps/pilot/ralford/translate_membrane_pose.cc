// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file 		src/apps/pilot/ralford/translate_membrane_pose.cc
///
/// @brief 		Transform Structure into membrane center/normal
/// @details	Given an existing center and normal describing the position of the membrane, transform
///				the structure into a new membrane (where the transform mover is constructed with such normal and center
///				and the default new normal/center is center = (0, 0, 0) and normal = (0, 0, 1))
///
/// 			Last Modified: 6/27/14
///				Note: Part of the Membrane Framework Applications Suite
///				Versioning: 1.0
///
/// @author 	Rebecca Alford (rfalford12@gmail.com)

// Unit Headers
#include <devel/init.hh>

// Package Headers
#include <protocols/membrane/AddMembraneMover.hh> 
#include <protocols/membrane/TransformIntoMembraneMover.hh> 

// Project Headers
#include <protocols/jd2/JobDistributor.hh> 
#include <protocols/jd2/util.hh>

#include <core/pose/Pose.hh> 
#include <core/conformation/Conformation.hh> 

// Utility Headers
#include <utility/vector1.hh>
#include <utility/excn/Exceptions.hh>

#include <numeric/random/random.hh> 
#include <utility/pointer/owning_ptr.hh> 

#include <basic/Tracer.hh> 

// C++ headers
#include <iostream>

static basic::Tracer TR( "apps.pilot.ralford.translate_membrane_pose" );

using namespace protocols::moves;

/// @brief Membrane Transform Mover: Transform a pose into a new mover
class MembraneTransformMover : public Mover {

public: 

	MembraneTransformMover() {}

	/// @brief Get Mover Name
	std::string get_name() const { return "MembraneTransformMover"; }

	/// @brief Apply Membrane Relax
	void apply( Pose & pose ) {

		using namespace core::scoring; 
		using namespace core::kinematics; 
		using namespace protocols::membrane;
	 	using namespace numeric; 

		// Add a Membrane
		AddMembraneMoverOP add_memb = new AddMembraneMover(); 
		add_memb->apply(pose); 

		// Apply Transformation Move
		TransformIntoMembraneMoverOP transform = new TransformIntoMembraneMover(); 
		transform->apply( pose ); 

		// Done! 
	}
};

typedef utility::pointer::owning_ptr< MembraneTransformMover > MembraneTransformMoverOP; 
typedef utility::pointer::owning_ptr< MembraneTransformMover const > MembraneTransformMoverCOP; 

/// @brief Main method
int
main( int argc, char * argv [] )
{
	try {

		// Devel init factories
		devel::init(argc, argv);

		// Register JD2 options
		protocols::jd2::register_options();

		// Minimize with mp
		MembraneTransformMoverOP mptrans = new MembraneTransformMover();
		protocols::jd2::JobDistributor::get_instance()->go( mptrans );

		return 0; 

	} catch ( utility::excn::EXCN_Base const & e ) {
		std::cout << "caught exception " << e.msg() << std::endl;
		return -1;
	}
}
