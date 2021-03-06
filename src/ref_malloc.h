
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef REF_MALLOC_H
#define REF_MALLOC_H

#include "ref_defs.h"

BEGIN_C_DECLORATION

#define ref_malloc(ptr, n, ptr_type)                            \
  {                                                             \
    RAS((n) >= 0, "malloc " #ptr " of " #ptr_type " negative"); \
    (ptr) = (ptr_type *)malloc((n) * sizeof(ptr_type));         \
    RNS((ptr), "malloc " #ptr " of " #ptr_type " NULL");        \
  }

#define ref_malloc_init(ptr, n, ptr_type, initial_value)                      \
  {                                                                           \
    REF_INT ref_malloc_init_i;                                                \
    ref_malloc(ptr, n, ptr_type);                                             \
    for (ref_malloc_init_i = 0; ref_malloc_init_i < (n); ref_malloc_init_i++) \
      (ptr)[ref_malloc_init_i] = (initial_value);                             \
  }

/* realloc of size zero with return NULL */

#define ref_realloc(ptr, n, ptr_type)                                        \
  {                                                                          \
    if (REF_FALSE)                                                           \
      printf("%d: %s: realloc n int %d uLong %lu size_of %lu = %lu\n",       \
             __LINE__, __func__, (REF_INT)(n), (unsigned long)(n),           \
             sizeof(ptr_type), (n) * sizeof(ptr_type));                      \
    fflush(stdout);                                                          \
    if (0 < (n)) (ptr) = (ptr_type *)realloc((ptr), (n) * sizeof(ptr_type)); \
    RNB((ptr), "realloc " #ptr " NULL",                                      \
        printf("failed to realloc n int %d uLong %lu size_of %lu = %lu\n",   \
               (REF_INT)(n), (unsigned long)(n), sizeof(ptr_type),           \
               (n) * sizeof(ptr_type)));                                     \
  }

#define ref_free(ptr) \
  if (NULL != (ptr)) free((ptr));

END_C_DECLORATION

#endif /* REF_MALLOC_H */
