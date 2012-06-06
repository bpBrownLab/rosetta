// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#ifndef INCLUDED_protocols_sic_dock_xyzStripeHashPose_hh
#define INCLUDED_protocols_sic_dock_xyzStripeHashPose_hh

#include <numeric/geometry/hashing/xyzStripeHash.hh>

#include <core/pose/Pose.hh>
#include <core/chemical/AtomType.hh>
#include <core/conformation/Residue.hh>
#include <core/id/AtomID.hh>
#include <core/id/AtomID_Map.hh>

#include <platform/types.hh>

namespace protocols {
namespace sic_dock {


enum xyzStripeHashPoseMode {
	NBR,
	CB,
	BB,
	BNP,
	HVY,
	ALL
};

class xyzStripeHashPose : public numeric::geometry::hashing::xyzStripeHash<double> {
public:
	xyzStripeHashPose(double radius) : numeric::geometry::hashing::xyzStripeHash<double>(radius) {}
	xyzStripeHashPose(double radius, core::pose::Pose p, xyzStripeHashPoseMode m = BB ) : numeric::geometry::hashing::xyzStripeHash<double>(radius) {
		init_with_pose(p,m);
	}
	void init_with_pose(core::pose::Pose const & p, xyzStripeHashPoseMode m = BB) {
		utility::vector1<double> dummy;
		init_with_pose(p,dummy,m);
	}

	void init_with_pose(core::pose::Pose const & p, core::id::AtomID_Map<double> const & amap) {
		using core::id::AtomID;
		int natom = 0;
		if( amap.n_residue() != p.n_residue()) utility_exit_with_message("BAD ATOMID_MAP");
		for(int ir = 1; ir <= (int)p.n_residue(); ++ir) {
			// core::conformation::Residue const & r(p.residue(ir));
			for(int ia = 1; ia <= (int)amap.n_atom(ir); ia++) {
				if(amap[AtomID(ia,ir)] > 0)  natom++;
			}
		}
		utility::vector1<numeric::xyzVector<double> > atoms(natom);
		utility::vector1<double>                      meta (natom);
		platform::uint count = 0;
		for(int ir = 1; ir <= (int)p.n_residue(); ++ir) {
			// core::conformation::Residue const & r(p.residue(ir));
			for(int ia = 1; ia <= (int)amap.n_atom(ir); ia++) {
				if(amap[AtomID(ia,ir)] > 0) {
					atoms[++count] = p.xyz(AtomID(ia,ir));
					meta [  count] = amap[AtomID(ia,ir)];
				}
			}
		}
		init(atoms,meta);
	}

	void init_with_pose(core::pose::Pose const & p, utility::vector1<double> const & meta_in, xyzStripeHashPoseMode m = BB) {
		int natom = 0;
		for(int ir = 1; ir <= (int)p.n_residue(); ++ir) {
			core::conformation::Residue const & r(p.residue(ir));
			if( NBR==m ) natom++;
			if( CB==m ) if(r.has("CB")) natom++;
			if( BB ==m ) natom += r.has("N")+r.has("CA")+r.has("C")+r.has("O")+r.has("CB");
			if( HVY==m ) natom += r.nheavyatoms();
			if( ALL==m ) natom += r.natoms();
		}
		utility::vector1<numeric::xyzVector<double> > atoms(natom);
		utility::vector1<double>                      meta (natom);
		platform::uint count = 0;
		for(int ir = 1; ir <= (int)p.n_residue(); ++ir) {
			core::conformation::Residue const & r(p.residue(ir));
			if(NBR==m) {
				int ia = r.nbr_atom();
				core::id::AtomID const aid(ia,ir);
				atoms[++count] = p.xyz(aid);
				meta [  count] = r.atom_type(ia).lj_radius();
			} else if(CB==m) {
				if(r.has("CB")){ atoms[++count]=r.xyz("CB"); meta[count]=r.atom_type(r.atom_index("CB")).lj_radius(); }
			} else if(BB==m) {
				if(r.has( "N")){ atoms[++count]=r.xyz( "N"); meta[count]=r.atom_type(r.atom_index( "N")).lj_radius(); }
				if(r.has("CA")){ atoms[++count]=r.xyz("CA"); meta[count]=r.atom_type(r.atom_index("CA")).lj_radius(); }
				if(r.has( "C")){ atoms[++count]=r.xyz( "C"); meta[count]=r.atom_type(r.atom_index( "C")).lj_radius(); }
				if(r.has( "O")){ atoms[++count]=r.xyz( "O"); meta[count]=r.atom_type(r.atom_index( "O")).lj_radius(); }
				if(r.has("CB")){ atoms[++count]=r.xyz("CB"); meta[count]=r.atom_type(r.atom_index("CB")).lj_radius(); }
			} else if(BB==m) {
				if(r.has("CA")){ atoms[++count]=r.xyz("CA"); meta[count]=r.atom_type(r.atom_index("CA")).lj_radius(); }
				if(r.has( "C")){ atoms[++count]=r.xyz( "C"); meta[count]=r.atom_type(r.atom_index( "C")).lj_radius(); }
				if(r.has("CB")){ atoms[++count]=r.xyz("CB"); meta[count]=r.atom_type(r.atom_index("CB")).lj_radius(); }
			} else {
				int natom = (ALL==m) ? r.natoms() : r.nheavyatoms();
				for(int ia = 1; ia <= natom; ++ia) {
					core::id::AtomID const aid(ia,ir);
					atoms[++count] = p.xyz(aid);
					meta [  count] = r.atom_type(ia).lj_radius();
				}
			}
		}
	if(meta_in.size()!=0) {
		init(atoms,meta_in);
	} else {
		init(atoms,meta);
	}
	}

};


} // namespace sic_dock
} // namespace protocols

#endif
