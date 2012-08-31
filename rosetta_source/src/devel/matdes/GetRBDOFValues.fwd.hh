// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   devel/matdes/GetRBDOFValues.fwd.hh
/// @brief
/// @author Jacob Bale (balej@u.washington.edu)


#ifndef INCLUDED_devel_matdes_GetRBDOFValues_fwd_hh
#define INCLUDED_devel_matdes_GetRBDOFValues_fwd_hh


// Utility headers
#include <utility/pointer/owning_ptr.fwd.hh>

namespace devel {
namespace matdes {

// Forward
class GetRBDOFValues;

// Types
typedef utility::pointer::owning_ptr< GetRBDOFValues >  GetRBDOFValuesOP;
typedef utility::pointer::owning_ptr< GetRBDOFValues const >  GetRBDOFValuesCOP;

} // namespace matdes
} // namespace devel

#endif
