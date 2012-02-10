
import rosetta

from rosetta import *


rosetta.init()
test_pose=pose_from_pdb('test/data/test_dock.pdb')
residue_1=test_pose.residue(275)
residue_2=test_pose.residue(55)
sfxn=create_score_function('standard')


#calculating atom-atom pairwise interactions and summing
#to get total energy (which should match, residue-residue energy!!!)
atr_total,rep_total,solv_total=0.0,0.0,0.0

for i in range(residue_1.natoms()):
	for j in range(residue_2.natoms()):
		atom1=residue_1.atom(i+1)
		atom2=residue_2.atom(j+1)
		atr,rep,solv=atom_atom_pair_energy(atom1,atom2,sfxn)

		atr_total+=atr
		rep_total+=rep
		solv_total+=solv

emap=EMapVector()
sfxn.eval_ci_2b(residue_1,residue_2,test_pose,emap)
print 'res-res atr score emap: '+str(emap[fa_atr])
print 'res-res atr score pairwise: '+str(atr_total)
print 'res-res rep score emap: '+str(emap[fa_rep])
print 'res-res rep score pairwise: '+str(rep_total)
print 'res-res solv score emap: '+str(emap[fa_sol])
print 'res-res solv score pairwise: '+str(solv_total)





