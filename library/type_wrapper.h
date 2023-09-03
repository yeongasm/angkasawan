#pragma once
#ifndef TYPE_WRAPPER_H
#define TYPE_WRAPPER_H

namespace lib
{

template <typename supplied_type>
struct type_wrapper
{
	using type = supplied_type;
};

}

#endif // !TYPE_WRAPPER_H
