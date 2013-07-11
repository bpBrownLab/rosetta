// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/design/util.cc
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#include <protocols/antibody/design/util.hh>
#include <boost/algorithm/string.hpp>
#include <string>
#include <utility/exit.hh>

namespace protocols {
namespace antibody {
namespace design {
	using namespace protocols::antibody;
	using namespace utility;
	
std::string
get_string_for_IN(core::Size const n){
	std::string result;
	if (n==1){
		result = "(?)";
		return result;
	}
	else{
		result = "(?";
		for (core::Size i =2; i<=n; ++i){
			result += ",?";
		}
		result += ")";
		return result;
	}
}

void
get_all_graft_permutations(
		vector1<core::Size > & total_cdr_set,
		vector1< vector1< core::Size> > & all_permutations,
		vector1<core::Size>current_index,
		core::Size const recurse_index
)
{
	//Current index is what is being worked on. 
	if (recurse_index>total_cdr_set.size()){return;}
	if (total_cdr_set[recurse_index]==0){
		current_index[recurse_index]=0;
		get_all_graft_permutations(total_cdr_set, all_permutations, current_index, recurse_index+1);
	}
	else{
		for(core::Size i = 1; i<=total_cdr_set[recurse_index]; ++i){
			current_index[recurse_index]=i;
			if (recurse_index==total_cdr_set.size()){
				all_permutations.push_back(current_index);
				continue;
			}
			get_all_graft_permutations(total_cdr_set, all_permutations, current_index, recurse_index+1);
		
		}
	}
}

DesignTypeEnum
design_type_from_string(std::string const design_type){
	std::string type = design_type;
	boost::to_upper(type);
	
	if (type == "FLXBB"){
		return flxbb;
	}
	else if(type == "FIXBB" || type == "FIXEDBB"){
		return fixbb;
	}
	else if(type == "RELAXED_DESIGN" || type == "RELAX_DESIGN" || type == "RELAX_TF"){
		return relaxed_design;
	}
	else{
		utility_exit_with_message("DesignType unrecognized.  Please check AntibodyDesign settings.");
	}
}

std::string
design_type_to_string(DesignTypeEnum const design_type){
	if (design_type == flxbb){
		return "FLXBB";
	}
	else if(design_type==fixbb){
		return "FIXBB";
	}
	else if(design_type==relaxed_design){
		return "RELAXED_DESIGN";
	}
	else{
		utility_exit_with_message("DesignType unrecognized.  Please check AntibodyDesign settings.");
	}
}


} //design
} //antibody
} //protocols
