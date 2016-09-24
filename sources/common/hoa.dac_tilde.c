/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <stdlib.h>

#define DEFDACBLKSIZE 64
EXTERN t_sample* sys_soundout;

typedef struct _hoa_dac
{
    t_object    x_obj;
    t_int       x_n;
    t_int*      x_vec;
    t_float     x_f;
} t_hoa_dac;

static t_class *hoa_dac_class;

static void *hoa_dac_new(t_symbol *s, int argc, t_atom *argv)
{
    int i, j;
    int min, max;
    t_atom channels[512];
    t_hoa_dac *x = (t_hoa_dac *)pd_new(hoa_dac_class);
    
    if(!argc)
    {
        x->x_n = 2;
        x->x_vec = (t_int *)getbytes(2 * sizeof(*x->x_vec));
        x->x_vec[0] = 1;
        x->x_vec[1] = 2;
    }
    else
    {
        x->x_n = 0;
        for(i = 0; i < argc; i++)
        {
            if(argv[i].a_type == A_FLOAT)
            {
                SETFLOAT(channels + x->x_n, (int)atom_getfloat(argv+i));
                x->x_n++;
            }
            else if(argv[i].a_type == A_SYMBOL)
            {
                min = atoi(atom_getsymbol(argv+i)->s_name);
                if(min > 0 && min <= 512)
                {
                    if (min < 10)
                        max = atoi(atom_getsymbol(argv+i)->s_name+2);
                    else if (min < 100)
                        max = atoi(atom_getsymbol(argv+i)->s_name+3);
                    else if (min < 1000)
                        max = atoi(atom_getsymbol(argv+i)->s_name+4);
                    else
                        max = atoi(atom_getsymbol(argv+i)->s_name+5);
                    if(max > 0 && max <= 512)
                    {
                        if(max > min)
                        {
                            for(j = min; j <= max; j++)
                            {
                                SETFLOAT(channels + x->x_n, j);
                                x->x_n++;
                            }
                        }
                        else
                        {
                            for(j = min; j >= max; j--)
                            {
                                SETFLOAT(channels + x->x_n, j);
                                x->x_n++;
                            }
                        }
                    }
                }
            }
        }
        x->x_vec = (t_int *)getbytes((size_t)(x->x_n ? x->x_n : 1) * sizeof(*x->x_vec));
        for (i = 0; i < x->x_n; i++)
        {
            x->x_vec[i] = atom_getintarg(i, (int)(x->x_n), channels);
        }
    }
    
    for (i = 1; i < x->x_n; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_f = 0;
    return (x);
}

static void hoa_dac_dsp(t_hoa_dac *x, t_signal **sp)
{
    t_int i, *ip;
    t_signal **sp2;
    for (i = x->x_n, ip = x->x_vec, sp2 = sp; i--; ip++, sp2++)
    {
        int ch = (int)(*ip - 1);
        if ((*sp2)->s_n != DEFDACBLKSIZE)
            error("hoa.dac~: bad vector size");
        else if (ch >= 0 && ch < sys_get_outchannels())
            dsp_add(plus_perform, 4, sys_soundout + DEFDACBLKSIZE*ch,
                    (*sp2)->s_vec, sys_soundout + DEFDACBLKSIZE*ch, DEFDACBLKSIZE);
    }
}

static void hoa_dac_free(t_hoa_dac *x)
{
    freebytes(x->x_vec, x->x_n * sizeof(t_atom));
}

extern void setup_hoa0x2edac_tilde(void)
{
    t_class* c = class_new(gensym("hoa.dac~"), (t_newmethod)hoa_dac_new, (t_method)hoa_dac_free,
                           (size_t)sizeof(t_hoa_dac), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_dac, x_f);
        class_addmethod(c, (t_method)hoa_dac_dsp, gensym("dsp"), A_CANT, 0);
        class_sethelpsymbol((t_class *)c, gensym("helps/hoa.dac~"));
    }
    hoa_dac_class = c;
}


