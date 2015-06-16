/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

#define DEFDACBLKSIZE 64
EXTERN t_sample *sys_soundout;

typedef struct _hoa_pi
{	
	t_eobj p_ob;
	double p_value;
    t_outlet *p_outlet;
} t_hoa_pi;

t_eclass *pi_class;

typedef struct _hoa_pi_tilde
{
    t_edspobj   p_ob;
    double      p_value;
    double      p_phase;
} t_hoa_pi_tilde;

static t_eclass *hoa_pi_tilde_class;

typedef struct _hoa_dac
{
    t_object    x_obj;
    t_int       x_n;
    t_int*      x_vec;
    t_float     x_f;
} t_hoa_dac;

t_class *hoa_dac_class;

typedef struct _hoa_connect
{
    t_eobj      f_obj;
} t_hoa_connect;

static t_eclass *hoa_connect_class;

static void *pi_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_pi *x = (t_hoa_pi *)eobj_new(pi_class);
	x->p_value = 1.;
    x->p_value = atom_getfloat(argv);
    x->p_outlet = floatout(x);

	return(x);
}

static void pi_bang(t_hoa_pi *x)
{
    outlet_float(x->p_outlet, (float)HOA_PI * x->p_value);
}

static void pi_float(t_hoa_pi *x, float n)
{
	x->p_value = n;
	pi_bang(x);
}

extern "C" void setup_hoa0x2epi(void)
{
    t_eclass* c;
    c = eclass_new("hoa.pi", (method)pi_new,(method)NULL, sizeof(t_hoa_pi), 0L, A_GIMME, 0);
    
    
    eclass_addmethod(c, (method)pi_bang,     "bang",      A_CANT, 0);
    eclass_addmethod(c, (method)pi_float,    "float",      A_FLOAT, 0);
    
    eclass_register(CLASS_OBJ, c);
    pi_class = c;
}

static void *hoa_pi_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_pi_tilde *x = (t_hoa_pi_tilde *)eobj_new(hoa_pi_tilde_class);
    x->p_value = 1.;
    x->p_phase = 1;
    x->p_value = atom_getfloat(argv);
    eobj_dspsetup(x, 2, 1);
    
    return(x);
}

static void hoa_pi_tilde_float(t_hoa_pi_tilde *x, float n)
{
    if(eobj_getproxy(x))
    {
        x->p_phase = n;
    }
    else
    {
        x->p_value = n;
        x->p_phase = 1;
    }
}

static void hoa_pi_tilde_perform(t_hoa_pi_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < sampleframes; i++)
    {
        x->p_value = ins[0][i];
        outs[0][i] = HOA_PI * ins[0][i];
    }
}

static void hoa_pi_tilde_perform_phase(t_hoa_pi_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < sampleframes; i++)
        outs[0][i] = HOA_PI * x->p_value * ins[1][i];
}

static void hoa_pi_tilde_perform_offset(t_hoa_pi_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < sampleframes; i++)
        outs[0][i] = HOA_PI * x->p_value * x->p_phase;
}

static void hoa_pi_tilde_dsp(t_hoa_pi_tilde *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(count[0])
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_pi_tilde_perform, 0, NULL);
    else if(count[1])
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_pi_tilde_perform_phase, 0, NULL);
    else
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_pi_tilde_perform_offset, 0, NULL);
    
}

extern "C" void setup_hoa0x2epi_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.pi~", (method)hoa_pi_tilde_new, (method)NULL, sizeof(t_hoa_pi_tilde), CLASS_NOINLET, A_GIMME, 0);
    
    eclass_dspinit(c);
    
    
    eclass_addmethod(c, (method)hoa_pi_tilde_float,    "float",    A_FLOAT, 0);
    eclass_addmethod(c, (method)hoa_pi_tilde_dsp,      "dsp",      A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_pi_tilde_class = c;
}

static void *hoa_dac_new(t_symbol *s, int argc, t_atom *argv)
{
    int i, j;
    int min, max;
    t_atom channels[512];
    t_hoa_dac *x = (t_hoa_dac *)pd_new(hoa_dac_class);
    
    if(!argc)
    {
        x->x_n = 2;
        x->x_vec = (t_int *)malloc(2 * sizeof(*x->x_vec));
        x->x_vec[0] = 1;
        x->x_vec[1] = 2;
    }
    else
    {
        x->x_n = 0;
        for(i = 0; i < argc; i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
            {
                atom_setfloat(channels + x->x_n, (int)atom_getfloat(argv+i));
                x->x_n++;
            }
            else if(atom_gettype(argv+i) == A_SYM)
            {
                min = atoi(atom_getsym(argv+i)->s_name);
                if(min > 0 && min <= 512)
                {
                    if (min < 10)
                        max = atoi(atom_getsym(argv+i)->s_name+2);
                    else if (min < 100)
                        max = atoi(atom_getsym(argv+i)->s_name+3);
                    else if (min < 1000)
                        max = atoi(atom_getsym(argv+i)->s_name+4);
                    else
                        max = atoi(atom_getsym(argv+i)->s_name+5);
                    if(max > 0 && max <= 512)
                    {
                        if(max > min)
                        {
                            for(j = min; j <= max; j++)
                            {
                                atom_setlong(channels + x->x_n, j);
                                x->x_n++;
                            }
                        }
                        else
                        {
                            for(j = min; j >= max; j--)
                            {
                                atom_setlong(channels + x->x_n, j);
                                x->x_n++;
                            }
                        }
                    }
                }
            }
        }
        x->x_vec = (t_int *)malloc(size_t(x->x_n ? x->x_n : 1) * sizeof(*x->x_vec));
        for (i = 0; i < x->x_n; i++)
        {
            x->x_vec[i] = atom_getintarg(i, int(x->x_n), channels);
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
        int ch = int(*ip - 1);
        if ((*sp2)->s_n != DEFDACBLKSIZE)
            error("hoa.dac~: bad vector size");
        else if (ch >= 0 && ch < sys_get_outchannels())
            dsp_add(plus_perform, 4, sys_soundout + DEFDACBLKSIZE*ch,
                    (*sp2)->s_vec, sys_soundout + DEFDACBLKSIZE*ch, DEFDACBLKSIZE);
    }
}

static void hoa_dac_free(t_hoa_dac *x)
{
    free(x->x_vec);
}

extern "C" void setup_hoa0x2edac_tilde(void)
{
    t_class* c;
    c = class_new(gensym("hoa.dac~"), (t_newmethod)hoa_dac_new, (t_method)hoa_dac_free, (short)sizeof(t_hoa_dac), 0, A_GIMME, 0);
    
    class_sethelpsymbol((t_class *)c, gensym("helps/hoa.dac~"));
    CLASS_MAINSIGNALIN(c, t_hoa_dac, x_f);
    class_addmethod(c, (t_method)hoa_dac_dsp, gensym("dsp"), A_CANT, 0);
    hoa_dac_class = c;
}

static void *hoa_connect_new(t_symbol *s, int argc, t_atom *argv)
{
    return (t_hoa_connect *)eobj_new(hoa_connect_class);
}

static void hoa_connect_bang(t_hoa_connect *x)
{
    int i, j, index = 0;
    int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    int nx1, nx2, ny1, ny2;
    t_gobj *y = NULL;
    t_gobj *list[512];
    t_outconnect *oc;
    t_glist *cnv;
    t_symbol* name;
    for(i = 0; i < 512; i++)
    {
        list[i] = NULL;
    }
    for(y = eobj_getcanvas(x)->gl_list; y; y = y->g_next)
    {
        if(strncmp(eobj_getclassname(y)->s_name, "hoa.", 4) == 0 && glist_isselected(eobj_getcanvas(x), y))
        {
            gobj_getrect(y, eobj_getcanvas(x), &nx1, &ny1, &nx2, &ny2);
            for(i = 0; i < index; i++)
            {
                gobj_getrect(list[i], eobj_getcanvas(x), &x1, &y1, &x2, &y2);
                if(ny1 < y1 || (ny1 == y1 && nx1 < x1))
                {
                    for(j = index; j > i; j--)
                    {
                        list[j] = list[j-1];
                    }
                    list[i] = y;
                    index = pd_clip_max(index+1, 255);
                    break;
                }
            }
            if(i == index)
            {
                list[index] = y;
                index = pd_clip_max(index+1, 255);
            }
        }
        else if(eobj_getclassname(y) == gensym("canvas"))
        {
            cnv = (t_glist *)y;
            if(cnv->gl_obj.te_binbuf && binbuf_getnatom(cnv->gl_obj.te_binbuf) && binbuf_getvec(cnv->gl_obj.te_binbuf))
            {
                if(atom_gettype(binbuf_getvec(cnv->gl_obj.te_binbuf)) == A_SYM)
                    name = atom_getsym(binbuf_getvec(cnv->gl_obj.te_binbuf));
                else
                    name  = NULL;
                
                if(name && strncmp(name->s_name, "hoa.", 4) == 0 && glist_isselected(eobj_getcanvas(x), y))
                {
                    gobj_getrect(y, eobj_getcanvas(x), &nx1, &ny1, &nx2, &ny2);
                    for(i = 0; i < index; i++)
                    {
                        gobj_getrect(list[i], eobj_getcanvas(x), &x1, &y1, &x2, &y2);
                        if(ny1 < y1 || (ny1 == y1 && nx1 < x1))
                        {
                            for(j = index; j > i; j--)
                            {
                                list[j] = list[j-1];
                            }
                            list[i] = y;
                            index = pd_clip_max(index+1, 255);
                            break;
                        }
                    }
                    if(i == index)
                    {
                        list[index] = y;
                        index = pd_clip_max(index+1, 255);
                    }
                }
                
            }
        }
    }
    for(i = 0; i < index-1; i++)
    {
        for(j = 0; j < obj_noutlets((t_object *)list[i]) && j < obj_ninlets((t_object *)list[i+1]); j++)
        {
            obj_disconnect((t_object *)list[i], j, (t_object *)list[i+1], j);
            oc = obj_connect((t_object *)list[i], j, (t_object *)list[i+1], j);
            if(glist_isvisible(eobj_getcanvas(x)))
            {
                sys_vgui((char *)".x%lx.c create line %d %d %d %d -width %d -fill black -tags [list l%lx cord]\n",
                         glist_getcanvas(eobj_getcanvas(x)), 0, 0, 0, 0,
                         (obj_issignaloutlet((t_object *)list[i], j) ? 2 : 1),
                         oc);
            }
            
        }
        canvas_fixlinesfor(eobj_getcanvas(x), (t_text *)list[i]);
    }
    canvas_dirty(eobj_getcanvas(x), 1);
}

static void hoa_connect_free(t_hoa_connect *x)
{
    eobj_free(x);
}

extern "C" void setup_hoa0x2econnect(void)
{
    t_eclass* c;
    
    c = eclass_new("hoa.connect", (method)hoa_connect_new, (method)hoa_connect_free, (short)sizeof(t_hoa_connect), 0, A_GIMME, 0);
    
    
    eclass_addmethod(c, (method)hoa_connect_bang,          "bang",             A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_connect_class = c;
}

