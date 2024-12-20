#ifndef FLAPJACK_MEM_H
#define FLAPJACK_MEM_H

#include <cstddef>
#include <cstdio>
#include <cstdlib>

template<class T>
T* allocate_array(std::size_t len)
{
   T* res = new T[len];
   if(res == NULL)
   {
        fprintf(stderr, "Error: Unable to allocate memory\r\n");
        exit(-1);
   }
   return res;
}

#endif
