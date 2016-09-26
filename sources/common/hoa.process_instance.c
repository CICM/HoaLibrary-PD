/*
// Copyright (c) 2012-2016 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.pd.h"
#include <string.h>
#include <g_canvas.h>
#include <m_imp.h>

static t_symbol* hoa_sym_canvas;
static t_symbol* hoa_sym_obj;

static t_symbol* hoa_sym_hoa_thisprocess;
static t_symbol* hoa_sym_hoa_in;
static t_symbol* hoa_sym_hoa_out;
static t_symbol* hoa_sym_hoa_in_tilde;
static t_symbol* hoa_sym_hoa_out_tilde;

static t_symbol* hoa_sym_harmonics;
static t_symbol* hoa_sym_planewaves;

void hoa_process_instance_setup(void)
{
    hoa_sym_canvas          = gensym("canvas");
    hoa_sym_obj             = gensym("obj");
    
    hoa_sym_hoa_thisprocess = gensym("hoa.thisprocess~");
    hoa_sym_hoa_in          = gensym("hoa.in");
    hoa_sym_hoa_out         = gensym("hoa.out");
    hoa_sym_hoa_in_tilde    = gensym("hoa.in~");
    hoa_sym_hoa_out_tilde   = gensym("hoa.out~");
    
    hoa_sym_harmonics       = gensym("harmonics");
    hoa_sym_planewaves      = gensym("planewaves");
}

static void hoa_process_instance_get_hoas(t_hoa_process_instance* x, t_canvas* cnv)
{
    t_gobj *y;
    t_symbol const* name;
    for(y = cnv->gl_list; y; y = y->g_next)
    {
        name = y->g_pd->c_name;
        if(name == hoa_sym_canvas)
        {
            hoa_process_instance_get_hoas(x, (t_canvas *)y);
        }
        else if(name == hoa_sym_hoa_in)
        {
            t_hoa_in* inlet = (t_hoa_in *)y;
            inlet->f_next = x->f_ins;
            x->f_ins = inlet;
        }
        else if(name == hoa_sym_hoa_out)
        {
            t_hoa_out* outlet = (t_hoa_out *)y;
            outlet->f_next = x->f_outs;
            x->f_outs = outlet;
        }
        else if(name == hoa_sym_hoa_in_tilde)
        {
            t_hoa_io_tilde* inlet_sig = (t_hoa_io_tilde *)y;
            inlet_sig->f_next = x->f_ins_sig;
            x->f_ins_sig = inlet_sig;
        }
        else if(name == hoa_sym_hoa_out_tilde)
        {
            t_hoa_io_tilde* outlet_sig = (t_hoa_io_tilde *)y;
            outlet_sig->f_next = x->f_outs_sig;
            x->f_outs_sig = outlet_sig;
        }
    }
}

char hoa_process_instance_init(t_hoa_process_instance* x, t_canvas* parent, t_symbol* name, size_t argc, t_atom* argv)
{
    t_gobj* z;
    t_atom* av;
    x->f_canvas = NULL;
    av = (t_atom *)getbytes((argc + 3) * sizeof(t_atom));
    if(av)
    {
        SETFLOAT(av, 10);
        SETFLOAT(av+1, 10);
        SETSYMBOL(av+2, name);
        memcpy(av+3, argv, (size_t)argc * sizeof(t_atom));
        pd_typedmess((t_pd *)parent, hoa_sym_obj, (int)(argc + 3), av);
        for(z = parent->gl_list; z->g_next; z = z->g_next) {
        }
        if(z && z->g_pd->c_name == hoa_sym_canvas)
        {
            x->f_canvas = (t_canvas *)z;
            canvas_loadbang(x->f_canvas);
            
            hoa_process_instance_get_hoas(x, x->f_canvas);
            freebytes(av, (argc + 3) * sizeof(t_atom));
            return 1;
        }
        freebytes(av, (argc + 3) * sizeof(t_atom));
    }
    
    return 0;
}






void hoa_process_instance_show(t_hoa_process_instance* x)
{
    if(x->f_canvas)
    {
        canvas_vis(x->f_canvas, 1);
    }
}

void hoa_process_instance_send_bang(t_hoa_process_instance* x, size_t extra)
{
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra == extra)
        {
            pd_bang((t_pd *)in);
        }
        in = in->f_next;
    }
}

void hoa_process_instance_send_float(t_hoa_process_instance* x, size_t extra, float f)
{
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra == extra)
        {
            pd_float((t_pd *)in, f);
        }
        in = in->f_next;
    }
}

void hoa_process_instance_send_symbol(t_hoa_process_instance* x, size_t extra, t_symbol* s)
{
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra == extra)
        {
            pd_symbol((t_pd *)in, s);
        }
        in = in->f_next;
    }
}

void hoa_process_instance_send_list(t_hoa_process_instance* x, size_t extra, t_symbol* s, int argc, t_atom* argv)
{
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra == extra)
        {
            pd_list((t_pd *)in, s, argc, argv);
        }
        in = in->f_next;
    }
}

void hoa_process_instance_send_anything(t_hoa_process_instance* x, size_t extra, t_symbol* s, int argc, t_atom* argv)
{
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra == extra)
        {
            pd_typedmess((t_pd *)in, s, argc, argv);
        }
        in = in->f_next;
    }
}

size_t hoa_process_instance_get_ninputs(t_hoa_process_instance* x)
{
    size_t index = 0;
    t_hoa_in* in = x->f_ins;
    while(in != NULL)
    {
        if(in->f_extra > index)
        {
            index = in->f_extra;
        }
        in = in->f_next;
    }
    return index;
}

size_t hoa_process_instance_get_noutputs(t_hoa_process_instance* x)
{
    size_t index = 0;
    t_hoa_out* out = x->f_outs;
    while(out != NULL)
    {
        if(out->f_extra > index)
        {
            index = out->f_extra;
        }
        out = out->f_next;
    }
    return index;
}

void hoa_process_instance_set_outlet(t_hoa_process_instance* x, size_t index, t_outlet* outlet)
{
    t_hoa_out* out = x->f_outs;
    while(out != NULL)
    {
        if(out->f_extra == index)
        {
            out->f_outlet = outlet;
        }
        out = out->f_next;
    }
}

char hoa_process_instance_has_inputs_sig_static(t_hoa_process_instance* x)
{
    t_hoa_io_tilde* in = x->f_ins_sig;
    while(in != NULL)
    {
        if(in->f_extra == 0)
        {
            return 1;
        }
        in = in->f_next;
    }
    return 0;
}

size_t hoa_process_instance_get_ninputs_sig_extra(t_hoa_process_instance* x)
{
    size_t index = 0;
    t_hoa_io_tilde* in = x->f_ins_sig;
    while(in != NULL)
    {
        if(in->f_extra > index)
        {
            index = in->f_extra;
        }
        in = in->f_next;
    }
    return index;
}

char hoa_process_instance_has_outputs_sig_static(t_hoa_process_instance* x)
{
    t_hoa_io_tilde* in = x->f_outs_sig;
    while(in != NULL)
    {
        if(in->f_extra == 0)
        {
            return 1;
        }
        in = in->f_next;
    }
    return 0;
}

size_t hoa_process_instance_get_noutputs_sig_extra(t_hoa_process_instance* x)
{
    size_t index = 0;
    t_hoa_io_tilde* in = x->f_outs_sig;
    while(in != NULL)
    {
        if(in->f_extra > index)
        {
            index = in->f_extra;
        }
        in = in->f_next;
    }
    return index;
}

void hoa_process_instance_set_inlet_sig(t_hoa_process_instance* x, size_t index, t_sample* s)
{
    t_hoa_io_tilde* in = x->f_ins_sig;
    while(in != NULL)
    {
        if(in->f_extra == index)
        {
            in->f_signal = s;
        }
        in = in->f_next;
    }
}

void hoa_process_instance_set_outlet_sig(t_hoa_process_instance* x, size_t index, t_sample* s)
{
    t_hoa_io_tilde* out = x->f_outs_sig;
    while(out != NULL)
    {
        if(out->f_extra == index)
        {
            out->f_signal = s;
        }
        out = out->f_next;
    }
}









