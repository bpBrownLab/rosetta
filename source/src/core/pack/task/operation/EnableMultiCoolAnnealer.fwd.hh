// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file core/pack/task/operation/EnableMultiCoolAnnealer.fwd.hh
/// @brief Task Operation for turning on the multi-cool annealer
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)

#ifndef INCLUDED_core_pack_task_operation_EnableMultiCoolAnnealer_fwd_hh
#define INCLUDED_core_pack_task_operation_EnableMultiCoolAnnealer_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>


// Forward
namespace core {
namespace pack {
namespace task {
namespace operation {

class EnableMultiCoolAnnealer;

typedef utility::pointer::shared_ptr< EnableMultiCoolAnnealer > EnableMultiCoolAnnealerOP;
typedef utility::pointer::shared_ptr< EnableMultiCoolAnnealer const > EnableMultiCoolAnnealerCOP;

} //core
} //pack
} //task
} //operation

#endif //INCLUDED_core_pack_task_operation_EnableMultiCoolAnnealer_fwd_hh
