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

size_t hoa_2d_get_number_of_harmonics(size_t order);
size_t hoa_2d_get_index(size_t degree, long order);
long hoa_2d_get_azimuthal_order(size_t index);
size_t hoa_2d_get_degree(size_t index);
size_t hoa_3d_get_number_of_harmonics(size_t order);
size_t hoa_3d_get_index(size_t degree, long order);
long hoa_3d_get_azimuthal_order(size_t index);
size_t hoa_3d_get_degree(size_t index);

size_t hoa_processor_clip_order(void* obj, size_t order);
size_t hoa_processor_clip_nplanewaves(void* obj, size_t nplanewaves);

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
    t_float     f_f;
    struct _hoa_io_tilde* f_next;
} t_hoa_io_tilde;

typedef struct _hoa_process_instance
{
    t_canvas*           f_canvas;
    t_hoa_in*           f_ins;
    t_hoa_out*          f_outs;
    t_hoa_io_tilde*     f_ins_sig;
    t_hoa_io_tilde*     f_outs_sig;
} t_hoa_process_instance;

void hoa_process_instance_setup(void);

char hoa_process_instance_init(t_hoa_process_instance* x, t_canvas* parent, t_symbol* name, size_t argc, t_atom* argv);

void hoa_process_instance_show(t_hoa_process_instance* x);
void hoa_process_instance_send_bang(t_hoa_process_instance* x, size_t extra);
void hoa_process_instance_send_float(t_hoa_process_instance* x, size_t extra, float f);
void hoa_process_instance_send_symbol(t_hoa_process_instance* x, size_t extra, t_symbol* s);
void hoa_process_instance_send_list(t_hoa_process_instance* x, size_t extra, t_symbol* s, int argc, t_atom* argv);
void hoa_process_instance_send_anything(t_hoa_process_instance* x, size_t extra, t_symbol* s, int argc, t_atom* argv);

size_t hoa_process_instance_get_ninputs(t_hoa_process_instance* x);
size_t hoa_process_instance_get_noutputs(t_hoa_process_instance* x);
void hoa_process_instance_set_outlet(t_hoa_process_instance* x, size_t index, t_outlet* outlet);
char hoa_process_instance_has_inputs_sig_static(t_hoa_process_instance* x);
size_t hoa_process_instance_get_ninputs_sig_extra(t_hoa_process_instance* x);
char hoa_process_instance_has_outputs_sig_static(t_hoa_process_instance* x);
size_t hoa_process_instance_get_noutputs_sig_extra(t_hoa_process_instance* x);

void hoa_process_instance_set_inlet_sig(t_hoa_process_instance* x, size_t index, t_sample* s);
void hoa_process_instance_set_outlet_sig(t_hoa_process_instance* x, size_t index, t_sample* s);


#endif //HOA_2D_PD_INCLUDE
