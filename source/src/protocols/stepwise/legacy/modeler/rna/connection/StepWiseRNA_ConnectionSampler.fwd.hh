// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/modeler/rna/rigid_body/StepWiseRNA_ConnectionSampler.fwd.hh
/// @brief
/// @detailed
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_rna_StepWiseRNA_ConnectionSampler_FWD_HH
#define INCLUDED_protocols_stepwise_rna_StepWiseRNA_ConnectionSampler_FWD_HH

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace stepwise {
namespace legacy {
namespace modeler {
namespace rna {
namespace connection {

	class StepWiseRNA_ConnectionSampler;
	typedef utility::pointer::owning_ptr< StepWiseRNA_ConnectionSampler > StepWiseRNA_ConnectionSamplerOP;
	typedef utility::pointer::owning_ptr< StepWiseRNA_ConnectionSampler const > StepWiseRNA_ConnectionSamplerCOP;

} //connection
} //rna
} //modeler
} //legacy
} //stepwise
} //protocols

#endif