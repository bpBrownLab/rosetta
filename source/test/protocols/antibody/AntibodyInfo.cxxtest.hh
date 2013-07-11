// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/antibody/AntibodyInfo.cxxtest.hh
/// @brief  tests for the AntibodyInfo class
/// @author Jared Adolf-Bryfogle

// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>

// Project Headers
#include <protocols/antibody/AntibodyInfo.hh>
#include <protocols/antibody/util.hh>
#include <protocols/antibody/AntibodyEnum.hh>
#include <protocols/antibody/CDRClusterEnum.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

// Protocol Headers
#include <basic/Tracer.hh>

using namespace protocols::antibody;

static basic::Tracer TR("protocols.antibody.AntibodyInfoTest");
class AntibodyInfoTest : public CxxTest::TestSuite {
	core::pose::Pose ab_pose_m_aho; //Full PDB
	core::pose::Pose ab_pose_chothia;
	AntibodyInfoOP ab_info_m_aho;
	AntibodyInfoOP ab_info_chothia;
	
public:
	
	void setUp(){
		
		core_init();
		core::import_pose::pose_from_pdb(ab_pose_m_aho, "protocols/antibody/2j88.pdb");
		core::import_pose::pose_from_pdb(ab_pose_chothia, "protocols/antibody/pdb1bln_chothia.pdb");
		ab_info_m_aho = new AntibodyInfo(ab_pose_m_aho, Modified_AHO);
		ab_info_chothia = new AntibodyInfo(ab_pose_chothia, Chothia);
		//Add another pose to test other numbering scheme.
	}
	
	void tearDown(){
		ab_pose_m_aho.clear();
	}
    
	void test_cdr_numbering(){
		
		//Please add for aroop/etc.
		
		//Starting Numbers
		//Modified Aho
		TS_ASSERT_EQUALS(24, ab_info_m_aho->get_CDR_start(l1, ab_pose_m_aho));
		TS_ASSERT_EQUALS(48, ab_info_m_aho->get_CDR_start(l2, ab_pose_m_aho));
		TS_ASSERT_EQUALS(88, ab_info_m_aho->get_CDR_start(l3, ab_pose_m_aho));
		TS_ASSERT_EQUALS(127, ab_info_m_aho->get_CDR_start(h1, ab_pose_m_aho));
		TS_ASSERT_EQUALS(156, ab_info_m_aho->get_CDR_start(h2, ab_pose_m_aho));
		TS_ASSERT_EQUALS(202, ab_info_m_aho->get_CDR_start(h3, ab_pose_m_aho));
		//Chothia
		TS_ASSERT_EQUALS(24, ab_info_chothia->get_CDR_start(l1, ab_pose_chothia));
		TS_ASSERT_EQUALS(55, ab_info_chothia->get_CDR_start(l2, ab_pose_chothia));
		TS_ASSERT_EQUALS(94, ab_info_chothia->get_CDR_start(l3, ab_pose_chothia));
		TS_ASSERT_EQUALS(140, ab_info_chothia->get_CDR_start(h1, ab_pose_chothia));
		TS_ASSERT_EQUALS(166, ab_info_chothia->get_CDR_start(h2, ab_pose_chothia));
		TS_ASSERT_EQUALS(213, ab_info_chothia->get_CDR_start(h3, ab_pose_chothia));
		
		//Ending Numbers
		//Modified Aho
		TS_ASSERT_EQUALS(34, ab_info_m_aho->get_CDR_end(l1, ab_pose_m_aho));
		TS_ASSERT_EQUALS(55, ab_info_m_aho->get_CDR_end(l2, ab_pose_m_aho));
		TS_ASSERT_EQUALS(95, ab_info_m_aho->get_CDR_end(l3, ab_pose_m_aho));
		TS_ASSERT_EQUALS(141, ab_info_m_aho->get_CDR_end(h1, ab_pose_m_aho));
		TS_ASSERT_EQUALS(164, ab_info_m_aho->get_CDR_end(h2, ab_pose_m_aho));
		TS_ASSERT_EQUALS(210, ab_info_m_aho->get_CDR_end(h3, ab_pose_m_aho));
		//Chothia
		TS_ASSERT_EQUALS(39, ab_info_chothia->get_CDR_end(l1, ab_pose_chothia));
		TS_ASSERT_EQUALS(61, ab_info_chothia->get_CDR_end(l2, ab_pose_chothia));
		TS_ASSERT_EQUALS(102, ab_info_chothia->get_CDR_end(l3, ab_pose_chothia));
		TS_ASSERT_EQUALS(146, ab_info_chothia->get_CDR_end(h1, ab_pose_chothia));
		TS_ASSERT_EQUALS(171, ab_info_chothia->get_CDR_end(h2, ab_pose_chothia));
		TS_ASSERT_EQUALS(222, ab_info_chothia->get_CDR_end(h3, ab_pose_chothia));
		
		
	}
    
	void test_info_functions(){
		TS_ASSERT_EQUALS("L1", ab_info_m_aho->get_CDR_Name(l1));
		TS_ASSERT_EQUALS("Modified_AHO", ab_info_m_aho->get_Current_AntibodyNumberingScheme());
		TS_ASSERT_EQUALS(false, ab_info_m_aho->get_pose_has_antigen());
		TS_ASSERT_EQUALS(11, ab_info_m_aho->get_CDR_length(l1));
		TS_ASSERT_EQUALS('H', ab_info_m_aho->get_CDR_chain(h3));
		TS_ASSERT(! ab_info_m_aho->get_pose_has_antigen());
		TS_ASSERT_EQUALS(24, ab_info_m_aho->get_CDR_start_PDB_num(l1));
		TS_ASSERT_EQUALS(CDRNameEnum_total, ab_info_m_aho->get_total_num_CDRs());
		TS_ASSERT_EQUALS(0, ab_info_m_aho->get_antigen_chains().size());
		
		//Test Camelid
		TS_ASSERT(! ab_info_chothia->is_camelid());
		
		//CDR Sequence + Antibody Sequence
		
	}
	void test_movemaps(){
		return;
	}
	
	void test_foldtrees(){
		return;
	}
	
	void test_loop_functions(){
		return;
	}
	
	void test_tf_functions(){
		return;
	}
	
	void test_kink_functions(){
		return;
	}
    
	
};

