#ifndef NALL_FOREACH_HPP
#define NALL_FOREACH_HPP

#include <nall/concept.hpp>

#undef foreach
#define foreach(iter, object) \
  for(unsigned foreach_counter = 0, foreach_limit = container_size(object), foreach_once = 0, foreach_broken = 0; foreach_counter < foreach_limit && foreach_broken == 0; foreach_counter++, foreach_once = 0) \
    for(typeof(object[0]) &iter = object[foreach_counter]; foreach_once == 0 && (foreach_broken = 1); foreach_once++, foreach_broken = 0)

#endif
