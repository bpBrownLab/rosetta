// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/design/AntibodyGraftDesigner.hh
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)


#ifndef INCLUDED_protocols_antibody_design_AntibodyGraftDesigner_hh
#define INCLUDED_protocols_antibody_design_AntibodyGraftDesigner_hh

// Project Includes
#include <protocols/antibody/design/AntibodyGraftDesigner.fwd.hh>
#include <protocols/antibody/design/AntibodyDesignEnum.hh>
#include <protocols/antibody/design/AntibodyDesignModeler.hh>
#include <protocols/antibody/AntibodyEnum.hh>
#include <protocols/antibody/AntibodyInfo.hh>
#include <protocols/antibody/CDRClusterEnum.hh>

#include <protocols/grafting/AnchoredGraftMover.hh>


#include <core/scoring/ScoreFunction.hh>

// Core Includes
#include <core/pose/Pose.hh>

// Protocol Includes
#include <protocols/moves/Mover.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <core/types.hh>

namespace protocols{
namespace antibody{
namespace design{
	using namespace protocols::antibody;
	using namespace protocols::grafting;
	using namespace utility;
	using namespace core;
	using namespace core::scoring;
	using std::map;

	///@brief This struct gives antibody designers complete control either through this classes interface or an instruction file.
	struct CDRGraftInstructions {
		bool graft; //Use grafting algorithm on this CDR?
		bool stay_native_cluster; //Only sample within the native cluster?
		bool cluster_centers_only;//Only use the center clusters for sampling?
		MinTypeEnum mintype; //What to do after graft? (Could be a simple enum - but we would need a manager for it too.)
		std::map< Size,  bool > cluster_types; //Only sample within cluster types (Cluster types 1,2,3)
		vector1< std::string > leave_out_pdb_ids;
		vector1< std::string > include_only_pdb_ids;
		vector1< CDRClusterEnum > leave_out_clusters;//Leave out these clusters.  
		vector1< CDRClusterEnum > include_only_clusters; //Include only these clusters.
		core::Size min_length;
		core::Size max_length;
	};

	//Sampling weights not currently implemented
	struct SamplingWeights {
		std::map< Size,  Real > types; //Used to sample more of one type then others.  Sample most in type1, but use a bit of type2/3.
		Real native_cluster; //May be used to weight the sampling of native clusters vs others.
		Real cdr; //Used to sample more of one or particular CDRs rather then others (H3)
		Real center; //Used to sample more of cluster centers then other CDRs.
	};	
	
	
	typedef map< CDRNameEnum, vector1< pose::PoseOP > > CDRSet;
	typedef map< CDRNameEnum, vector1< CDRClusterEnum > > CDRClusterMap; 
	typedef std::map< CDRNameEnum, vector1< std::string > > PDBMap;
	
	
	
	
	
	
	
///@brief This class designs antibodies by grafting, using cluster information and constraints to help.
/// It represents the first step in The Rosetta Antibody Designer, but can be used outside of the application.
/// The Antibody Database that this class uses will be available online (Too large for the rosetta_database)
/// Till then, email me for access to the github repository.
///
///@details To use this class:
///			1) Use default instruction path, or set one.  Loads CDRs from AntibodyDatabase
///			    *See rosetta_database/sampling/antibodies for file format.
///			2) Use class interface settings to control sampling.  Loads CDRs from AntibodyDatabase.
///			3) Use set_cdr_sets to set your own CDRs to graft.
///  
class AntibodyGraftDesigner: public protocols::moves::Mover {

public:
	
	AntibodyGraftDesigner(AntibodyInfoOP & ab_info);
	
	//AntibodyGraftDesigner(AntibodyInfoOP & ab_info, std::string instruction_path);
			
	virtual ~AntibodyGraftDesigner();
	
	///@brief Reads default CDRGraftInstruction file and creates structs.  
	void
	set_defaults();
	
	//Parse my tag
	
	virtual void
	apply(pose::Pose & pose);
	
	
public:
	////////////////////////////////////////////////////////////////////////////
	// Full Settings
	//
	//
	
	///@brief Will not initialize CDRs from the AntibodyDatabase.  Use if you have your own CDR's you are interested in grafting.
	///@details Overhang residues will be used for superposition.  To identify your CDRs, use the functions in AntibodyInfo or the pilot app identify_cdr_clusters
	/// in pilot/jadolfbr/cluster_utils
	void
	set_cdr_set(CDRSet & cdr_set, CDRClusterMap & cdr_cluster_map, core::Size overhang); 
	
public:
	////////////////////////////////////////////////////////////////////////////
	// Modeling Settings
	//
	//
	
	void
	set_scorefunction(ScoreFunctionOP & scorefxn);
	
	void
	set_graft_rounds(core::Size graft_rounds);
	
	//void
	//set_filters();
	
	///@brief Options are: relax, min, repack.
	void
	set_mintype(CDRNameEnum const cdr_name, MinTypeEnum mintype);
	
	void
	set_mintype_range(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, MinTypeEnum mintype);
	
	///@brief Sets the protocol to keep a specific number of top designs.  Default is 10
	void
	set_keep_top_designs(core::Size top_designs);
	
	///@brief Get the top designs found.  You can then use them in other protocols, dump them, etc. They are in order. 
	///@details - This should be refactored to get_additional_output. 
	vector1< pose::PoseOP>
	get_top_designs(){
		return top_designs_;
	};
	
public:	
	////////////////////////////////////////////////////////////////////////////
	// Cluster Settings
	//
	//
	
	///@brief Leave out or include only clusters added here
	void
	set_cluster(CDRNameEnum const cdr, CDRClusterEnum const cluster, bool const setting);
	
	///@brief Leave out or include only clusters added here
	void
	set_cluster_range(
		CDRNameEnum const cdr, 
		CDRClusterEnum const cluster_start,
		CDRClusterEnum const cluster_end,
		bool const setting);

	
public:
	////////////////////////////////////////////////////////////////////////////
	// CDR Settings
	//
	//
	
	///@brief Use this CDR during design.
	void
	set_cdr(CDRNameEnum const cdr, bool const setting);
	
	///@brief Use this range of CDRs during design
	void
	set_cdr_range(CDRNameEnum const cdr_start, CDRNameEnum cdr_end, bool const setting);
	
	
	///@brief Set to only sample within the native cluster of this CDR.  
	void
	set_cdr_stay_in_native_cluster(CDRNameEnum const cdr, bool const setting);
	
	///@brief Set a range of CDRs for which to only sample their native clusters.
	void
	set_cdr_range_stay_in_native_cluster(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, bool const setting);
	
	
	///@brief Set to only sample with clusters of the given type for this CDR.
	void
	set_cdr_stay_in_type(CDRNameEnum const cdr, core::Size const type, bool const setting);
	
	///@brief set a range of CDRs for which to stay within a particular type.
	void
	set_cdr_range_stay_in_type(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, core::Size const type, bool const setting);
	
	
	///@brief set the minimum cdr length to sample.  Nothing shorter then this will be used during graft.
	void
	set_cdr_min_length(CDRNameEnum const cdr, core::Size length);
	
	void
	set_cdr_min_length_range(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, core::Size length);
	
	///@brief set the maximum cdr length to sample.  Nothing longer then this will be used.  Useful for H3.
	void
	set_cdr_max_length(CDRNameEnum const cdr, core::Size length);
	
	void
	set_cdr_max_length_range(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, core::Size length);
	
	///@brief Only sample cluster centers for this CDR
	void
	set_cdr_cluster_centers_only(CDRNameEnum const cdr, bool setting);
	
	void
	set_cdr_cluster_centers_only_range(CDRNameEnum const cdr_start, CDRNameEnum const cdr_end, bool setting);
	
	///@brief Leave out these PDBs from sampling.  Mainly for testing, but who knows.
	//void
	//leave_out_pdb_ids(CDRNameEnum const cdr, vector1< std::string > pdbids);
	
public:
	////////////////////////////////////////////////////////////////////////////
	// Sampling Settings
	//
	//
	
	//void
	//set_type_weight(CDRNameEnum const cdr, Size type, Real weight);
	
	//void
	//set_native_cluster_weight(CDRNameEnum const cdr, Real weight);
	
	//void
	//set_cdr_weight(CDRNameEnum const cdr, Real weight);
	
	//void
	//set_center_weight(CDRNameEnum const cdr, Real weight);

public:
	////////////////////////////////////////////////////////////////////////////////
	// Boiler Plate
	//
	//
	
	virtual string get_name() const;

	
public:
	////////////////////////////////////////////////////////////////////////////////
	// Rosetta Scripts
	//
	//
	
private:
	
	void
	read_command_line_options();
	
	///@brief Call DesignInstructionParser to determine settings for the design run.
	void
	read_instructions(std::string instruction_path);
	
	//Determine each CDRs cluster if not already calculated and set in AntibodyInfo instance.
	void
	setup_native_clusters(pose::Pose & pose);
	
	///@brief Uses instructions to Query the AntibodyDatabase and load poses.  
	void
	initialize_cdr_set();
	
	void
	check_for_top_designs(pose::Pose & pose);
	
	///@brief Grafts a single CDR. Index is the vector index of CDRSet/CDRClusterMap
	void
	graft_cdr(pose::Pose & pose, CDRNameEnum cdr, core::Size index);
	
	///@brief Fixes the pose's pdb_info after graft. Start and end are rosetta residue numbers before and after the cdr.
	void
	fix_pdb_info(pose::Pose pose, CDRNameEnum cdr, CDRClusterEnum cluster, core::Size original_start, core::Size original_pdb_end);
	
	///@brief Mutates framework residues needed to stay within a particular cluster.  Only one  (L1-11-1) is known to have framework dependencies.  For now.
	/// Will be replaced by AntibodyDesignOptimizer
	void
	mutate_framework_residues(pose::Pose pose, CDRClusterEnum cluster);
	
private:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Main Algorithms
	//
	//
	
	///@brief.  If rounds >= number of possible combinations - Try them all. Recursive function due to variable number of cdrs to design.
	void
	run_deterministic_graft_algorithm(pose::Pose & pose, vector1< CDRNameEnum > & cdrs_to_design, core::Size recurse_num);
	
	///@breif Main algorithm used by mover
	void
	run_random_graft_algorithm(pose::Pose & pose, vector1< CDRNameEnum> & cdrs_to_design);
	
	
	//void
	//run_stochastic_graft_algorithm(pose::Pose & pose, vector1< CDRNameEnum > & cdrs_to_design);
	
	
	map< CDRNameEnum, CDRClusterEnum > native_clusters_; //Native North cluster
	map< CDRNameEnum,  CDRGraftInstructions > cdr_instructions_;
	map< CDRNameEnum, SamplingWeights > sampling_instructions_;
	std::string instruction_path_;
	CDRSet cdr_set_; //Actual poses that will be grafted.
	CDRClusterMap cdr_cluster_map_; //Maps CDRSet to CDRClusterEnums to constrained relaxes
	PDBMap pdbmap_; //Maps CDRSet to PDB tags
	
	AntibodyInfoOP ab_info_;
	AnchoredGraftMoverOP graft_mover_; //May need one for each CDR?
	ScoreFunctionOP scorefxn_;
	AntibodyDesignModelerOP modeler_;
	
	protocols::moves::MonteCarloOP mc_;
	core::Size overhang_;
	core::Size graft_rounds_;
	
	core::Size num_top_designs_; //Number of top designs to keep.
	
	//Can be a struct.  Just not now.
	vector1< pose::PoseOP > top_designs_; 
	vector1< core::Real> top_scores_;
	
	core::Size total_permutations_; //Total number of possible combinations
};
}
}
}



#endif	//INCLUDED_protocols_antibody_design_AntibodyGraftDesigner.hh
