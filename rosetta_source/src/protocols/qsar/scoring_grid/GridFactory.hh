// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   src/protocols/qsar/scoring_grid/GridFactory.hh
/// @author Sam DeLuca

#ifndef INCLUDED_protocols_qsar_scoring_grid_GridFactory_HH_
#define INCLUDED_protocols_qsar_scoring_grid_GridFactory_HH_

//Unit Headers
#include <protocols/qsar/scoring_grid/GridFactory.fwd.hh>
#include <protocols/qsar/scoring_grid/GridCreator.hh>

// Utility Headers
#include <utility/factory/WidgetRegistrator.hh>
#include <utility/pointer/ReferenceCount.hh>
#include <utility/tag/Tag.fwd.hh>
#include <utility/vector1.hh>
// c++ headers
#include <map>

namespace protocols {
namespace qsar {
namespace scoring_grid {

/// @brief this templated class will register an instance of a
/// GridCreator (class T) with the GridFactory.  It will ensure that
/// no GridCreator is registered twice and centralizes the registration logic
template< class T >
class GridRegistrator : public utility::factory::WidgetRegistrator<GridFactory,T>
{
public:
	typedef utility::factory::WidgetRegistrator<GridFactory,T> parent;
public:
	GridRegistrator() : parent() {}
};

class GridFactory : public utility::pointer::ReferenceCount
{
public:

	typedef std::map<std::string, GridCreatorOP > GridMap;
	typedef utility::tag::Tag Tag;
	typedef utility::tag::TagPtr TagPtr;

public:
	virtual ~GridFactory();

	static
	GridFactory * get_instance();

	void factory_register(GridCreatorOP creator);

	///@brief create Grid given grid tag
	GridBaseOP new_grid(utility::tag::TagPtr const tag) const;

private:
	GridFactory();
	//unimplemented -- uncopyable
	GridFactory(GridFactory const & );
	GridFactory const & operator = (GridFactory const &);

private:
	static GridFactory * instance_ ;
	GridMap grid_creator_map_;

};

}
}
}


#endif /* GRIDFACTORY_HH_ */
