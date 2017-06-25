// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   ui/task/task.h
/// @brief  Task class for ui library.
/// @author Sergey Lyskov (sergey.lyskov@jhu.edu).

#ifndef TASK_H
#define TASK_H

#include <ui/task/node.h>

namespace ui {
namespace task {

class Task : public Node
{
public:
	explicit Task(QUuid _node_id, Node *_parent);

	std::string type() const override;

};

} // namespace task
} // namespace ui


#endif // TASK_H
