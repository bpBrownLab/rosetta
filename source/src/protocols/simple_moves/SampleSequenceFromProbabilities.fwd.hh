// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/simple_moves/SampleSequenceFromProbabilities.fwd.hh
/// @brief A class to sample sequences from a PerResidueProbabilitiesMetric and thread them onto the pose.
/// @author Moritz Ertelt (moritz.ertelt@gmail.com), University of Leipzig

#ifndef INCLUDED_protocols_simple_moves_SampleSequenceFromProbabilities_fwd_hh
#define INCLUDED_protocols_simple_moves_SampleSequenceFromProbabilities_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>


// Forward
namespace protocols {
namespace simple_moves {

class SampleSequenceFromProbabilities;

using SampleSequenceFromProbabilitiesOP = utility::pointer::shared_ptr< SampleSequenceFromProbabilities >;
using SampleSequenceFromProbabilitiesCOP = utility::pointer::shared_ptr< SampleSequenceFromProbabilities const >;

} //simple_moves
} //protocols

#endif //INCLUDED_protocols_simple_moves_SampleSequenceFromProbabilities_fwd_hh
