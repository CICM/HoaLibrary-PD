/*
// Copyright (c) 2012-2016 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
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

typedef struct _hoa_in
{
    t_object    f_obj;
    int         f_extra;
} t_hoa_in;

typedef struct _hoa_out
{
    t_object    f_obj;
    int         f_extra;
    t_outlet*   f_outlet;
} t_hoa_out;

typedef struct _hoa_in_tilde
{
    t_object    f_obj;
    int         f_extra;
    t_sample*   f_signal;
} t_hoa_in_tilde;

typedef struct _hoa_out_tilde
{
    t_object    f_obj;
    int         f_extra;
    t_sample*   f_signal;
} t_hoa_out_tilde;

#endif //HOA_2D_PD_INCLUDE
