/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Hoa2D.pd.h"

//! The ambisonic scope.
/** The scope discretize a circle by a set of point and uses a decoder to project the circular harmonics on it. This class should be used for graphical interfaces outside the digital signal processing if the number of points to discretize the circle is very large. Then you should prefer to record snapshot of the circular harmonics and to call the process method at an interval adapted to a graphical rendering.
 */
template <typename T>class Scope : public DecoderRegular<T>
{
private:
    T   m_maximum;
    T*  m_matrix;
public:
    
    //! The scope constructor.
    /**	The scope constructor allocates and initialize the member values to computes circular harmonics projection on a circle depending on a decomposition order and a circle discretization. The circle is discretized by the number of points. The order must be at least 1. The number of points and column should be at least 3 (but it's very low).
     
     @param     order            The order.
     @param     numberOfPoints   The number of points.
     */
    Scope(unsigned long order, unsigned long numberOfPoints) : DecoderRegular<T>(order, numberOfPoints)
    {
        m_matrix = new T[DecoderRegular<T>::getNumberOfChannels()];
        for(unsigned long i = 0; i < DecoderRegular<T>::getNumberOfChannels(); i++)
        {
            m_matrix[i] = 0.;
        }
        m_maximum = 0;
    }
    
    //! The Scope destructor.
    /**	The Scope destructor free the memory.
     */
    ~Scope()
    {
        delete [] m_matrix;
    }
    
    //! Retrieve the number of points.
    /**	Retrieve the number of points used to discretize the ambisonic circle.
     @return     This method returns the number of points used to discretize the circle.
     */
    inline unsigned long getNumberOfPoints() const noexcept
    {
        return DecoderRegular<T>::getNumberOfChannels();
    }
    
    //! Retrieve the value of a point of the circular harmonics projection.
    /**	Retrieve the result value of the circular harmonics projection for a given point defined by an index. The absolute of the value can be used as the radius of the point for a 2 dimentionnal representation. For the index, 0 is the 0 azimtuh of the circle. The maximum index must be the number of points - 1.
     @param     index   The point index of the point.
     @return    This method returns the value of a point of the ambisonic circle.
     */
    inline T getPointValue(const unsigned long index) const noexcept
    {
        return m_matrix[index];
    }
    
    //! Retrieve the radius of a point of the circular harmonics projection.
    /**	Retrieve the radius of the circular harmonics projection for a given point defined by an index. This the absolute of the result of the projection. For the index, 0 is the 0 azimtuh of the circle. The maximum index must be the number of points - 1.
     @param     pointIndex   The point index of the point.
     @return    This method returns the radius of a point of the ambisonic circle.
     */
    inline T getPointRadius(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]);
    }
    
    //! Retrieve the azimuth of a point of the circular harmonics projection.
    /**	Retrieve the azimuth of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     @param     pointIndex   The point index of the point.
     @return    This method returns the azimuth of a point of the ambisonic circle.
     */
    inline T getPointAzimuth(const unsigned long index) const noexcept
    {
        return DecoderRegular<T>::getChannelAzimuth(index);
    }
    
    //! Retrieve the abscissa of a point of the circular harmonics projection.
    /**	Retrieve the abscissa of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     
     @param     pointIndex   The point index of the point.
     @return    This method returns the abscissa of a point of the ambisonic circle.
     
     @see       getOrdinate
     */
    inline T getPointAbscissa(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]) * DecoderRegular<T>::getChannelAbscissa(index);
    }
    
    //! Retrieve the ordinate of a point of the circular harmonics projection.
    /**	Retrieve the ordinate of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     
     @param     pointIndex   The point index of the point.
     @return    This method returns the ordinate of a point of the ambisonic circle.
     
     @see       getAbscissa
     */
    inline T getPointOrdinate(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]) * DecoderRegular<T>::getChannelOrdinate(index);
    }
    
    //! This method performs the circular harmonics projection.
    /**	You should use this method to compute the projection of the circular harmonics over an ambisonics circle. The inputs array contains the circular harmonics samples and the minimum size must be the number of harmonics.
     @param     inputs   The inputs array.
     */
    inline void process(const T* inputs) noexcept
    {
        DecoderRegular<T>::process(inputs, m_matrix);
        m_maximum = fabsf(vector_max(DecoderRegular<T>::getNumberOfChannels(), m_matrix));
        if(m_maximum > 1.)
        {
            vector_scale(DecoderRegular<T>::getNumberOfChannels(), (1. / m_maximum), m_matrix);
        }
    }
};

typedef struct  _hoa_scope
{
	t_edspbox               f_box;
    unique_ptr<Scope<float>>f_scope;
    int                     f_index;
	t_clock*        f_clock;
	int             f_startclock;
	long            f_interval;
    float           f_gain;
	
	t_rgba          f_color_bg;
    t_rgba          f_color_bd;
	t_rgba          f_color_nh;
	t_rgba          f_color_ph;
	
	float           f_center;
	float           f_radius;
    t_float*        f_signals;
    void*           f_attrs;
} t_hoa_scope;

t_eclass *hoa_scope_class;

void *hoa_scope_new(t_symbol *s, int argc, t_atom *argv);
void hoa_scope_free(t_hoa_scope *x);
void hoa_scope_assist(t_hoa_scope *x, void *b, long m, long a, char *s);
void hoa_scope_tick(t_hoa_scope *x);

void hoa_scope_dsp(t_hoa_scope *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void hoa_scope_perform(t_hoa_scope *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);

t_pd_err hoa_scope_notify(t_hoa_scope *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
long hoa_scope_oksize(t_hoa_scope *x, t_rect *newrect);
void hoa_scope_getdrawparams(t_hoa_scope *x, t_object *patcherview, t_edrawparams *params);

void hoa_scope_paint(t_hoa_scope *x, t_object *view);
void draw_background(t_hoa_scope *x, t_object *view, t_rect *rect);
void draw_harmonics(t_hoa_scope *x,  t_object *view, t_rect *rect);

t_pd_err    get_order(t_hoa_scope *x, void *attr, long *argc, t_atom **argv);
t_pd_err    set_order(t_hoa_scope *x, t_object *attr, long argc, t_atom *argv);
t_hoa_err hoa_getinfos(t_hoa_scope* x, t_hoa_boxinfos* boxinfos);

extern "C" void setup_hoa0x2e2d0x2escope_tilde(void)
{
	t_eclass *c;

	c = eclass_new("hoa.2d.scope~", (method)hoa_scope_new, (method)hoa_scope_free, (short)sizeof(t_hoa_scope), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_scope_new, gensym("hoa.scope~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    eclass_init(c, 0);
	hoa_initclass(c, (method)hoa_getinfos);
    eclass_addmethod(c, (method)hoa_scope_dsp,			"dsp",          A_CANT, 0);
	eclass_addmethod(c, (method)hoa_scope_assist,		"assist",		A_CANT,	0);
	eclass_addmethod(c, (method)hoa_scope_paint,		"paint",		A_CANT,	0);
	eclass_addmethod(c, (method)hoa_scope_notify,		"notify",		A_CANT, 0);
	eclass_addmethod(c, (method)hoa_scope_getdrawparams,"getdrawparams", A_CANT, 0);
	eclass_addmethod(c, (method)hoa_scope_oksize,		"oksize",		A_CANT, 0);

    CLASS_ATTR_INVISIBLE            (c, "fontname", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontweight", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontslant", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontsize", 1);
    CLASS_ATTR_INVISIBLE            (c, "send", 1);
	CLASS_ATTR_DEFAULT              (c, "size", 0, "225. 225.");

    CLASS_ATTR_LONG                 (c, "order", 0, t_hoa_scope, f_attrs);
    CLASS_ATTR_ACCESSORS            (c, "order", get_order, set_order);
	CLASS_ATTR_CATEGORY             (c, "order", 0, "Ambisonic");
	CLASS_ATTR_ORDER                (c, "order", 0, "1");
	CLASS_ATTR_LABEL                (c, "order", 0, "Ambisonic Order");
	CLASS_ATTR_FILTER_MIN           (c, "order", 1);
	CLASS_ATTR_DEFAULT              (c, "order", 0, "1");
	CLASS_ATTR_SAVE                 (c, "order", 1);
    
    CLASS_ATTR_FLOAT                (c, "gain", 0, t_hoa_scope, f_gain);
	CLASS_ATTR_CATEGORY             (c, "gain", 0, "Behavior");
	CLASS_ATTR_ORDER                (c, "gain", 0, "1");
	CLASS_ATTR_LABEL                (c, "gain", 0, "Gain");
	CLASS_ATTR_FILTER_MIN           (c, "gain", 1.);
	CLASS_ATTR_DEFAULT              (c, "gain", 0, "1.");
	CLASS_ATTR_SAVE                 (c, "gain", 1);
    
	CLASS_ATTR_LONG                 (c, "interval", 0, t_hoa_scope, f_interval);
	CLASS_ATTR_CATEGORY             (c, "interval", 0, "Behavior");
	CLASS_ATTR_ORDER                (c, "interval", 0, "2");
	CLASS_ATTR_LABEL                (c, "interval", 0, "Refresh Interval in Milliseconds");
	CLASS_ATTR_FILTER_MIN           (c, "interval", 20);
	CLASS_ATTR_DEFAULT              (c, "interval", 0, "100");
	CLASS_ATTR_SAVE                 (c, "interval", 1);
	
	CLASS_ATTR_RGBA                 (c, "bgcolor", 0, t_hoa_scope, f_color_bg);
	CLASS_ATTR_CATEGORY             (c, "bgcolor", 0, "Color");
	CLASS_ATTR_STYLE                (c, "bgcolor", 0, "rgba");
	CLASS_ATTR_LABEL                (c, "bgcolor", 0, "Background Color");
	CLASS_ATTR_ORDER                (c, "bgcolor", 0, "1");
	CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "bgcolor", 0, "0.76 0.76 0.76 1.");
    
    CLASS_ATTR_RGBA                 (c, "bdcolor", 0, t_hoa_scope, f_color_bd);
	CLASS_ATTR_CATEGORY             (c, "bdcolor", 0, "Color");
	CLASS_ATTR_STYLE                (c, "bdcolor", 0, "rgba");
	CLASS_ATTR_LABEL                (c, "bdcolor", 0, "Border Color");
	CLASS_ATTR_ORDER                (c, "bdcolor", 0, "2");
	CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "bdcolor", 0, "0.7 0.7 0.7 1.");
	
	CLASS_ATTR_RGBA                 (c, "phcolor", 0, t_hoa_scope, f_color_ph);
	CLASS_ATTR_CATEGORY             (c, "phcolor", 0, "Color");
	CLASS_ATTR_STYLE                (c, "phcolor", 0, "rgba");
	CLASS_ATTR_LABEL                (c, "phcolor", 0, "Positive Harmonics Color");
	CLASS_ATTR_ORDER                (c, "phcolor", 0, "3");
	CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "phcolor", 0, "1. 0. 0. 1.");
	
	CLASS_ATTR_RGBA                 (c, "nhcolor", 0, t_hoa_scope, f_color_nh);
	CLASS_ATTR_CATEGORY             (c, "nhcolor", 0, "Color");
	CLASS_ATTR_STYLE                (c, "nhcolor", 0, "rgba");
	CLASS_ATTR_LABEL                (c, "nhcolor", 0, "Negative Harmonics Color");
	CLASS_ATTR_ORDER                (c, "nhcolor", 0, "4");
	CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "nhcolor", 0, "0. 0. 1. 1.");
    
    eclass_register(CLASS_BOX, c);
    hoa_scope_class = c;
}

void *hoa_scope_new(t_symbol *s, int argc, t_atom *argv)
{
    long flags;
	t_hoa_scope *x =  NULL;
	t_binbuf *d;
	
    d = binbuf_via_atoms(argc, argv);
	x = (t_hoa_scope *)eobj_new(hoa_scope_class);
    if(x && d)
    {
        long order = 1;
        binbuf_get_attribute_long(d, gensym("@order"), &order);
        x->f_scope      = unique_ptr<Scope<float>>(new Scope<float>(order, HOA_DISPLAY_NPOINTS));
        x->f_startclock = 0;
        
        x->f_signals    = new t_float[HOA_MAX_CHANNELS * HOA_MAX_BLOCKSIZE];
        x->f_index      = 0;
        
        eobj_dspsetup(x, x->f_scope->getNumberOfHarmonics(), 0);
        
        flags = 0
        | EBOX_IGNORELOCKCLICK
        | EBOX_GROWLINK
        ;
        ebox_new((t_ebox *)x, flags);
        
        x->f_clock = clock_new(x,(t_method)hoa_scope_tick);
        x->f_startclock = 0;
        
        ebox_attrprocess_viabinbuf(x, d);
        ebox_ready((t_ebox *)x);
        
        return (x);
    }
    
	return NULL;
}

t_hoa_err hoa_getinfos(t_hoa_scope* x, t_hoa_boxinfos* boxinfos)
{
	boxinfos->object_type = HOA_OBJECT_2D;
	boxinfos->autoconnect_inputs    = x->f_scope->getNumberOfHarmonics();
	boxinfos->autoconnect_outputs   = 0;
	boxinfos->autoconnect_inputs_type = HOA_CONNECT_TYPE_AMBISONICS;
	boxinfos->autoconnect_outputs_type = HOA_CONNECT_TYPE_STANDARD;
	return HOA_ERR_NONE;
}

void hoa_scope_dsp(t_hoa_scope *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_index = 0;
    object_method(dsp64, gensym("dsp_add"), x, (method)hoa_scope_perform, 0, NULL);
    x->f_startclock = 1;
}

void hoa_scope_perform(t_hoa_scope *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < numins; i++)
    {
        cblas_scopy(sampleframes, ins[i], 1, x->f_signals+i, numins);
    }
    cblas_sscal(numins * sampleframes, x->f_gain, x->f_signals, 1);
    x->f_index = 0;
    while(--sampleframes)
    {
        x->f_index++;
    }
    if(x->f_startclock)
	{
		x->f_startclock = 0;
		clock_delay(x->f_clock, 0);
	}
}

void hoa_scope_tick(t_hoa_scope *x)
{
    x->f_scope->process(x->f_signals + x->f_index * x->f_scope->getNumberOfHarmonics());
    
	ebox_invalidate_layer((t_ebox *)x, hoa_sym_harmonics_layer);
	ebox_redraw((t_ebox *)x);
	if(sys_getdspstate())
		clock_delay(x->f_clock, x->f_interval);
}

void hoa_scope_free(t_hoa_scope *x)
{
	ebox_free((t_ebox *)x);
    clock_free(x->f_clock);
    delete [] x->f_signals;
}

void hoa_scope_assist(t_hoa_scope *x, void *b, long m, long a, char *s)
{
    ;
}

t_pd_err hoa_scope_notify(t_hoa_scope *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
	if (msg == hoa_sym_attr_modified)
	{
		if(s == gensym("phcolor") || s == gensym("nhcolor"))
		{
			ebox_invalidate_layer((t_ebox *)x, hoa_sym_harmonics_layer);
            ebox_redraw((t_ebox *)x);
		}
	}
	return 0;
}

void hoa_scope_getdrawparams(t_hoa_scope *x, t_object *patcherview, t_edrawparams *params)
{
	params->d_boxfillcolor = x->f_color_bg;
    params->d_bordercolor = x->f_color_bd;
	params->d_borderthickness = 1;
	params->d_cornersize = 8;
}

long hoa_scope_oksize(t_hoa_scope *x, t_rect *newrect)
{
	if (newrect->width < 20)
		newrect->width = newrect->height = 20;
	return 0;
}

t_pd_err get_order(t_hoa_scope *x, void *attr, long *argc, t_atom **argv)
{
    argc[0] = 1;
    argv[0] = (t_atom *)malloc(sizeof(t_atom));
    if(argv[0] && argc[0])
    {
        atom_setfloat(argv[0], x->f_scope->getDecompositionOrder());
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

t_pd_err set_order(t_hoa_scope *x, t_object *attr, long argc, t_atom *argv)
{
    long order;
	if (argc && argv && atom_gettype(argv) == A_LONG)
    {
        order = atom_getlong(argv);
        if(order >= 1 && order != x->f_scope->getDecompositionOrder())
        {
            int dspState = canvas_suspend_dsp();
            x->f_scope      = unique_ptr<Scope<float>>(new Scope<float>(order, HOA_DISPLAY_NPOINTS));
            
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
            ebox_redraw((t_ebox *)x);
            
            eobj_resize_inputs((t_ebox *)x, x->f_scope->getNumberOfHarmonics());
            canvas_resume_dsp(dspState);
        }
	}
    
	return 0;
}

void hoa_scope_paint(t_hoa_scope *x, t_object *view)
{
	t_rect rect;
	ebox_get_rect_for_view((t_ebox *)x, &rect);
	
	x->f_center = rect.width * .5;
	x->f_radius = x->f_center * 0.95;
	
    draw_background(x, view, &rect);
    draw_harmonics(x, view, &rect);
}

void draw_background(t_hoa_scope *x, t_object *view, t_rect *rect)
{
    t_matrix transform;
    t_rgba black = rgba_addContrast(x->f_color_bg, -HOA_CONTRAST_BLACK);
    t_rgba white = rgba_addContrast(x->f_color_bg, HOA_CONTRAST_WHITE);
    
	t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_background_layer, rect->width, rect->height);
    
	if (g)
	{
		egraphics_matrix_init(&transform, 1, 0, 0, -1, x->f_center, x->f_center);
		egraphics_set_matrix(g, &transform);
        
        double angle, x1, x2, y1, y2, cosa, sina;
        for(int i = 0; i <= x->f_scope->getNumberOfHarmonics() ; i++)
		{
            angle = ((double)(i - 0.5) / (x->f_scope->getNumberOfHarmonics() + 1) * HOA_2PI);
			cosa = cos(angle);
            sina = sin(angle);
            x1 = cosa * x->f_radius * 0.2;
			y1 = sina * x->f_radius * 0.2;
            x2 = cosa * x->f_radius;
			y2 = sina * x->f_radius;
            
            egraphics_move_to(g, x1, y1);
			egraphics_line_to(g, x2, y2);
            egraphics_set_line_width(g, 3);
            egraphics_set_color_rgba(g, &white);
            egraphics_stroke(g);
            egraphics_set_color_rgba(g, &black);
			egraphics_set_line_width(g, 1);
			egraphics_stroke(g);
		}
        
        for(int i = 5; i > 0; i--)
		{
            egraphics_set_line_width(g, 3);
            egraphics_set_color_rgba(g, &white);
            egraphics_arc(g, 0, 0, (double)i * 0.2 * x->f_radius,  0., HOA_2PI);
            egraphics_stroke(g);
            egraphics_set_line_width(g, 1);
            egraphics_set_color_rgba(g, &black);
            egraphics_stroke(g);
		}
        
		ebox_end_layer((t_ebox*)x, hoa_sym_background_layer);
	}
	ebox_paint_layer((t_ebox *)x, hoa_sym_background_layer, 0., 0.);
}

void draw_harmonics(t_hoa_scope *x, t_object *view, t_rect *rect)
{
    char pathLength;
    t_matrix transform;
	t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_harmonics_layer, rect->width, rect->height);
    
	if(g)
	{
        egraphics_rotate(g, HOA_PI);
		egraphics_set_line_width(g, 1);
        egraphics_matrix_init(&transform, 1, 0, 0, -1, x->f_center, x->f_center);
        egraphics_set_matrix(g, &transform);
        
        // positive harmonics
        pathLength = 0;
        egraphics_set_color_rgba(g, &x->f_color_ph);
        for(int i = 0; i < x->f_scope->getNumberOfPoints(); i++)
        {
            if(x->f_scope->getPointValue(i) >= 0)
            {
                if(!pathLength)
                {
                    egraphics_move_to(g, x->f_scope->getPointAbscissa(i) * x->f_radius, x->f_scope->getPointOrdinate(i) * x->f_radius);
                    pathLength++;
                }
                else
                {
                    egraphics_line_to(g, x->f_scope->getPointAbscissa(i) * x->f_radius, x->f_scope->getPointOrdinate(i) * x->f_radius);
                }
            }
        }
        egraphics_close_path(g);
        if(pathLength)
            egraphics_stroke(g);
        
        // negative harmonics
        pathLength = 0;
        egraphics_set_color_rgba(g, &x->f_color_nh);
        for(int i = 0; i < x->f_scope->getNumberOfPoints(); i++)
        {
            if(x->f_scope->getPointValue(i) < 0)
            {
                if(!pathLength)
                {
                    egraphics_move_to(g, x->f_scope->getPointAbscissa(i) * x->f_radius, x->f_scope->getPointOrdinate(i) * x->f_radius);
                    pathLength++;
                }
                else
                {
                    egraphics_line_to(g, x->f_scope->getPointAbscissa(i) * x->f_radius, x->f_scope->getPointOrdinate(i) * x->f_radius);
                }
            }
        }
        egraphics_close_path(g);
        if(pathLength)
            egraphics_stroke(g);
        
		ebox_end_layer((t_ebox *)x, hoa_sym_harmonics_layer);
	}
	ebox_paint_layer((t_ebox *)x, hoa_sym_harmonics_layer, 0., 0.);
}