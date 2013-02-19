// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/init.cc
/// @brief  options system initialization routines
/// @author Sergey Lyskov
///

#ifdef USEMPI
#include <mpi.h> // Must go first
#include <basic/TracerToFile.hh>
#endif

// Unit headers
#include <time.h>
#include <core/init.hh>

// Project Headers
#include <core/svn_version.hh>
#include <core/types.hh>
#include <basic/options/option.hh>
#include <utility/basic_sys_util.hh>
#include <utility/excn/Exceptions.hh>
#include <utility/io/izstream.hh>
#include <basic/Tracer.hh>
#include <basic/prof.hh>
// Classes in core that must register with factorys
#include <core/scoring/constraints/ConstraintsEnergyCreator.hh>
#include <core/scoring/disulfides/CentroidDisulfideEnergyCreator.hh>
#include <core/scoring/disulfides/DisulfideMatchingEnergyCreator.hh>
#include <core/scoring/disulfides/FullatomDisulfideEnergyCreator.hh>
#include <core/scoring/etable/EtableEnergyCreator.hh>

#include <core/scoring/hbonds/HBondEnergyCreator.hh>
#include <core/scoring/methods/EnergyMethodRegistrator.hh>
#include <core/scoring/methods/CenPairEnergyCreator.hh>
#include <core/scoring/methods/ChainbreakEnergyCreator.hh>
#include <core/scoring/methods/ContactOrderEnergyCreator.hh>
#include <core/scoring/methods/EnvEnergyCreator.hh>
#include <core/scoring/methods/EnvSmoothEnergyCreator.hh>
#include <core/scoring/methods/NMerRefEnergyCreator.hh>
#include <core/scoring/methods/NMerPSSMEnergyCreator.hh>
#include <core/scoring/methods/OmegaTetherEnergyCreator.hh>
#include <core/scoring/methods/P_AA_EnergyCreator.hh>
#include <core/scoring/methods/P_AA_pp_EnergyCreator.hh>
#include <core/scoring/methods/PackStatEnergyCreator.hh>
#include <core/scoring/methods/PairEnergyCreator.hh>
#include <core/scoring/methods/PeptideBondEnergyCreator.hh>
#include <core/scoring/methods/ProClosureEnergyCreator.hh>
#include <core/scoring/methods/BurialEnergyCreator.hh>
#include <core/scoring/methods/RG_Energy_FastCreator.hh>
#include <core/scoring/methods/SA_EnergyCreator.hh>
#include <core/scoring/methods/RMS_EnergyCreator.hh>
#include <core/scoring/methods/Rama2BOffsetEnergyCreator.hh>
#include <core/scoring/methods/RamachandranEnergy2BCreator.hh>
#include <core/scoring/methods/RamachandranEnergyCreator.hh>
#include <core/scoring/methods/ReferenceEnergyCreator.hh>
#include <core/scoring/methods/ReferenceEnergyNoncanonicalCreator.hh>
#include <core/scoring/methods/SecondaryStructureEnergyCreator.hh>
#include <core/scoring/methods/VDW_EnergyCreator.hh>
#include <core/scoring/methods/dfire/DFIRE_EnergyCreator.hh>

#include <core/pack/dunbrack/DunbrackEnergyCreator.hh>

// define this for compiling a slimmed down version of mini libraries lacking about 3/4s of the code
// this is required for compiling a less memory hungry version of mini for Bluegene etc..
//#define MINI_SLIM
#ifndef MINI_SLIM
#include <core/scoring/carbon_hbonds/CarbonHBondEnergyCreator.hh>
#include <core/scoring/custom_pair_distance/FullatomCustomPairDistanceEnergyCreator.hh>
#include <core/scoring/methods/CustomAtomPairEnergyCreator.hh>
#include <core/scoring/electron_density/FastDensEnergyCreator.hh>
#include <core/scoring/electron_density/ElecDensCenEnergyCreator.hh>
#include <core/scoring/electron_density/ElecDensAllAtomCenEnergyCreator.hh>
#include <core/scoring/electron_density/ElecDensEnergyCreator.hh>
#include <core/scoring/electron_density/PattersonCorrEnergyCreator.hh>
#include <core/scoring/electron_density_atomwise/ElecDensAtomwiseEnergyCreator.hh>
//XRW_B_T1
//#include <core/scoring/etable/CoarseEtableEnergyCreator.hh>
//XRW_E_T1
#include <core/scoring/geometric_solvation/ExactOccludedHbondSolEnergyCreator.hh>
#include <core/scoring/geometric_solvation/GeometricSolEnergyCreator.hh>
#include <core/scoring/geometric_solvation/ContextIndependentGeometricSolEnergyCreator.hh>
#include <core/scoring/geometric_solvation/OccludedHbondSolEnergyCreator.hh>
#include <core/scoring/geometric_solvation/OccludedHbondSolEnergy_onebodyCreator.hh>
#include <core/scoring/hackelec/HackElecEnergyCreator.hh>
#include <core/scoring/hackelec/HackElecEnergyAroAroCreator.hh>
#include <core/scoring/hackelec/HackElecEnergyAroAllCreator.hh>
#include <core/scoring/hackelec/RNAHackElecEnergyCreator.hh>
#include <core/scoring/hackaro/HackAroEnergyCreator.hh>
#include <core/scoring/dna/DNAChiEnergyCreator.hh>
#include <core/scoring/dna/DNATorsionEnergyCreator.hh>
#include <core/scoring/methods/DNA_BaseEnergyCreator.hh>
#include <core/scoring/methods/DirectReadoutEnergyCreator.hh>
#include <core/scoring/methods/DistanceChainbreakEnergyCreator.hh>
#include <core/scoring/methods/Fa_MbenvEnergyCreator.hh>
#include <core/scoring/methods/Fa_MbsolvEnergyCreator.hh>
#include <core/scoring/methods/GenBornEnergyCreator.hh>
#include <core/scoring/methods/HybridVDW_EnergyCreator.hh>
#include <core/scoring/methods/LK_CosThetaEnergyCreator.hh>
#include <core/scoring/methods/LK_hackCreator.hh>
#include <core/scoring/methods/LK_BallEnergyCreator.hh>
#include <core/scoring/methods/LinearChainbreakEnergyCreator.hh>
#include <core/scoring/methods/MMBondAngleEnergyCreator.hh>
#include <core/scoring/methods/MMBondLengthEnergyCreator.hh>
#include <core/scoring/methods/CartesianBondedEnergyCreator.hh>
#include <core/scoring/methods/MMTorsionEnergyCreator.hh>
#include <core/scoring/methods/MMLJEnergyIntraCreator.hh>
#include <core/scoring/methods/MMLJEnergyInterCreator.hh>
#include <core/scoring/methods/MembraneCbetaEnergyCreator.hh>
#include <core/scoring/methods/MembraneCenPairEnergyCreator.hh>
#include <core/scoring/methods/MembraneEnvEnergyCreator.hh>
#include <core/scoring/methods/MembraneEnvPenaltiesCreator.hh>
#include <core/scoring/methods/MembraneLipoCreator.hh>
#include <core/scoring/methods/MembraneEnvSmoothEnergyCreator.hh>
#include <core/scoring/methods/pHEnergyCreator.hh>
#include <core/scoring/methods/PoissonBoltzmannEnergyCreator.hh>
#include <core/scoring/methods/ChemicalShiftAnisotropyEnergyCreator.hh>
#include <core/scoring/methods/DipolarCouplingEnergyCreator.hh>
#include <core/scoring/methods/ResidualDipolarCouplingEnergyCreator.hh>
#include <core/scoring/methods/ResidualDipolarCouplingEnergy_RohlCreator.hh>
#include <core/scoring/methods/SmoothCenPairEnergyCreator.hh>
#include <core/scoring/methods/SmoothEnvEnergyCreator.hh>
#include <core/scoring/methods/CenHBEnergyCreator.hh>
#include <core/scoring/methods/SuckerEnergyCreator.hh>
#include <core/scoring/methods/GaussianOverlapEnergyCreator.hh>
#include <core/scoring/methods/YHHPlanarityEnergyCreator.hh>
#include <core/pack/interaction_graph/SurfaceEnergyCreator.hh>
#include <core/pack/interaction_graph/HPatchEnergyCreator.hh>
#include <core/scoring/methods/SymmetricLigandEnergyCreator.hh>
#include <core/scoring/methods/UnfoldedStateEnergyCreator.hh>
#include <core/scoring/methods/WaterAdductHBondEnergyCreator.hh>
#include <core/scoring/methods/WaterAdductIntraEnergyCreator.hh>
#include <core/scoring/nv/NVscoreCreator.hh>
#include <core/scoring/orbitals/OrbitalsScoreCreator.hh>
#include <core/scoring/interface/DDPscoreCreator.hh>
#include <core/scoring/packing/HolesEnergyCreator.hh>
#include <core/scoring/packing/SurfVolEnergyCreator.hh>
#include <core/scoring/packing/SurfEnergyCreator.hh>
#include <core/scoring/rna/RG_Energy_RNACreator.hh>
#include <core/scoring/rna/RNA_BulgeEnergyCreator.hh>
#include <core/scoring/rna/RNA_DataBackboneEnergyCreator.hh>
#include <core/scoring/rna/RNA_FullAtomStackingEnergyCreator.hh>
#include <core/scoring/rna/RNA_LJ_BaseEnergyCreator.hh>
#include <core/scoring/rna/RNA_PairwiseLowResolutionEnergyCreator.hh>
#include <core/scoring/rna/RNA_SugarCloseEnergyCreator.hh>
#include <core/scoring/rna/RNA_TorsionEnergyCreator.hh>
#include <core/scoring/rna/RNA_VDW_EnergyCreator.hh>
#include <core/scoring/rna/RNA_Mg_EnergyCreator.hh>
#include <core/scoring/rna/RNA_FullAtomVDW_BasePhosphateCreator.hh>
#include <core/scoring/rna/chemical_shift/RNA_ChemicalShiftEnergyCreator.hh>
#include <core/scoring/sym_e/symECreator.hh>
#include <core/scoring/saxs/FastSAXSEnergyCreator.hh>
#include <core/scoring/saxs/SAXSEnergyCreator.hh>
#include <core/scoring/saxs/SAXSEnergyCreatorFA.hh>
#include <core/scoring/saxs/SAXSEnergyCreatorCEN.hh>


// Constraint registration
#include <core/scoring/constraints/ConstraintFactory.hh>
#include <core/scoring/constraints/BasicConstraintCreators.hh>

#include <core/pack/dunbrack/DunbrackConstraintCreator.hh>
#include <core/scoring/constraints/SequenceProfileConstraintCreator.hh>

// SilentStruct registration
#include <core/io/silent/SilentStructFactory.hh>
#include <core/io/silent/BasicSilentStructCreators.hh>
#include <core/import_pose/PDBSilentStructCreator.hh>

// Sequence registration
#include <core/sequence/SequenceFactory.hh>
#include <core/sequence/BasicSequenceCreators.hh>

// for registering TaskOperations, ResLvlTaskOperations, and ResFilters
#include <core/pack/task/operation/TaskOperationRegistrator.hh>
#include <core/pack/task/operation/TaskOperationCreators.hh>
#include <core/pack/task/operation/OperateOnCertainResiduesCreator.hh>
#include <core/pack/task/operation/NoRepackDisulfidesCreator.hh>
#include <core/pack/task/operation/OptCysHGCreator.hh>
#include <core/pack/task/operation/OptHCreator.hh>
#include <core/pack/task/operation/ResLvlTaskOperationRegistrator.hh>
#include <core/pack/task/operation/ResLvlTaskOperationCreators.hh>
#include <core/pack/task/operation/ResFilterRegistrator.hh>
#include <core/pack/task/operation/ResFilterCreators.hh>
#include <core/pack/task/operation/ReplicateTaskCreator.hh>
// (end for registering TaskOperations, ResLvlTaskOperations, and ResFilters)

#endif

#ifndef WIN_PYROSETTA
	#include <platform/types.hh>
#endif

#ifdef MAC
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif


#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <cstring>

using basic::T;
using basic::Error;
using basic::Warning;

// Windows headers
#if (defined WIN32) && (!defined WIN_PYROSETTA)
#include <windows.h>
#include <wincrypt.h>
#endif

// STL headers
#include <fstream>
#include <sstream>
#include <string>

// option key includes

#include <basic/options/keys/out.OptionKeys.gen.hh>
#include <basic/options/keys/run.OptionKeys.gen.hh>
#include <basic/options/keys/corrections.OptionKeys.gen.hh>
#include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/options/keys/OptionKeys.hh>

// ResourceManager includes
#include <basic/resource_manager/ResourceLoaderRegistrator.hh>
#include <core/conformation/symmetry/SymmDataLoaderCreator.hh>
#include <core/conformation/symmetry/SymmDataOptionsCreator.hh>
#include <core/io/silent/SilentFileLoaderCreator.hh>
#include <core/io/silent/SilentFileOptionsCreator.hh>
#include <core/import_pose/PoseFromPDBLoaderCreator.hh>
#include <core/import_pose/import_pose_options_creator.hh>
#include <basic/resource_manager/ResourceOptionsRegistrator.hh>
#include <core/scoring/electron_density/ElectronDensityLoaderCreator.hh>
#include <core/scoring/electron_density/ElectronDensityOptionsCreator.hh>
#include <core/chemical/ResidueLoaderCreator.hh>
#include <core/chemical/ResidueLoaderOptionsCreator.hh>




//option key includes for deprecated pdbs
#include <basic/options/keys/LoopModel.OptionKeys.gen.hh>

#include <core/pack/task/operation/ResFilterFactory.hh>
#include <core/pack/task/operation/ResLvlTaskOperationFactory.hh>
#include <core/pack/task/operation/TaskOperationFactory.hh>
#include <core/scoring/ScoringManager.hh>
#include <utility/vector0.hh>
#include <utility/vector1.hh>


#ifdef UNICODE
typedef std::wostringstream ostringstream_t;
#else
typedef std::ostringstream ostringstream_t;
#endif

namespace core {

/// The following global varialbles force the linker to always include
/// the EnergyMethodCreator files to be included in staticly linked
/// executables.  These variables will be initialized before main()
/// begins during the "dynamic initialization" phase of loading.
/// During this time, the Registrotor classes will register their templated
/// EnergyMethods with the ScoringManager before main() begins.

using namespace scoring::methods;

static EnergyMethodRegistrator< scoring::constraints::ConstraintsEnergyCreator > ConstraintsEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::disulfides::CentroidDisulfideEnergyCreator > CentroidDisulfideEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::disulfides::DisulfideMatchingEnergyCreator > DisulfideMatchingEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::disulfides::FullatomDisulfideEnergyCreator > FullatomDisulfideEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::etable::EtableEnergyCreator > EtableEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::hbonds::HBondEnergyCreator > HBondEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ChainbreakEnergyCreator > ChainbreakEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::CenPairEnergyCreator > CenPairEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ContactOrderEnergyCreator > ContactOrderEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::EnvEnergyCreator > EnvEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::EnvSmoothEnergyCreator > EnvSmoothEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::OmegaTetherEnergyCreator > OmegaTetherEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::NMerRefEnergyCreator > NMerRefEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::NMerPSSMEnergyCreator > NMerPSSMEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::P_AA_EnergyCreator > P_AA_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::P_AA_pp_EnergyCreator > P_AA_pp_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::PackStatEnergyCreator > PackStatEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::PairEnergyCreator > PairEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::PeptideBondEnergyCreator > PeptideBondEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ProClosureEnergyCreator > ProClosureEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::BurialEnergyCreator > BurialCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::RG_Energy_FastCreator > RG_Energy_FastCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SA_EnergyCreator > SA_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::RMS_EnergyCreator > RMS_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::Rama2BOffsetEnergyCreator > Ramac2BOffsetEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::RamachandranEnergy2BCreator > RamachandranEnergy2BCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::RamachandranEnergyCreator > RamachandranEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ReferenceEnergyCreator > ReferenceEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ReferenceEnergyNoncanonicalCreator > ReferenceEnergyNoncanonicalCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SecondaryStructureEnergyCreator > SecondaryStructureEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::VDW_EnergyCreator > VDW_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::dfire::DFIRE_EnergyCreator > DFIRE_EnergyCreator_registrator;

static EnergyMethodRegistrator< pack::dunbrack::DunbrackEnergyCreator > DunbrackEnergyCreator_registrator;

// define this for compiling a slimmed down version of mini libraries lacking about 3/4s of the code
// this is required for compiling a less memory hungry version of mini for Bluegene etc..
#ifndef MINI_SLIM
static EnergyMethodRegistrator< scoring::nv::NVscoreCreator > NVscoreCreator_registrator;
static EnergyMethodRegistrator< scoring::orbitals::OrbitalsScoreCreator > OrbitalsScoreCreator_registrator;
static EnergyMethodRegistrator< scoring::interface::DDPscoreCreator > DDPscoreCreator_registrator;
static EnergyMethodRegistrator< scoring::carbon_hbonds::CarbonHBondEnergyCreator > CarbonHBondEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::custom_pair_distance::FullatomCustomPairDistanceEnergyCreator > FullatomCustomPairDistanceEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::CustomAtomPairEnergyCreator > CustomAtomPairEnergy_registrator;
static EnergyMethodRegistrator< scoring::electron_density::FastDensEnergyCreator > FastDensEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::electron_density::ElecDensCenEnergyCreator > ElecDensCenEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::electron_density::ElecDensAllAtomCenEnergyCreator > ElecDensAllAtomCenEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::electron_density::ElecDensEnergyCreator > ElecDensEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::electron_density::PattersonCorrEnergyCreator > PattersonCorrEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::electron_density_atomwise::ElecDensAtomwiseEnergyCreator > ElecDensAtomwiseEnergyCreator_registrator;
//XRW_B_T1
//static EnergyMethodRegistrator< scoring::etable::CoarseEtableEnergyCreator > CoarseEtableEnergyCreator_registrator;
//XRW_E_T1
static EnergyMethodRegistrator< scoring::geometric_solvation::ExactOccludedHbondSolEnergyCreator > ExactOccludedHbondSolEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::geometric_solvation::GeometricSolEnergyCreator > GeometricSolEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::geometric_solvation::ContextIndependentGeometricSolEnergyCreator > ContextIndependentGeometricSolEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::geometric_solvation::OccludedHbondSolEnergyCreator > OccludedHbondSolEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::geometric_solvation::OccludedHbondSolEnergy_onebodyCreator > OccludedHbondSolEnergy_onebodyCreator_registrator;
static EnergyMethodRegistrator< scoring::hackelec::HackElecEnergyCreator > HackElecEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::hackelec::RNAHackElecEnergyCreator > RNAHackElecEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::hackelec::HackElecEnergyAroAroCreator > HackElecEnergyAroAroCreator_registrator;
static EnergyMethodRegistrator< scoring::hackelec::HackElecEnergyAroAllCreator > HackElecEnergyAroAllCreator_registrator;
static EnergyMethodRegistrator< scoring::hackaro::HackAroEnergyCreator > HackAroEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::dna::DNAChiEnergyCreator > DNAChiEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::dna::DNATorsionEnergyCreator > DNATorsionEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::DNA_BaseEnergyCreator > DNA_BaseEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::DirectReadoutEnergyCreator > DirectReadoutEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::DistanceChainbreakEnergyCreator > DistanceChainbreakEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::Fa_MbenvEnergyCreator > Fa_MbenvEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::Fa_MbsolvEnergyCreator > Fa_MbsolvEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::GenBornEnergyCreator > GenBornEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::HybridVDW_EnergyCreator > HybridVDW_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::LK_CosThetaEnergyCreator > LK_CosThetaEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::LK_hackCreator > LK_hackCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::LK_BallEnergyCreator > LK_BallEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::LinearChainbreakEnergyCreator > LinearChainbreakEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MMBondAngleEnergyCreator > MMBondAngleEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::CartesianBondedEnergyCreator > CartesianBondedEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MMBondLengthEnergyCreator > MMBondLengthEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MMTorsionEnergyCreator > MMTorsionEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MMLJEnergyInterCreator > MMLJEnergyInterCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MMLJEnergyIntraCreator > MMLJEnergyIntraCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneCbetaEnergyCreator > MembraneCbetaEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneCenPairEnergyCreator > MembraneCenPairEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneEnvEnergyCreator > MembraneEnvEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneEnvPenaltiesCreator > MembraneEnvPenaltiesCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SmoothCenPairEnergyCreator > SmoothCenPairEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SmoothEnvEnergyCreator > SmoothEnvEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::CenHBEnergyCreator > CenHBEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneLipoCreator > MembraneLipoCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::MembraneEnvSmoothEnergyCreator > MembraneEnvSmoothEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::pHEnergyCreator > pHEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ChemicalShiftAnisotropyEnergyCreator > ChemicalShiftAnisotropyEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::DipolarCouplingEnergyCreator > DipolarCouplingEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ResidualDipolarCouplingEnergyCreator > ResidualDipolarCouplingEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::ResidualDipolarCouplingEnergy_RohlCreator > ResidualDipolarCouplingEnergy_RohlCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SuckerEnergyCreator > SuckerEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::GaussianOverlapEnergyCreator > GaussianOverlapEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::YHHPlanarityEnergyCreator > YYHPlanarityEnergyCreator_registrator;
static EnergyMethodRegistrator< pack::interaction_graph::SurfaceEnergyCreator > SurfaceEnergyCreator_registrator;
static EnergyMethodRegistrator< pack::interaction_graph::HPatchEnergyCreator > HPatchEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::SymmetricLigandEnergyCreator > SymmetricLigandEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::UnfoldedStateEnergyCreator > UnfoldedStateEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::WaterAdductHBondEnergyCreator > WaterAdductHBondEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::methods::WaterAdductIntraEnergyCreator > WaterAdductIntraEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::packing::HolesEnergyCreator > HolesEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::packing::SurfVolEnergyCreator > SurfVolEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::packing::SurfEnergyCreator > SurfEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RG_Energy_RNACreator > RG_Energy_RNACreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_BulgeEnergyCreator > RNA_BulgeEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_DataBackboneEnergyCreator > RNA_DataBackboneEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_FullAtomStackingEnergyCreator > RNA_FullAtomStackingEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_LJ_BaseEnergyCreator > RNA_LJ_BaseEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_PairwiseLowResolutionEnergyCreator > RNA_PairwiseLowResolutionEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_SugarCloseEnergyCreator > RNA_SugarCloseEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_TorsionEnergyCreator > RNA_TorsionEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_VDW_EnergyCreator > RNA_VDW_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_Mg_EnergyCreator > RNA_Mg_EnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::RNA_FullAtomVDW_BasePhosphateCreator > RNA_FullAtomVDW_BasePhosphateCreator_registrator;
static EnergyMethodRegistrator< scoring::rna::chemical_shift::RNA_ChemicalShiftEnergyCreator > NA_ChemicalShiftEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::sym_e::symECreator > symECreator_registrator;
static EnergyMethodRegistrator< scoring::methods::PoissonBoltzmannEnergyCreator > PoissonBoltzmannEnergyCreator_registrator;

static EnergyMethodRegistrator< scoring::saxs::FastSAXSEnergyCreator > FastSAXSEnergyCreator_registrator;
static EnergyMethodRegistrator< scoring::saxs::SAXSEnergyCreatorCEN > SAXSEnergyCreatorCEN_registrator;
static EnergyMethodRegistrator< scoring::saxs::SAXSEnergyCreatorFA > SAXSEnergyCreatorFA_registrator;
static EnergyMethodRegistrator< scoring::saxs::SAXSEnergyCreator > SAXSEnergyCreator_registrator;

/// Constraint Registrators
using namespace scoring::constraints;
static ConstraintRegistrator< core::scoring::constraints::AmbiguousConstraintCreator > AmbiguousConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::AmbiguousNMRConstraintCreator > AmbiguousNMRConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::AmbiguousNMRDistanceConstraintCreator > AmbiguousNMRDistanceConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::AngleConstraintCreator > AngleConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::AtomPairConstraintCreator > AtomPairConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::BigBinConstraintCreator > BigBinConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::CoordinateConstraintCreator > CoordinateConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::DihedralConstraintCreator > DihedralConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::DihedralPairConstraintCreator > DihedralPairConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::KofNConstraintCreator > KofNConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::LocalCoordinateConstraintCreator > LocalCoordinateConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::MultiConstraintCreator > MultiConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::FabConstraintCreator > FabConstraintCreator_registrator;

static ConstraintRegistrator< core::pack::dunbrack::DunbrackConstraintCreator > DunbrackConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::SiteConstraintCreator > SiteConstraintCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::SiteConstraintResiduesCreator > SiteConstraintResiduesCreator_registrator;
static ConstraintRegistrator< core::scoring::constraints::SequenceProfileConstraintCreator > SequenceProfileConstraintCreator_registrator;

// SilentStruct registrators
using namespace core::io::silent;
static SilentStructRegistrator< core::io::silent::ProteinSilentStructCreator > ProteinSilentStructCreator_registrator;
static SilentStructRegistrator< core::io::silent::ProteinSilentStruct_SinglePrecCreator > ProteinSilentStruct_SinglePrecCreator_registrator;
static SilentStructRegistrator< core::io::silent::RNA_SilentStructCreator > RNA_SilentStructCreator_registrator;
static SilentStructRegistrator< core::import_pose::PDBSilentStructCreator > PDBSilentStructCreator_registrator;
static SilentStructRegistrator< core::io::silent::BinaryProteinSilentStructCreator > BinaryProteinSilentStructCreator_registrator;
static SilentStructRegistrator< core::io::silent::ScoreFileSilentStructCreator > ScoreFileSilentStructCreator_registrator;
static SilentStructRegistrator< core::io::silent::BinaryRNASilentStructCreator > BinaryRNASilentStructCreator_registrator;

// Sequence registrators
using namespace core::sequence;
static SequenceRegistrator< core::sequence::SimpleSequenceCreator > SimpleSequenceCreator_registrator;
static SequenceRegistrator< core::sequence::SequenceProfileCreator > SequenceProfileCreator_registrator;
static SequenceRegistrator< core::sequence::SequenceCouplingCreator > SequenceCouplingCreator_registrator;
static SequenceRegistrator< core::sequence::CompositeSequenceCreator > CompositeSequenceCreator_registrator;

// perhaps these long registration lists live in their own separate file?
using namespace pack::task::operation;
// register TaskOperationCreators
static TaskOperationRegistrator< RestrictYSDesignCreator > RestrictYSDesignCreator_registrator;
static TaskOperationRegistrator< PreventRepackingCreator > PreventRepackingCreator_registrator;
static TaskOperationRegistrator< PreserveCBetaCreator > PreserveCBetaCreator_registrator;
static TaskOperationRegistrator< AppendRotamerSetCreator > AppendRotamerSetCreator_registrator;
static TaskOperationRegistrator< AppendRotamerCreator > AppendRotamerCreator_registrator;
static TaskOperationRegistrator< ExtraRotamersCreator > ExtraRotamersCreator_registrator;
static TaskOperationRegistrator< ExtraChiCutoffCreator > ExtraChiCutoffCreator_registrator;
static TaskOperationRegistrator< SetRotamerCouplingsCreator > SetRotamerCouplingsCreator_registrator;
static TaskOperationRegistrator< SetRotamerLinksCreator > SetRotamerLinksCreator_registrator;
static TaskOperationRegistrator< ReadResfileCreator > ReadResfileCreator_registrator;
static TaskOperationRegistrator< ReadResfileAndObeyLengthEventsCreator > ReadResfileAndObeyLengthEventsCreator_registrator;
static TaskOperationRegistrator< IncludeCurrentCreator > IncludeCurrentCreator_registrator;
static TaskOperationRegistrator< InitializeExtraRotsFromCommandlineCreator > InitializeExtraRotsFromCommandlineCreator_registrator;
static TaskOperationRegistrator< InitializeFromCommandlineCreator > InitializeFromCommandlineCreator_registrator;
static TaskOperationRegistrator< ExtraRotamersGenericCreator > ExtraRotamersGenericCreator_registrator;
static TaskOperationRegistrator< RotamerExplosionCreator > RotamerExplosionCreator_registrator;
static TaskOperationRegistrator< RestrictAbsentCanonicalAASCreator > RestrictAbsentCanonicalAASCreator_registrator;
static TaskOperationRegistrator< DisallowIfNonnativeCreator > DisallowIfNonnativeCreator_registrator;
static TaskOperationRegistrator< RestrictResidueToRepackingCreator > RestrictResidueToRepackingCreator_registrator;
static TaskOperationRegistrator< RestrictToRepackingCreator > RestrictToRepackingCreator_registrator;
static TaskOperationRegistrator< OperateOnCertainResiduesCreator > OperateOnCertainResiduesCreator_registrator;
static TaskOperationRegistrator< NoRepackDisulfidesCreator > NoRepackDisulfidesCreator_registrator;
static TaskOperationRegistrator< ReplicateTaskCreator > ReplicateTaskCreator_registrator;
static TaskOperationRegistrator< OptCysHGCreator > OptCysHGCreator_registrator;
static TaskOperationRegistrator< OptHCreator > OptHCreator_registrator;
// register ResLvlTaskOperationCreators
static ResLvlTaskOperationRegistrator< RestrictToRepackingRLTCreator > RestrictToRepackingRLTCreator_registrator;
static ResLvlTaskOperationRegistrator< RestrictAbsentCanonicalAASRLTCreator > RestrictAbsentCanonicalAASRLTCreator_registrator;
static ResLvlTaskOperationRegistrator< DisallowIfNonnativeRLTCreator > DisallowIfNonnativeRLTCreator_registrator;
static ResLvlTaskOperationRegistrator< PreventRepackingRLTCreator > PreventRepackingRLTCreator_registrator;
static ResLvlTaskOperationRegistrator< AddBehaviorRLTCreator > AddBehaviorRLTCreator_registrator;
// register ResFilterCreators
static ResFilterRegistrator< ResidueHasPropertyCreator > ResidueHasPropertyCreator_registrator;
static ResFilterRegistrator< ResidueLacksPropertyCreator > ResidueLacksPropertyCreator_registrator;
static ResFilterRegistrator< ResidueName3IsCreator > ResidueName3IsCreator_registrator;
static ResFilterRegistrator< ResidueName3IsntCreator > ResidueName3IsntCreator_registrator;
static ResFilterRegistrator< ResidueIndexIsCreator > ResidueIndexIsCreator_registrator;
static ResFilterRegistrator< ResidueIndexIsntCreator > ResidueIndexIsntCreator_registrator;
static ResFilterRegistrator< ResiduePDBIndexIsCreator > ResiduePDBIndexIsCreator_registrator;
static ResFilterRegistrator< ResiduePDBIndexIsntCreator > ResiduePDBIndexIsntCreator_registrator;
static ResFilterRegistrator< ChainIsCreator > ChainIsCreator_registrator;
static ResFilterRegistrator< ChainIsntCreator > ChainIsntCreator_registrator;
static ResFilterRegistrator< AnyResFilterCreator > AnyResFilterCreator_registrator;
static ResFilterRegistrator< AllResFilterCreator > AllResFilterCreator_registrator;
static ResFilterRegistrator< NoResFilterCreator > NoResFilterCreator_registrator;
static ResFilterRegistrator< ResidueTypeFilterCreator > ResidueTypeResFilterCreator_registrator;

using basic::resource_manager::ResourceLoaderRegistrator;
static ResourceLoaderRegistrator< core::conformation::symmetry::SymmDataLoaderCreator > SymmDataLoaderCreator_registrator;
static ResourceLoaderRegistrator< core::io::silent::SilentFileLoaderCreator > SilentFileLoaderCreator_registrator;
static ResourceLoaderRegistrator< core::import_pose::PoseFromPDBLoaderCreator > PoseFromPDBLoaderCreator_registrator;
static ResourceLoaderRegistrator< core::scoring::electron_density::ElectronDensityLoaderCreator > ElectronDensityLoaderCreator_registrator;
static ResourceLoaderRegistrator< core::chemical::ResidueLoaderCreator > ResidueLoaderCreator_registrator;

using basic::resource_manager::ResourceOptionsRegistrator;
static ResourceOptionsRegistrator< core::conformation::symmetry::SymmDataOptionsCreator > SymmDataOptionsCreator_registrator;
static ResourceOptionsRegistrator< core::import_pose::ImportPoseOptionsCreator > ImportPoseOptionsCreator_registrator;
static ResourceOptionsRegistrator< core::io::silent::SilentFileOptionsCreator > SilentFileOptionsCreator_registrator;
static ResourceOptionsRegistrator< core::scoring::electron_density::ElectronDensityOptionsCreator > ElectronDensityOptionsCreator_registrator;
static ResourceOptionsRegistrator< core::chemical::ResidueLoaderOptionsCreator > ResiudeLoaderOptionsCreator_registrator;

#endif

static basic::Tracer TR("core.init");

using namespace numeric::random;
using namespace basic::options;
using namespace basic::options::OptionKeys;

#ifdef USEMPI
void
init_mpi(int argc, char * argv []) {
	int already_initialized( 0 );
	MPI_Initialized( & already_initialized );
	if ( already_initialized == 0 ) MPI_Init(&argc, &argv);
}
#else
void
init_mpi(int, char **) {}
#endif


void
init_options(int argc, char * argv []) {
#ifdef BOINC
	std::cerr << "Initializing options.... ok " << std::endl;std::cerr.flush();
#endif

	// initialize options
	initialize().load( argc, argv, false /* no "free" cmd line args (just discarded anyway) */ );
#ifdef BOINC
	std::cerr << "Loaded options.... ok " << std::endl;std::cerr.flush();
#endif
	process();
#ifdef BOINC
	std::cerr << "Processed options.... ok " << std::endl; std::cerr.flush();
#endif

	// Set option system global
	OptionCollection::set_show_accessed_options_flag( option[ out::show_accessed_options ].value() );
}

void
init_tracers(){
	// set Tracer options
	basic::TracerOptions & TO( basic::Tracer::tracer_options() );

	if( option[ out::mute ].active() )   TO.muted = option[ out::mute ]();

	if( option[ out::unmute ].active() ) TO.unmuted = option[ out::unmute ]();
	if( option[ out::level  ].active() ) TO.level   = option[ out::level ]();
	if( option[ out::levels ].active() ) TO.levels  = option[ out::levels ]();
	if( option[ out::chname ].active() ) TO.print_channel_name = option[ out::chname ]();
	if( option[ out::chtimestamp ].active() ) TO.timestamp = option[ out::chtimestamp ]();

	// Adding Tracer::flush_all_tracers to list of exit-callbacks so all tracer output got flush out when utility_exit is used.
	utility::add_exit_callback(basic::Tracer::flush_all_tracers);
}

void
init_score_function_corrections(){

	// set default corrections
	if( option[corrections::correct]) {
		// Pair energy
		if ( ! option[ corrections::score::no_his_his_pairE ].user() ) {
			option[corrections::score::no_his_his_pairE].value( true );
		}

		// p_aa_pp
		if ( ! option[ corrections::score::p_aa_pp ].user() ) {
			option[corrections::score::p_aa_pp].value( "scoring/score_functions/P_AA_pp/P_AA_pp_08.2009" );
		}
		if ( ! option[ corrections::score::p_aa_pp_nogridshift ].user() ) {
			option[corrections::score::p_aa_pp_nogridshift].value( true );
		}

		//Ramachandran
		if ( ! option[ corrections::score::rama_not_squared ].user() ) {
			option[corrections::score::rama_not_squared].value( true );
		}
		if ( ! option[ corrections::score::rama_map ].user() ) {
			option[ corrections::score::rama_map ].value("scoring/score_functions/rama/Rama09_noEH_kernel25_it08.dat");
		}
		//rotamer library
		if ( ! option[ corrections::score::dun02_file ].user() ) {
			option[corrections::score::dun02_file].value( "rotamer/bbdep02.May.sortlib-correct.12.2010" );
		}

		//icoor
		if ( ! option[ corrections::chemical::icoor_05_2009 ].user() ) {
			option[corrections::chemical::icoor_05_2009].value( true );
		}
		if ( ! option[ score::hbond_params ].user() ) {
			option[score::hbond_params].value( "correct_params" );
		}

		if ( ! option[ corrections::score::ch_o_bond_potential ].user() ) {
			option[ corrections::score::ch_o_bond_potential ].value("scoring/score_functions/carbon_hbond/ch_o_bond_potential_near_min_yf.dat");
		}
	}
}

void
init_source_revision(){
	if( option[ run::version ]() ) {
		TR << "Mini-Rosetta version " << core::minirosetta_svn_version() << " from " << core::minirosetta_svn_url() << std::endl;
	}
}

void
init_paths(){
	if( option[ in::path::path ].user() ){
		utility::io::izstream::set_alternative_search_paths(
			option[ in::path::path ]());
	}
}

/// @detail If you deprecate a long standing flag, add a line to this function to describe what the deprecated flag has been replaced with.
/// If the user specifies one of these flags, Rosetta will utility exit with a helpful message directing the user towards the new functionality
void check_deprecated_flags(){

	utility::vector1<std::string> error_messages;

	//Add deprecated flags and corresponding helpful error messages here.  This is the only thing you need to do to deprecate a flag
	if(option[LoopModel::input_pdb].user())
		error_messages.push_back("-LoopModel:input_pdb is no longer used.  Please use -s to input pdb files.");


	if(error_messages.size() > 0)
	{
		utility::vector1<std::string>::const_iterator error_it;
		for(error_it = error_messages.begin();error_it != error_messages.end();++error_it)
		{
			TR.Fatal << "ERROR: You have specified one or more deprecated flags:" <<std::endl;
			TR.Fatal << *error_it <<std::endl;
		}
		std::exit(1);

	}

}

void
report_application_command(int argc, char * argv []){
	TR << "command:";
	for ( int i=0; i< argc; ++i ) {
		TR << ' ' <<  argv[i];
	}
	TR << std::endl;
}

void
init_random_number_generators(){
	using namespace numeric::random;

	int seed = 1111111;
	int seed_offset = 0;
	bool const_seed = false;
	bool use_time_as_seed = false;
	if( option[ run::constant_seed ].active() )  const_seed = option[ run::constant_seed ]();
	if( option[ run::jran ].active() )  seed = option[ run::jran ]();
	if( option[ run::seed_offset ].active() )  seed_offset = option[ run::seed_offset ]();
	if( option[ run::use_time_as_seed ].active() )  use_time_as_seed = option[ run::use_time_as_seed ]();

#ifdef USEMPI
	if( option[ out::mpi_tracer_to_file ].user() ){
		int mpi_rank( 0 );
		MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );

		std::stringstream outfilename;
		outfilename << option[ out::mpi_tracer_to_file ]() << "_" << mpi_rank;
		basic::otstreamOP redirect_tracer = new basic::TracerToFile( outfilename.str() );
		basic::Tracer::set_ios_hook( redirect_tracer, basic::Tracer::AllChannels, false );
		basic::Tracer::super_mute( true );
	}

#endif


	std::string random_device_name( option[ run::rng_seed_device ]() ); // typically /dev/urandom or /dev/random

	int real_seed;

	if( const_seed ) {
		real_seed = seed + seed_offset;
#ifdef USEMPI
		{ // scope
			/// Give a different RNG seed to each processor
			int mpi_rank( 0 );
			MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );
			real_seed += mpi_rank;
		}
#endif
		T("core.init") << "Constant seed mode, seed=" << seed << " seed_offset=" << seed_offset
			<< " real_seed=" << real_seed << std::endl;
	}
	else {
#if (defined WIN32) && (!defined WIN_PYROSETTA)
		bool const on_windows_platform = true;
#else
		bool const on_windows_platform = false;
#endif
		// attempt to open rng device, if failure then fallback to time
		std::ifstream random_device( random_device_name.c_str(), std::ios::in | std::ios::binary );
		if ( ( random_device.fail() && !on_windows_platform ) || use_time_as_seed ) {
			if ( !use_time_as_seed ) {
				// notify user that opening rng device has failed
				T("core.init") << "NOTICE: rng device failure, using time as seed" << std::endl;
			}
			random_device.close();

			//iwd  When using time-based seed on a cluster, seed_offset is usually from 0 to num_processes.
			//iwd  If jobs start a few seconds apart, a simple sum of seed and seed_offset can lead to collisions.
			//iwd  Thus we multiply the time by some number larger than the largest expected value of seed_offset.
			//iwd  If anyone's using this on more than 1000 machines at a time, we need to increase this value.
			//iwd  (Rosetta++ used a multiplier of 20, which helps some, but is nonetheless too small.)
			seed = time(0);
			//seed = seed%10000; // PB-- USE THIS ON OUR CLUSTER TO GET UNIQUE RUNS
			//real_seed = seed + seed_offset;
			real_seed = 1000*seed + seed_offset;

#ifdef USEMPI
			// When we use MPI and time-based seeds on a cluster, adjust the RNG seed so that it is the seed of the head node
			// (the node with an mpi rank of zero) plus the rank of the processer. This or is garentees that each node will
			// have a unique seed.

			/// get the processor rank
			int mpi_rank( 0 );
			MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );
			// set the real_seed of each processor to the real seed of the head node
			MPI_Bcast( &real_seed, 1, MPI_INT, 0, MPI_COMM_WORLD );
			// adjust the real seed based on the rank
			real_seed += mpi_rank;
#endif

			T("core.init") << "'Time' seed mode, seed=" << seed << " seed_offset=" << seed_offset
				<< " real_seed=" << real_seed << std::endl;
		} else {
			// grab seeds from device
			uint32_t unsigned_32bit_seed;

#if (defined WIN32) && (!defined WIN_PYROSETTA)
			// windows random device name
			random_device_name = "CryptGenRandom";

			// unique name for key container
			ostringstream_t key_container_name;
			key_container_name << "arzetta-" << GetCurrentProcessId() << "-" << GetCurrentThreadId();

			// init cryptographic provider, creates key container
			HCRYPTPROV hCryptProv = 0;

			if ( !CryptAcquireContext( &hCryptProv, key_container_name.str().c_str(), NULL, PROV_RSA_AES, CRYPT_NEWKEYSET ) ) {
				TR.Fatal << "FATAL: CryptAcquireContext unable to acquire cryptographic provider!" << std::endl;
				TR.Fatal << std::hex << GetLastError() << std::endl;
				std::exit( 1 );
			}

			// grab the random number seed from CryptGenRandom
			BYTE pbData[ 4 ];
			if ( !CryptGenRandom( hCryptProv, 4, pbData ) ) {
				TR.Fatal << "FATAL: Unable to obtain random number seed using CryptGenRandom!" << std::endl;
				TR.Fatal << std::hex << GetLastError() << std::endl;
				std::exit( 1 );
			}

			// store seed in 'unsigned_32bit_seed'
			unsigned_32bit_seed = pbData[ 0 ] + ( pbData[ 1 ] << 8 ) + ( pbData[ 2 ] << 16 ) + ( pbData[ 3 ] << 24 );

			// release cryptographic provider handle
			if ( !CryptReleaseContext( hCryptProv, 0 ) ) {
				TR.Fatal << "FATAL: CryptReleaseContext failed to release cryptographic provider!" << std::endl;
				TR.Fatal << std::hex << GetLastError() << std::endl;
				std::exit( 1 );
			}

			// delete key container
			if ( !CryptAcquireContext( &hCryptProv, key_container_name.str().c_str(), NULL, PROV_RSA_AES, CRYPT_DELETEKEYSET ) ) {
				TR.Fatal << "FATAL: CryptAcquireContext failed to delete key container!" << std::endl;
				TR.Fatal << std::hex << GetLastError() << std::endl;
				std::exit( 1 );
			}

#else
			random_device.read( reinterpret_cast< char * >( &unsigned_32bit_seed ), sizeof( uint32_t ));
#endif
			random_device.close();

			// calculate actual seeds
			seed = static_cast< int >( unsigned_32bit_seed );
			real_seed = seed + seed_offset;

#ifdef USEMPI
			// Although not as critical with device-based seeding as with time-based clusters, when we use MPI and
			// device-based seeds on a cluster, adjust the RNG seed so that it is the seed of the head node (the node with an
			// mpi rank of zero) plus the rank of the processer. This or is garentees that each node will have a unique
			// seed. Different OSs impliment their RNG differently and as we use increassingly large numbers of processors
			// this may become an issue (but probaly not).

			/// get the processor rank
			int mpi_rank( 0 );
			MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );
			// set the real_seed of each processor to the real seed of the head node
			MPI_Bcast( &real_seed, 1, MPI_INT, 0, MPI_COMM_WORLD );
			// adjust the real seed based on the rank
			real_seed += mpi_rank;
#endif

			// log seeds
			T("core.init") << "'RNG device' seed mode, using '" << random_device_name << "', seed=" << seed << " seed_offset=" << seed_offset
			    << " real_seed=" << real_seed << std::endl;
		}

	}

	/*numeric::random::RandomGenerator::initializeRandomGenerators(
		 real_seed, numeric::random::_RND_ConstantSeed_,
		 option[ run::rng ]  );
	 */
#ifdef BOINC
	std::cerr << "Initializing random generators... ok " << std::endl;std::cerr.flush();
#endif
	init_random_generators(real_seed, _RND_NormalRun_, option[ run::rng ]);

	srand( time(NULL) );  // seed default random generator, this will expose all code that use non approved random methods
}

/// @brief Initialize random generator systems (and send debug io to tracer with seed/mode info).
void init_random_generators(int const start_seed, RND_RunType run_type, std::string const & RGtype)
{
	if( run_type == _RND_TestRun_ ) {
		T("core.init.random") << "RandomGenerator:init: _RND_TestRun_ mode, seed=" << start_seed <<
			" RG_type=" << RGtype << std::endl;
	}
	else {
		T("core.init.random") << "RandomGenerator:init: Normal mode, seed=" << start_seed <<
			" RG_type=" << RGtype << std::endl;
	}

	RandomGenerator::initializeRandomGenerators(start_seed, run_type, RGtype);
}


void
random_delay(){
	// no silly waiting in DEBUG or BOINC builds
#ifdef NDEBUG
#ifndef BOINC
	if( !option[ run::nodelay ]() ){
		if( option[ run::delay ]() > 0 ) {
			int waittime = option[ run::delay ]();
			TR << "Delaying start of mini for " << waittime << " seconds due to -delay option" << std::endl;
			utility::sys_sleep( waittime );
		} else
		if( option[ run::random_delay ]() > 0 ) {
			int waittime = (int) ( (Real)option[ run::random_delay ]() * numeric::random::uniform() );
			TR << "Delaying of mini for " << waittime << " seconds (maximum = "
			   <<  option[ run::random_delay ]()
				 << " )" << std::endl
				 << "This prevents extreme IO levels when multiple jobs start simultaneously on" << std::endl
				 << "large computer clusters  and is default now. To prevent this add the option -nodelay" << std::endl
				 << "To change the random wait time use -run::random_delay <int> " << std::endl;
			utility::sys_sleep( waittime );
		}
	}
#endif
#endif
}

void
locate_rosetta_database(){
	if ( !option[ in::path::database ].user() ) {
		std::string database_path;
		char * descr = getenv("ROSETTA3_DB");
		if ( descr ) {
			TR << "found database environment variable ROSETTA3_DB: "<< descr << std::endl;
			database_path = std::string( descr );
		} else {

			char path[1024];
			uint32_t path_size = sizeof( path );

#ifdef MAC
			_NSGetExecutablePath(path, &path_size );
#endif
#ifdef LINUX
			readlink( "/proc/self/exe", path, path_size ); // This works on plain Linux, but not FreeBSD or Solaris.
#endif
#ifdef WIN32
			TR << "There is some way to automatically figure out the path to rosetta_database with GetModuleFileName in Windows. This is already set up for linux and mac, its probably just one line to change in core/init.cc" << std::endl;
			// I think its this -- can someone comment out and compile on Windows?
			// GetModuleFileName( NULL, path, path_size );
#endif

			std::string path_string( path );
			Size found = path_string.find("rosetta_source/"); //This better be in the path -- now part of the standard rosetta3 directory structure!
			if ( found != std::string::npos && path_size > 0){
				std::string rosetta_exe_dir = path_string.substr(0,found);
				database_path = rosetta_exe_dir + "rosetta_database/";
				TR << "Looking for database based on location of executable: " << database_path << std::endl;
			} else {
				TR << "Could not determine location of executable." << std::endl;
			}

		}

		if ( database_path.size() > 0 ){
			option[ in::path::database ].value( database_path );
		} else {
			TR << "Could not find database. Either specify -database or set environment variable ROSETTA3_DB." << std::endl;
		}

	}
}

void
init_profiling(){
	basic::prof_reset(); //reads option run::profile -- starts clock TOTAL
}

/// @brief Init basic core systems: options system, random system.
void init(int argc, char * argv [])
{
	try{
    //Initialize MPI
    init_mpi(argc, argv);

    //The options system manages command line options
    init_options(argc, argv);

    //Tracers control output to std::cout and std::cerr
    init_tracers();

    //Initialize the latest and greatest score function parameters
    init_score_function_corrections();

    //Choose to output source version control information?
    init_source_revision();

    //Setup basic search paths
    init_paths();

    //Check for deprecated flags specified by the user and output error messages if necessary
    check_deprecated_flags();

    //Describe the application execution command
    report_application_command(argc, argv);

    //Initalize random number generators
    init_random_number_generators();

    //Choose to randomly delay execution to desyncronize parallel execution
    random_delay();

    //Locate rosetta_database
    locate_rosetta_database();

#ifdef BOINC
    std::cerr << "Initialization complete. " << std::endl;
#endif

    //Profiling measures execution performance
    init_profiling();

    // help out user...
    if  ( argc == 1 )  TR << std::endl << "USEFUL TIP: Type -help to get the options for this Rosetta executable." << std::endl << std::endl;

  }
  // Catch any Rosetta exceptions
  catch( utility::excn::EXCN_Msg_Exception &e){
    // print the error message to standard error
    e.show( std::cerr );
    // and rethrow to make sure we quit (or give caller opportunity to clean up or catch)
    throw;
  }
}


/// @brief wrapper for core system Init
void init( utility::vector1<std::string> const & args )
{
	// create arguments in argc/argv format
	int argc = args.size();
	char **argv = new char*[ argc ];
	for( int ii = 0; ii < argc; ++ii ) {
		argv[ ii ] = new char[ args[ii+1].size()+1 ];
		strncpy( argv[ii], args[ii+1].c_str(), args[ii+1].size() );
		argv[ ii ][ args[ii+1].size() ] = 0; // ensure null termination
	}

	// call init
	init( argc, argv );

	// delete freestore
	for ( int ii = 0; ii < argc; ++ii ) {
		delete[] argv[ ii ];
	}
	delete[] argv;
}

} // namespace core

