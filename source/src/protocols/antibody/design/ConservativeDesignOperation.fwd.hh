// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/design/ConservativeDesignOperation.fwd.hh
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)


#ifndef INCLUDED_protocols_antibody_design_ConservativeDesignOperation_fwd_hh
#define	INCLUDED_protocols_antibody_design_ConservativeDesignOperation_fwd_hh

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace antibody {
namespace design {

class ConservativeDesignOperation;

typedef utility::pointer::owning_ptr< ConservativeDesignOperation > ConservativeDesignOperationOP;
typedef utility::pointer::owning_ptr< ConservativeDesignOperation const > ConservativeDesignOperationCOP;

} //namespace protocols
} //namespace toolbox
} //namespace task_operations

#endif	//INCLUDED_protocols_antibody_design_ConservativeDesignOperation.fwd.hh

