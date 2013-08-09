// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file    core/chemical/carbohydrates/RingConformerSet.fwd.hh
/// @brief   Forward declarations for RingConformerSet.
/// @author  Labonte

#ifndef INCLUDED_core_chemical_carbohydrates_RingConformerSet_FWD_HH
#define INCLUDED_core_chemical_carbohydrates_RingConformerSet_FWD_HH

// Utility header
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace chemical {
namespace carbohydrates {

/// @brief  A structure for storing information for specific, idealized ring conformers.
struct RingConformer;

/// @brief  A class containing data and methods for the set of possible ring conformations for a carbohydrate residue.
class RingConformerSet;

typedef utility::pointer::owning_ptr<RingConformer> RingConformerOP;
typedef utility::pointer::owning_ptr<RingConformer const> RingConformerCOP;

typedef utility::pointer::owning_ptr<RingConformerSet> RingConformerSetOP;
typedef utility::pointer::owning_ptr<RingConformerSet const> RingConformerSetCOP;

}  // namespace carbohydrates
}  // namespace chemical
}  // namespace core

#endif  // INCLUDED_core_chemical_carbohydrates_RingConformerSet_FWD_HH
