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
    struct _hoa_in* f_next;
} t_hoa_in;

typedef struct _hoa_out
{
    t_object    f_obj;
    int         f_extra;
    t_outlet*   f_outlet;
    struct _hoa_out* f_next;
} t_hoa_out;

typedef struct _hoa_io_tilde
{
    t_object    f_obj;
    int         f_extra;
    t_sample*   f_signal;
    struct _hoa_io_tilde* f_next;
} t_hoa_io_tilde;





typedef struct _hoa_attr
{
    t_symbol*   name;
    size_t      size;
    t_atom*     values;
} t_hoa_attr;

typedef struct _hoa_thisprocess
{
    t_object    f_obj;
    
    t_outlet*   f_out_hoa_args;
    t_outlet*   f_out_hoa_mode;
    t_outlet*   f_out_args;
    t_outlet*   f_out_attrs;
    
    t_atom      f_hoa_args[3];
    t_atom      f_hoa_mode[2];
    
    t_atom*     f_args;
    size_t      f_argc;
    
    size_t      f_nattrs;
    t_hoa_attr* f_attrs;
    
    double      f_time;
    struct _hoa_thisprocess* f_next;
} t_hoa_thisprocess;

#endif //HOA_2D_PD_INCLUDE
