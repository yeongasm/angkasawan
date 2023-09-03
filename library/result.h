#pragma once
#ifndef RESULT_H
#define RESULT_H

#include "concepts.h"

namespace lib
{

template <is_enum flag_type, typename payload_type>
struct result
{
	flag_type		status;
	payload_type	payload;
};

}

#endif // !RESULT_H
