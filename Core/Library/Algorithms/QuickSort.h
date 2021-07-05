#pragma once
#ifndef LEARNVK_CORE_LIBRARY_ALGORITHMS_QUICK_SORT_H
#define LEARNVK_CORE_LIBRARY_ALGORITHMS_QUICK_SORT_H

#include "Library/Templates/Templates.h"

namespace astl
{

  template <typename ContainerType, typename SortFunc>
  void QuickSort(ContainerType& Container, SortFunc Func)
  {
    for (auto& a : Container) {
      for (auto& b : Container) {
        if (Func(a, b)) {
          Swap(b, a);
        }
      }
    }
  }

}
#endif // !LEARNVK_CORE_LIBRARY_ALGORITHMS_QUICK_SORT_H
