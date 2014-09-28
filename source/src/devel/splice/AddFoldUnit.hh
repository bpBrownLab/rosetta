// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file AddFoldUnitMover.hh

#ifndef INCLUDED_devel_splice_AddFoldUnitMover_hh
#define INCLUDED_devel_splice_AddFoldUnitMover_hh

#include <devel/splice/AddFoldUnit.fwd.hh>
#include <protocols/moves/Mover.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <core/pose/Pose.fwd.hh>
#include <devel/splice/Splice.hh>

// C++ Headers
namespace devel {
namespace splice {

///@brief an object to manage the torsion/sequence data and to perform individual moves, such as introducing a new fragment
class FoldUnitUtils : public utility::pointer::ReferenceCount {
	public:
		FoldUnitUtils() : fragment_dbase_( "" ), pair_dbase_( "" ), overlap_length_( 10 ) {};
		~FoldUnitUtils();
		bool read_dbase(); //return whether read is successful
		void fragment_dbase( std::string const dbase ){ fragment_dbase_ = dbase; }
		std::string fragment_dbase() const{ return fragment_dbase_; }
		void pair_dbase( std::string const dbase ){ pair_dbase_ = dbase; }
		std::string pair_dbase() const{ return pair_dbase_; }

//		utility::vector1< ResidueBBDofs > & bbdofs(){ return bbdofs_; }// useful to generate bbdofs subsets
		utility::vector1< ResidueBBDofs > const & bbdofs() const { return bbdofs_; }// useful to generate bbdofs subsets
		utility::vector1< core::Size > & overlap_length() { return overlap_length_; }
		utility::vector1< core::Size > & overlap_rmsd() { return overlap_rmsd_; }
		bool fragment_compatibility_check( Size const i, Size const j, utility::vector1< std::pair< core::Size, core::Size > >::const_iterator it, core::Real const max_rmsd = 10.0 ) const;
	private:

/// methods below add or replace fragments without testing that the fragments are compatible, so they shouldn't be public
		void add_fragment_to_pose( core::pose::Pose & pose, PoseFragmentInfo pose_fragment_info, core::Size const entry, bool c_term/*or n-term*/ ) const;
		void replace_fragment_in_pose( core::pose::Pose & pose, PoseFragmentInfo pose_fragment_info, core::Size const entry, core::Size const fragment_num );

		std::string fragment_dbase_, pair_dbase_;
		utility::vector1< ResidueBBDofs > bbdofs_;// from fragment database
		utility::vector1< std::pair< core::Size/*i*/, core::Size/*j*/ > > entry_pairs_; // is this connection valid? i->j
		utility::vector1< core::Size > overlap_length_; //what is the overlap between i,j fragments?
		utility::vector1< core::Size > overlap_rmsd_; // what's the rmsd over this overlap (rmsd is computed in degrees over dihedral angles in MatLab
};

///@brief manage pose-fragmnet information.
class PoseFragmentInfo : public utility::pointer::ReferenceCount{
	public:
		PoseFragmentInfo(){};
		~PoseFragmentInfo();
		void load_fragment_info_from_pose( core::pose::Pose const & pose ); // set the object according to comments in the pose
		void set_fragment_info_in_pose( core::pose::Pose & pose ) const; // set the pose comments according to the object
		void fragment_start_end( FoldUnitUtils & fuu, core::Size const fragment, core::Size & start, core::Size & end ); // provide start/end residues for fragment #fragment
		core::Size operator[]( core::Size const s );
		core::Size size() const{ return fragment_map_.size(); }
	private:
		std::map< core::Size/*pose segment #*/, core::Size/*fragment_dbase entry*/ > fragment_map_;
};

class AddFoldUnitMover : public protocols::moves::Mover {
public:
	AddFoldUnitMover();
  ~AddFoldUnitMover();

	virtual void apply( core::pose::Pose & pose );
	virtual std::string get_name() const;

	virtual protocols::moves::MoverOP clone() const;
	virtual protocols::moves::MoverOP fresh_instance() const;

	virtual void parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );

	std::string fragment_dbase() const{ return fragment_dbase_; }
	void fragment_dbase( std::string const s ){ fragment_dbase_ = s; }
	core::Size max_length() const{ return max_length_; }
	void max_length( core::Size const s ){ max_length_ = s; }
	bool add_c() const{ return add_c_; }
	void add_c( bool const b ){ add_c_ = b; }
	FoldUnitUtilsOP fold_unit_utils() const{ return fold_unit_utils_; }
	void fold_unit_utils( FoldUnitUtilsOP f ){ fold_unit_utils_ = f; }

private:
		std::string fragment_dbase_;
		core::Size max_length_; //dflt 40 ; largest fragment to insert
		bool add_c_; //dflt true; add to C-terminus or N-terminus?
		FoldUnitUtilsOP fold_unit_utils_;
};

/// @brief silly mover that nukes out all information from the pose. RosettaScripts can't start without a pose...
class StartFreshMover : public protocols::moves::Mover {
public:
	StartFreshMover();
  ~StartFreshMover();

	virtual void apply( core::pose::Pose & pose );
	virtual std::string get_name() const;

	virtual protocols::moves::MoverOP clone() const;
	virtual protocols::moves::MoverOP fresh_instance() const;

	virtual void parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );

	std::string residue_type_set() const{ return residue_type_set_; }
	void residue_type_set( std::string const s ){ residue_type_set_ = s; }
private:
	std::string residue_type_set_; // dflt "CENTROID"; centroid or fullatom?
};
} // simple_moves
} // protocols

#endif