#include "min_size.h"

template <typename T>
IGL_INLINE int igl::min_size(const std::vector<T> & V)
{
  int min_size = -1;
  for(
    typename std::vector<T>::const_iterator iter = V.begin();
    iter != V.end(); 
    iter++)
  {
    int size = (int)iter->size();
    // have to handle base case
    if(min_size == -1)
    {
      min_size = size;
    }else{
      min_size = (min_size < size ? min_size : size);
    }
  }
  return min_size;
}



#ifndef IGL_HEADER_ONLY
// Explicit template specialization
// generated by autoexplicit.sh
template int igl::min_size<std::vector<int, std::allocator<int> > >(std::vector<std::vector<int, std::allocator<int> >, std::allocator<std::vector<int, std::allocator<int> > > > const&);
// generated by autoexplicit.sh
template int igl::min_size<std::vector<double, std::allocator<double> > >(std::vector<std::vector<double, std::allocator<double> >, std::allocator<std::vector<double, std::allocator<double> > > > const&);
#endif