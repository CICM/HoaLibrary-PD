/*
// Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef HOA_2D_PD_INCLUDE
#define HOA_2D_PD_INCLUDE

#include <m_pd.h>

typedef void (*t_hoa_processor_perfm)(void *x, size_t sampleframes,
                                 size_t nins, t_sample **ins,
                                 size_t nouts, t_sample **outs);

typedef struct _hoa_processor
{
    t_object    f_obj;
    size_t      f_vectorsize;
    size_t      f_nins;
    t_sample**  f_inputs;
    size_t      f_nouts;
    t_sample**  f_outputs;
    t_hoa_processor_perfm f_method;
} t_hoa_processor;

size_t hoa_processor_clip_order(void* obj, size_t order);
void hoa_processor_init(void* obj, size_t nins, size_t nouts);
void hoa_processor_clear(void* obj);
void hoa_processor_prepare(void* obj, t_hoa_processor_perfm m, t_signal **sp);

#endif //HOA_2D_PD_INCLUDE