#pragma once
#ifndef LIB_RESULT_H
#define LIB_RESULT_H

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

#endif // !LIB_RESULT_H
