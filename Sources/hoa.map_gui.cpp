/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

#define MAX_ZOOM 1.
#define MIN_ZOOM 0.01

#define ODD_BINDING_SUFFIX "map1572"

#define atom_isNumber(av) (atom_gettype(av) == A_FLOAT)

typedef enum _BindingMapMsgFlag {
	BMAP_REDRAW		= 0x01,
	BMAP_NOTIFY		= 0x02,
	BMAP_OUTPUT		= 0x04
} BindingMapMsgFlag;

typedef struct _linkmap t_linkmap;

static inline double hoa_pd_distance(double x1, double y1, double x2, double y2, double z1 = 0., double z2 = 0.)
{
    return sqrt((x1-x2) * (x1-x2) + (y1-y2) * (y1-y2) + (z1-z2) * (z1-z2));
}

typedef struct  _hoa_map
{
	t_ebox           j_box;
	t_rect           rect;

	t_outlet*		 f_out_sources;
    t_outlet*		 f_out_groups;
    t_outlet*		 f_out_infos;

	Source::Manager* f_manager;
	Source::Manager* f_self_manager;

	Source*          f_selected_source;
	Source::Group*   f_selected_group;

    t_pt             f_cursor_position;

    t_rgba           f_color_bg;
    t_rgba           f_color_bd;

	double		     f_zoom_factor;

    t_rect		     f_rect_selection;
	int			     f_rect_selection_exist;

    t_symbol*        f_output_mode;
    ulong            f_read;
    ulong            f_write;

	int			     f_mouse_was_dragging;

	t_symbol*	     f_coord_view;

	t_symbol*	     f_binding_name;
	t_linkmap*	     f_listmap;
	int			     f_output_enabled;
} t_hoa_map;

typedef struct _linkmap
{
	t_linkmap *next;
	t_hoa_map *map;
	void update_headptr(t_linkmap *linkmap_headptr, Source::Manager* manager)
	{
		map->f_listmap = linkmap_headptr;
		map->f_manager = manager;
		if(next != NULL)
			next->update_headptr(linkmap_headptr, manager);
	}
} t_linkmap;

static    t_eclass *hoa_map_class;

/*New/free*/
void      hoa_map_free(t_hoa_map *x);
void*     hoa_map_new(t_symbol *s, int argc, t_atom *argv);

/*Initialisation par l'utilisateur*/
void      hoa_map_clearAll(t_hoa_map *x);
void      hoa_map_set(t_hoa_map *x, t_symbol *s, short ac, t_atom *av);
void      hoa_map_group(t_hoa_map *x, t_symbol *s, short ac, t_atom *av);
void      hoa_map_source(t_hoa_map *x, t_symbol *s, short ac, t_atom *av);
t_pd_err  hoa_map_zoom(t_hoa_map *x, t_object *attr, long argc, t_atom *argv);
t_pd_err  hoa_map_view(t_hoa_map *x, t_object *attr, long argc, t_atom *argv);
t_pd_err  hoa_map_notify(t_hoa_map *x, t_symbol *s, t_symbol *msg, void *sender, void *data);

/*Sorties*/
void      hoa_map_infos(t_hoa_map *x);
void      hoa_map_output(t_hoa_map *x);

/*Paint*/
void      hoa_map_paint(t_hoa_map *x, t_object *view);
void      hoa_map_popup(t_hoa_map *x, t_symbol *s, long itemid);
void      hoa_map_drawGroups(t_hoa_map *x,  t_object *view, t_rect *rect);
void      hoa_map_drawSources(t_hoa_map *x,  t_object *view, t_rect *rect);
void      hoa_map_drawBackground(t_hoa_map *x, t_object *view, t_rect *rect);
void      hoa_map_drawRectSelection(t_hoa_map *x,  t_object *view, t_rect *rect);

/*Souris et clavier*/
t_symbol* hoa_map_stringFormat(const char *s);
void      hoa_map_preset(t_hoa_map *x, t_binbuf *b);
void      hoa_map_sourcesPreset(t_hoa_map *x, t_symbol *s, short ac, t_atom *av);
void      hoa_map_mouseup(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers);
void      hoa_map_mousedown(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers);
void      hoa_map_mousedrag(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers);
void      hoa_map_mousemove(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers);
void      hoa_map_interpolate(t_hoa_map *x, short ac, t_atom *av, short ac2, t_atom* av2, t_atom theta);
void      hoa_map_mousewheel(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers, double x_inc, double y_inc);

/*Others*/
void      hoa_map_oksize(t_hoa_map *x, t_rect *newrect);
void      hoa_map_isElementSelected(t_hoa_map *x, t_pt pt);
void      hoa_map_sendBindedMapUpdate(t_hoa_map *x, long flags);
t_pd_err  hoa_map_bindnameSet(t_hoa_map *x, void *attr, long argc, t_atom *argv);
void      hoa_map_linkmapAddWithBindingName(t_hoa_map *x, t_symbol* binding_name);
void      hoa_map_linkmapRemoveWithBindingName(t_hoa_map *x, t_symbol* binding_name);
void      hoa_map_getDrawParams(t_hoa_map *x, t_object *patcherview, t_edrawparams *params);

t_symbol* hoa_sym_sources_preset;
t_symbol* hoa_sym_view_xy = gensym("xy");
t_symbol* hoa_sym_view_xz = gensym("xz");
t_symbol* hoa_sym_view_yz = gensym("yz");

extern "C" void setup_hoa0x2emap(void)
{
	t_eclass *c;

	c = eclass_new("hoa.map", (method)hoa_map_new, (method)hoa_map_free, sizeof(t_hoa_map), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_map_new, gensym("hoa.2d.map"), A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_map_new, gensym("hoa.3d.map"), A_GIMME, 0);

    eclass_init(c, 0);

	eclass_addmethod(c, (method) hoa_map_paint,         "paint",          A_CANT,  0);
	eclass_addmethod(c, (method) hoa_map_getDrawParams, "getDrawParams",  A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_oksize,        "oksize",         A_CANT,  0);
	eclass_addmethod(c, (method) hoa_map_notify,        "notify",         A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_output,        "bang",           A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_infos,         "getinfo",        A_GIMME, 0);

    eclass_addmethod(c, (method) hoa_map_source,        "source",         A_GIMME, 0);
    eclass_addmethod(c, (method) hoa_map_group,         "group",          A_GIMME, 0);
    eclass_addmethod(c, (method) hoa_map_clearAll,      "clear",          A_GIMME, 0);
	eclass_addmethod(c, (method) hoa_map_set,			"set",			  A_GIMME, 0);

    eclass_addmethod(c, (method) hoa_map_mousedown,     "mousedown",      A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_mousedrag,     "mousedrag",      A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_mouseup,       "mouseup",        A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_mousemove,     "mousemove",      A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_mousewheel,    "mousewheel",     A_CANT,  0);
	eclass_addmethod(c, (method) hoa_map_popup,         "popup",          A_CANT,  0);

    eclass_addmethod(c, (method) hoa_map_preset,        "preset",         A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_interpolate,   "interpolate",    A_CANT,  0);
    eclass_addmethod(c, (method) hoa_map_sourcesPreset, "sources_preset",  A_GIMME, 0);

	CLASS_ATTR_DEFAULT              (c, "size", 0, "225 225");

    CLASS_ATTR_RGBA					(c, "bgcolor", 0, t_hoa_map, f_color_bg);
	CLASS_ATTR_CATEGORY				(c, "bgcolor", 0, "Color");
	CLASS_ATTR_STYLE				(c, "bgcolor", 0, "rgba");
	CLASS_ATTR_LABEL				(c, "bgcolor", 0, "Background Color");
	CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bgcolor", 0, "0.76 0.76 0.76 1.");

    CLASS_ATTR_RGBA					(c, "bdcolor", 0, t_hoa_map, f_color_bd);
	CLASS_ATTR_CATEGORY				(c, "bdcolor", 0, "Color");
	CLASS_ATTR_STYLE                (c, "bdcolor", 0, "rgba");
    CLASS_ATTR_LABEL				(c, "bdcolor", 0, "Border Color");
	CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bdcolor", 0, "0.7 0.7 0.7 1.");

	CLASS_ATTR_SYMBOL				(c, "view", 0, t_hoa_map, f_coord_view);
	CLASS_ATTR_LABEL				(c, "view", 0, "Coordinate View");
	CLASS_ATTR_ITEMS				(c, "view", 0, "xy xz yz");
	CLASS_ATTR_STYLE                (c, "view", 1, "menu");
	CLASS_ATTR_DEFAULT				(c, "view", 0,  "xy");
    CLASS_ATTR_SAVE					(c, "view", 1);
    CLASS_ATTR_ORDER				(c, "view", 0, "1");

	CLASS_ATTR_SYMBOL               (c, "outputmode", 0, t_hoa_map, f_output_mode);
	CLASS_ATTR_LABEL                (c, "outputmode", 0, "Output Mode");
	CLASS_ATTR_CATEGORY             (c, "outputmode", 0, "Behavior");
	CLASS_ATTR_DEFAULT              (c, "outputmode", 0, "polar");
    CLASS_ATTR_SAVE                 (c, "outputmode", 1);
    CLASS_ATTR_ORDER                (c, "outputmode", 0, "1");
    CLASS_ATTR_STYLE                (c, "outputmode", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "outputmode", 1, "polar cartesian");

	CLASS_ATTR_DOUBLE               (c, "zoom", 0, t_hoa_map, f_zoom_factor);
    CLASS_ATTR_ACCESSORS            (c, "zoom", NULL, hoa_map_zoom);
	CLASS_ATTR_LABEL                (c, "zoom", 0, "Zoom");
	CLASS_ATTR_CATEGORY             (c, "zoom", 0, "Behavior");
	CLASS_ATTR_DEFAULT              (c, "zoom", 0, "0.35");
    CLASS_ATTR_ORDER                (c, "zoom", 0, "2");
    CLASS_ATTR_SAVE                 (c, "zoom", 1);
    CLASS_ATTR_PAINT                (c, "zoom", 0);
    CLASS_ATTR_STYLE                (c, "zoom", 0, "number");

	CLASS_ATTR_SYMBOL				(c, "mapname", 0, t_hoa_map, f_binding_name);
	CLASS_ATTR_ACCESSORS			(c, "mapname", NULL, hoa_map_bindnameSet);
	CLASS_ATTR_LABEL				(c, "mapname", 0, "Map Name");
	CLASS_ATTR_CATEGORY				(c, "mapname", 0, "Name");
	CLASS_ATTR_DEFAULT				(c, "mapname", 0,  "(null)");
    CLASS_ATTR_SAVE					(c, "mapname", 1);
    CLASS_ATTR_ORDER				(c, "mapname", 0, "1");

    eclass_register(CLASS_BOX, c);
	hoa_map_class = c;

    hoa_sym_sources_preset = gensym("sources_preset");
}

void *hoa_map_new(t_symbol *s, int argc, t_atom *argv)
{
	t_hoa_map *x =  NULL;
	t_binbuf *d;
	ulong flags;

	if (!(d = binbuf_via_atoms(argc,argv)))
		return NULL;

    x = (t_hoa_map *)eobj_new(hoa_map_class);
    if (x)
    {
        flags = 0
        | EBOX_GROWLINK
        ;

        x->f_manager      = new Source::Manager(1. / (double)MIN_ZOOM - 5.);
        x->f_self_manager = x->f_manager;

        x->f_rect_selection_exist = 0;
        x->f_read   = 0;
        x->f_write  = 0;
        x->f_out_sources    = listout(x);
        x->f_out_groups     = listout(x);
        x->f_out_infos      = listout(x);

		x->f_binding_name = hoa_sym_null;
		x->f_listmap = NULL;
		x->f_output_enabled = 1;

		ebox_new((t_ebox *)x, flags);

        ebox_attrprocess_viabinbuf(x, d);

        ebox_ready((t_ebox *)x);
    }

	return (x);
}

void hoa_map_free(t_hoa_map *x)
{
	hoa_map_linkmapRemoveWithBindingName(x, x->f_binding_name);

    ebox_free((t_ebox *)x);
    delete x->f_self_manager;
}

void hoa_map_linkmapAddWithBindingName(t_hoa_map *x, t_symbol* binding_name)
{
	char strname[2048];
	t_symbol* name = NULL;
	t_canvas *canvas = canvas_getrootfor(eobj_getcanvas(x));

    if(!binding_name || binding_name == hoa_sym_nothing || binding_name == hoa_sym_null)
        return;

	if(canvas)
	{
		sprintf(strname, "p%ld_%s_%s", (ulong)canvas, binding_name->s_name, ODD_BINDING_SUFFIX);
		name = gensym(strname);

		if(name->s_thing == NULL)
		{
			x->f_listmap = (t_linkmap *)malloc(sizeof(t_linkmap));
			if (x->f_listmap)
			{
				x->f_listmap->map = x;
				x->f_listmap->next = NULL;
				name->s_thing = (t_class **)x->f_listmap;
				x->f_manager = x->f_self_manager;
			}
		}
		else // t_listmap exist => add our object in it
		{
			t_linkmap *temp, *temp2;

			if(x->f_listmap != NULL)
			{
				temp = x->f_listmap;
				while(temp)
				{
					if(temp->next != NULL && temp->next->map == x)
					{
						temp2 = temp->next->next;
						free(temp->next);
						temp->next = temp2;
					}
					temp = temp->next;
				}
			}

			x->f_listmap = (t_linkmap *)name->s_thing;
			temp = x->f_listmap;
			t_hoa_map* head_map = temp->map;

			while(temp)
			{
				if(temp->next == NULL)
				{
					temp2 = (t_linkmap *)malloc(sizeof(t_linkmap));
					if (temp2)
					{
						temp2->map = x;
						temp2->next = NULL;
						temp->next = temp2;
						temp->next->map->f_manager = head_map->f_self_manager;
					}
					break;
				}
				temp = temp->next;
			}
		}
	}
}

void hoa_map_linkmapRemoveWithBindingName(t_hoa_map *x, t_symbol* binding_name)
{
	char strname[2048];
	t_symbol* name = NULL;
	t_canvas *canvas = canvas_getrootfor(eobj_getcanvas(x));

    if(!binding_name || binding_name == hoa_sym_nothing || binding_name == hoa_sym_null)
        return;

	if(canvas)
	{
		sprintf(strname, "p%ld_%s_%s", (ulong)canvas, binding_name->s_name, ODD_BINDING_SUFFIX);
		name = gensym(strname);

		if(name->s_thing != NULL)
		{
			t_linkmap *temp, *temp2;
			temp = (t_linkmap *)name->s_thing;
			t_hoa_map* head_map = temp->map;
			int counter = 0;

			while(temp)
			{
				if (counter == 0 && temp->map == x) // head of the linkmap
				{
					head_map = temp->map;

					if(temp->next == NULL) // is also the last item of the linkmap
					{
						name->s_thing = NULL;
					}
					else
					{
						name->s_thing = (t_class **)temp->next;

						// bind all object to the next Source::Manager (next becoming the new head of the t_linkmap)
						temp->next->map->f_self_manager = new Source::Manager(*head_map->f_manager);
						temp->next->update_headptr((t_linkmap *)name->s_thing, temp->next->map->f_self_manager);
					}

                    //free(x->f_listmap);
                    x->f_listmap = NULL;

					x->f_manager = x->f_self_manager; // not sure if this is necessary (normally it is the same pointer)
				}
				else if(temp->next != NULL && temp->next->map == x)
				{
					// we restore the original pointer
					temp->next->map->f_manager = temp->next->map->f_manager;
					// then we copy the shared Source::Manager into the original one
					temp->next->map->f_manager = new Source::Manager(*head_map->f_self_manager);

					temp2 = temp->next->next;
					free(temp->next);
					x->f_listmap = NULL;
					temp->next = temp2;
				}

				temp = temp->next;
			}
		}
	}
}

t_pd_err hoa_map_bindnameSet(t_hoa_map *x, void *attr, long argc, t_atom *argv)
{
	if (argc && argv && atom_gettype(argv) == A_SYM)
	{
		t_symbol* new_binding_name = atom_getsym(argv);

		if(new_binding_name != x->f_binding_name)
		{
			hoa_map_linkmapRemoveWithBindingName(x, x->f_binding_name);
			if(new_binding_name != hoa_sym_nothing || new_binding_name != hoa_sym_null)
			{
				hoa_map_linkmapAddWithBindingName(x, new_binding_name);
				x->f_binding_name = new_binding_name;
			}
			else
            {
				x->f_binding_name = hoa_sym_null;
            }

            ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
            ebox_redraw((t_ebox *)x);
            hoa_map_output(x);
		}
	}
	else
	{
       hoa_map_linkmapRemoveWithBindingName(x, x->f_binding_name);
		x->f_binding_name = hoa_sym_null;
	}

	return 0;
}

void hoa_map_sendBindedMapUpdate(t_hoa_map *x, long flags)
{
	if(x->f_listmap)
	{
		t_linkmap *temp = x->f_listmap;
		t_object* mapobj;
		while (temp)
		{
			mapobj = (t_object*)temp->map;

			if (mapobj != (t_object*)x)
			{
				if (flags & BMAP_REDRAW)
				{
					ebox_invalidate_layer((t_ebox *)mapobj, hoa_sym_sources_layer);
					ebox_invalidate_layer((t_ebox *)mapobj, hoa_sym_groups_layer);
					ebox_redraw((t_ebox *)mapobj);
				}
				if (flags & BMAP_NOTIFY)
				{
					ebox_notify((t_ebox *)mapobj, NULL, hoa_sym_modified, NULL, NULL);
				}
				if (flags & BMAP_OUTPUT && x->f_output_enabled)
				{
					pd_bang((t_pd *)mapobj);
				}
			}

			temp = temp->next;
		}
	}
}

void hoa_map_getDrawParams(t_hoa_map *x, t_object *patcherview, t_edrawparams *params)
{
    params->d_boxfillcolor = x->f_color_bg;
    params->d_bordercolor = x->f_color_bd;
	params->d_borderthickness = 1;
	params->d_cornersize = 8;
}

void hoa_map_oksize(t_hoa_map *x, t_rect *newrect)
{
    newrect->width = pd_clip_min(newrect->width, 20.);
    newrect->height = pd_clip_min(newrect->height, 20.);
}

void hoa_map_isElementSelected(t_hoa_map *x, t_pt pt)
{
    t_pt cursor, displayed_coords;
    cursor.x = ((pt.x / x->rect.width * 2.) - 1.) / x->f_zoom_factor;
    cursor.y = ((-pt.y / x->rect.height * 2.) + 1.) / x->f_zoom_factor;
	double distanceSelected = ebox_getfontsize((t_ebox *)x) / (x->f_zoom_factor * 2. * x->rect.width);
	double distanceSelected_test;

    x->f_cursor_position.x = cursor.x;
    x->f_cursor_position.y = cursor.y;

    x->f_selected_source = NULL;
    x->f_selected_group = NULL;

	for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource() ; it ++)
    {
        if(x->f_coord_view == hoa_sym_view_xy)
        {
            displayed_coords.x = it->second->getAbscissa();
            displayed_coords.y = it->second->getOrdinate();
        }
        else if(x->f_coord_view == hoa_sym_view_xz)
        {
            displayed_coords.x = it->second->getAbscissa();
            displayed_coords.y = it->second->getHeight();
        }
        else
        {
            displayed_coords.x = it->second->getOrdinate();
            displayed_coords.y = it->second->getHeight();
        }
		distanceSelected_test = hoa_pd_distance(displayed_coords.x, displayed_coords.y, cursor.x, cursor.y);

        if(distanceSelected_test <= distanceSelected)
        {
            distanceSelected = distanceSelected_test;
            x->f_selected_source = it->second;
        }
    }

    if(!x->f_selected_source)
    {
        for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup() ; it ++)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                displayed_coords.x = it->second->getAbscissa();
                displayed_coords.y = it->second->getOrdinate();
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                displayed_coords.x = it->second->getAbscissa();
                displayed_coords.y = it->second->getHeight();
            }
            else
            {
                displayed_coords.x = it->second->getOrdinate();
                displayed_coords.y = it->second->getHeight();
            }
			distanceSelected_test = hoa_pd_distance(displayed_coords.x, displayed_coords.y, cursor.x, cursor.y);

			if(distanceSelected_test <= distanceSelected)
			{
				distanceSelected = distanceSelected_test;
				x->f_selected_group = it->second;
			}
        }
    }
}

/**********************************************************/
/*          Intialisation par l'utilisateur               */
/**********************************************************/

void hoa_map_clearAll(t_hoa_map *x)
{
	// mute all source and output before clearing them to notify hoa.#.map~
	for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource() ; it ++)
			it->second->setMute(true);

	hoa_map_output(x);
	hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);

	// now we can clear, then notify, output and redraw all maps
    x->f_manager->clear();

    ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
    ebox_redraw((t_ebox *)x);
    hoa_map_output(x);
	hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW | BMAP_OUTPUT | BMAP_NOTIFY);
}

void hoa_map_set(t_hoa_map *x, t_symbol *s, short ac, t_atom *av)
{
	x->f_output_enabled = 0;
	if (ac && av && atom_gettype(av) == A_SYM)
	{
		t_symbol* msgtype = atom_getsym(av);
		av++; ac--;
		if (msgtype == hoa_sym_source)
			pd_typedmess((t_pd *)x, hoa_sym_source, ac, av);
		else if (msgtype == hoa_sym_group)
			pd_typedmess((t_pd *)x, hoa_sym_group, ac, av);
	}
	x->f_output_enabled = 1;
}

void hoa_map_source(t_hoa_map *x, t_symbol *s, short ac, t_atom *av)
{
    int index;
    if(ac && av && atom_gettype(av)==A_LONG && atom_getlong(av) >= 1 && atom_gettype(av+1) == A_SYM)
    {
		t_symbol* param = atom_getsym(av+1);
        index = atom_getlong(av);
		int causeOutput = 1;
        if (index > 0)
        {
            Source* tmp = x->f_manager->newSource(index);

            if(param == hoa_sym_polar || param == hoa_sym_pol)
            {
                if (ac >= 5 && atom_isNumber(av+2) && atom_isNumber(av+3) && atom_isNumber(av+4))
                    tmp->setCoordinatesPolar(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4));
                else if (ac >= 4 && atom_isNumber(av+2) && atom_isNumber(av+3))
                    tmp->setCoordinatesPolar(index, atom_getfloat(av+2), atom_getfloat(av+3));
            }
            else if(param == hoa_sym_radius)
                tmp->setRadius(atom_getfloat(av+2));
            else if(param == hoa_sym_azimuth)
                tmp->setAzimuth(atom_getfloat(av+2));
            else if(param == hoa_sym_elevation)
                tmp->setElevation(atom_getfloat(av+2));
            else if(param == hoa_sym_cartesian || param == hoa_sym_car)
            {
                if (ac >= 5 && atom_isNumber(av+2) && atom_isNumber(av+3) && atom_isNumber(av+4))
                    tmp->setCoordinatesCartesian(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4));
                else if (ac >= 4 && atom_isNumber(av+2) && atom_isNumber(av+3))
                    tmp->setCoordinatesCartesian(atom_getfloat(av+2), atom_getfloat(av+3));
            }
            else if(param == hoa_sym_abscissa)
                tmp->setAbscissa(atom_getfloat(av+2));
            else if(param == hoa_sym_ordinate)
                tmp->setOrdinate(atom_getfloat(av+2));
            else if(param == hoa_sym_height)
                tmp->setHeight(atom_getfloat(av+2));
            else if(param == hoa_sym_remove)
            {
                x->f_manager->removeSource(index);
                t_atom av[3];
                atom_setlong(av, index);
                atom_setsym(av+1, hoa_sym_mute);
                atom_setlong(av+2, 1);
                outlet_list(x->f_out_sources, 0L, 3, av);
            }
            else if(param == hoa_sym_mute)
                tmp->setMute(atom_getlong(av+2));
            else if(param == hoa_sym_description)
            {
                causeOutput = 0;
                char description[250];
                char number[250];
                if(atom_gettype(av+1) == A_SYM)
                {
                    strcpy(description, atom_getsym(av+2)->s_name);
                    strcat(description, " ");
                    if(atom_getsym(av+2) == hoa_sym_remove)
                    {
                        tmp->setDescription("");
                        ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
                        ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
                        ebox_redraw((t_ebox *)x);
                        return;
                    }
                }
                for(int i = 3; i < ac; i++)
                {
                    if(atom_gettype(av+i) == A_SYM)
                    {
                        strcat(description, atom_getsym(av+i)->s_name);
                        strcat(description, " ");
                    }
                    else if(atom_gettype(av+i) == A_LONG)
                    {
                        sprintf(number, "%ld ", (ulong)atom_getlong(av+i));
                        strcat(description, number);
                    }
                    else if(atom_gettype(av+i) == A_FLOAT)
                    {
                        sprintf(number, "%f ", atom_getfloat(av+i));
                        strcat(description, number);
                    }
                }
                tmp->setDescription(description);
            }
            else if(param == hoa_sym_color && ac >= 5)
            {
                tmp->setColor(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4), atom_getfloat(av+5));
                causeOutput = 0;
            }
            else
            {
                causeOutput = 0;
            }

            ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
            ebox_redraw((t_ebox *)x);

            if (causeOutput)
            {
                hoa_map_output(x);
                hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
            }
            hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW | BMAP_NOTIFY);
        }
    }
}

void hoa_map_group(t_hoa_map *x, t_symbol *s, short ac, t_atom *av)
{
    if(ac && av && atom_gettype(av) == A_LONG && atom_getlong(av) >= 1 && atom_gettype(av+1) == A_SYM)
    {
        ulong index = atom_getlong(av);
		t_symbol* param = atom_getsym(av+1);
		int causeOutput = 1;
		if (index > 0)
        {
            bool newGroupCreated = false;
            Source::Group* tmp = x->f_manager->getGroup(index);
            if (!tmp)
            {
                tmp = x->f_manager->createGroup(index);
                newGroupCreated = true;
            }

            if(param == hoa_sym_set)
            {
                for(int i = 2; i < ac; i++)
                {
                    ulong ind = atom_getlong(av+i);
                    if (ind > 0)
                    {
                        Source* src = x->f_manager->newSource(ind);
                        tmp->addSource(src);
                    }
                }
            }
            else if(param == hoa_sym_polar || param == hoa_sym_pol)
            {
                if (ac >= 5 && atom_isNumber(av+2) && atom_isNumber(av+3) && atom_isNumber(av+4))
                    tmp->setCoordinatesPolar(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4));
                else if (ac >= 4 && atom_isNumber(av+2) && atom_isNumber(av+3))
                    tmp->setCoordinatesPolar(atom_getfloat(av+2), atom_getfloat(av+3));
            }
            else if(param == hoa_sym_azimuth)
                tmp->setAzimuth(atom_getfloat(av+2));
            else if(param == hoa_sym_elevation)
                tmp->setElevation(atom_getfloat(av+2));
            else if(param == hoa_sym_cartesian || param == hoa_sym_car)
            {
                if (ac >= 5 && atom_isNumber(av+2) && atom_isNumber(av+3) && atom_isNumber(av+4))
                    tmp->setCoordinatesCartesian(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4));
                else if (ac >= 4 && atom_isNumber(av+2) && atom_isNumber(av+3))
                    tmp->setCoordinatesCartesian(atom_getfloat(av+2), atom_getfloat(av+3));
            }
            else if(param == hoa_sym_abscissa)
                tmp->setAbscissa(atom_getfloat(av+2));
            else if(param == hoa_sym_ordinate)
                tmp->setOrdinate(atom_getfloat(av+2));
            else if(param == hoa_sym_height)
                tmp->setHeight(atom_getfloat(av+2));
            else if(param == hoa_sym_relpolar)
            {
                if (ac >= 5 && atom_isNumber(av+2) && atom_isNumber(av+3) && atom_isNumber(av+4))
                    tmp->setRelativeCoordinatesPolar(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4));
                else if (ac >= 4 && atom_isNumber(av+2) && atom_isNumber(av+3))
                    tmp->setRelativeCoordinatesPolar(atom_getfloat(av+2), atom_getfloat(av+3));
            }
            else if(param == hoa_sym_relradius)
            {
                tmp->setRelativeRadius(atom_getfloat(av+2));
            }
            else if(param == hoa_sym_relazimuth)
            {
                tmp->setRelativeAzimuth(atom_getfloat(av+2));
            }
            else if(param == hoa_sym_relelevation)
            {
                tmp->setRelativeElevation(atom_getfloat(av+2));
            }
            else if(param == hoa_sym_mute)
            {
                tmp->setMute(atom_getlong(av+2));
            }
            else if(param == hoa_sym_remove)
            {
                x->f_manager->removeGroup(index);
                t_atom av[3];
                atom_setlong(av, index+1);
                atom_setsym(av+1, hoa_sym_mute);
                atom_setlong(av+2, 1);
                outlet_list(x->f_out_groups, 0L, 3, av);
            }
            else if(param == hoa_sym_description)
            {
                causeOutput = 0;
                char description[250];
                char number[250];
                if(atom_gettype(av+1) == A_SYM)
                {
                    strcpy(description, atom_getsym(av+2)->s_name);
                    strcat(description, " ");
                    if(atom_getsym(av+2) == hoa_sym_remove)
                    {
                        tmp->setDescription("");
                        ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
                        ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
                        ebox_redraw((t_ebox *)x);
                        return;
                    }
                }
                for(int i = 3; i < ac; i++)
                {
                    if(atom_gettype(av+i) == A_SYM)
                    {
                        strcat(description, atom_getsym(av+i)->s_name);
                        strcat(description, " ");
                    }
                    else if(atom_gettype(av+i) == A_LONG)
                    {
                        sprintf(number, "%ld ", (ulong)atom_getlong(av+i));
                        strcat(description, number);
                    }
                    else if(atom_gettype(av+i) == A_FLOAT)
                    {
                        sprintf(number, "%f ", atom_getfloat(av+i));
                        strcat(description, number);
                    }
                }
                tmp->setDescription(description);
            }
            else if(param == hoa_sym_color && ac >= 6)
            {
                causeOutput = 0;
                tmp->setColor(atom_getfloat(av+2), atom_getfloat(av+3), atom_getfloat(av+4), atom_getfloat(av+5));
            }
            else
            {
                causeOutput = 0;
            }

            if (newGroupCreated)
            {
                if (!x->f_manager->addGroup(tmp))
                {
                    delete tmp;
                }
            }
        }

		ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
		ebox_redraw((t_ebox *)x);
		if (causeOutput)
		{
			hoa_map_output(x);
			hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
		}
		hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW | BMAP_NOTIFY);
    }
}

t_pd_err hoa_map_zoom(t_hoa_map *x, t_object *attr, long argc, t_atom *argv)
{
    if(argc >= 1 && argv && atom_gettype(argv) == A_FLOAT)
    {
        x->f_zoom_factor = atom_getfloat(argv);
        x->f_zoom_factor = pd_clip_minmax(x->f_zoom_factor, MIN_ZOOM, MAX_ZOOM);
    }

    ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
    return 0;
}

t_pd_err hoa_map_view(t_hoa_map *x, t_object *attr, long argc, t_atom *argv)
{
    post("oui");
    return 0;
}

t_pd_err hoa_map_notify(t_hoa_map *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
    if (msg == gensym("attr_modified"))
    {
        if(s == gensym("bgcolor"))
        {
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        }
        else if(s == gensym("fontname") || s == gensym("fontface") || s == gensym("fontsize"))
        {
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
        }
        else if(s == gensym("zoom"))
        {
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
            ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
        }
        ebox_redraw((t_ebox *)x);
    }

	return ebox_notify((t_ebox *)x, s, msg, sender, data);
}

/**********************************************************/
/*                          Sortie                        */
/**********************************************************/

void hoa_map_output(t_hoa_map *x)
{
	if (!x->f_output_enabled)
		return;

	t_atom av[5];
    atom_setsym(av+1, hoa_sym_mute);

	// output group mute state
	for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
    {
        atom_setlong(av, it->first);
        atom_setfloat(av+2, it->second->getMute());
        outlet_list(x->f_out_groups, 0L, 3, av);
    }
	// output source mute state
    for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource(); it ++)
    {
        atom_setlong(av, it->first);
        atom_setlong(av+2, it->second->getMute());
        outlet_list(x->f_out_sources, 0L, 3, av);
    }
    if(x->f_output_mode == hoa_sym_polar)
    {
        atom_setsym(av+1, hoa_sym_polar);
		for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
        {
            atom_setlong(av, it->first);
            atom_setfloat(av+2, it->second->getRadius());
            atom_setfloat(av+3, it->second->getAzimuth());
            atom_setfloat(av+4, it->second->getElevation());
            outlet_list(x->f_out_groups, 0L, 5, av);
        }
        for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource(); it ++)
        {
            atom_setlong(av, it->first);
            atom_setfloat(av+2, it->second->getRadius());
            atom_setfloat(av+3, it->second->getAzimuth());
            atom_setfloat(av+4, it->second->getElevation());
            outlet_list(x->f_out_sources, 0L, 5, av);
        }
    }
    else
    {
        atom_setsym(av+1, hoa_sym_cartesian);
		for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
        {
            atom_setlong(av, it->first);
            atom_setfloat(av+2, it->second->getAbscissa());
            atom_setfloat(av+3, it->second->getOrdinate());
            atom_setfloat(av+4, it->second->getHeight());
            outlet_list(x->f_out_groups, 0L, 5, av);
        }
        for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource(); it ++)
        {
            atom_setlong(av, it->first);
            atom_setfloat(av+2, it->second->getAbscissa());
            atom_setfloat(av+3, it->second->getOrdinate());
            atom_setfloat(av+4, it->second->getHeight());
            outlet_list(x->f_out_sources, 0L, 5, av);
        }
    }
}

void hoa_map_infos(t_hoa_map *x)
{
    t_atom avNumber[3];
    t_atom* avIndex;
    t_atom* avSource;
    t_atom avMute[4];

    // Sources
    ulong numberOfSource = x->f_manager->getNumberOfSources();
    atom_setsym(avNumber, hoa_sym_source);
    atom_setsym(avNumber+1, hoa_sym_number);
    atom_setlong(avNumber+2, numberOfSource);
    outlet_list(x->f_out_infos, &s_list, 3, avNumber);

    avIndex = new t_atom[numberOfSource+2];
    atom_setsym(avIndex, hoa_sym_source);
    atom_setsym(avIndex+1, hoa_sym_index);

    ulong j = 0;
    for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource(); it ++)
    {
        atom_setlong(avIndex+j+2, it->first);
        j ++;
    }
    outlet_list(x->f_out_infos, 0L, numberOfSource+2, avIndex);
    delete [] avIndex;

    atom_setsym(avMute, hoa_sym_source);
    atom_setsym(avMute+1, hoa_sym_mute);
    for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource(); it ++)
    {
        atom_setlong(avMute+2, it->first);
        atom_setlong(avMute+3, it->second->getMute());
        outlet_list(x->f_out_infos, &s_list, 4, avMute);
    }

    // Groups
    ulong numberOfGroups = x->f_manager->getNumberOfGroups();
    atom_setsym(avNumber, hoa_sym_group);
    atom_setsym(avNumber+1, hoa_sym_number);
    atom_setlong(avNumber+2, numberOfGroups);
    outlet_list(x->f_out_infos, 0L, 3, avNumber);

    avIndex = new t_atom[numberOfGroups+2];
    atom_setsym(avIndex, hoa_sym_group);
    atom_setsym(avIndex+1, hoa_sym_index);
    j = 0;
    for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
    {
        atom_setlong(avIndex+j+2, it->first);
        j++;
    }
    outlet_list(x->f_out_infos, &s_list, numberOfGroups+2, avIndex);
    free(avIndex);

    for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
    {
        avSource = new t_atom[it->second->getNumberOfSources()+3];
        atom_setsym(avSource, hoa_sym_group);
        atom_setsym(avSource+1, hoa_sym_source);
        atom_setlong(avSource+2, it->first);

        j = 0;
        map<ulong, Source*>& sourcesOfGroup = it->second->getSources();
        for(Source::source_iterator ti = sourcesOfGroup.begin() ; ti != sourcesOfGroup.end(); ti ++)
        {
            atom_setlong(avSource+3+j, ti->first);
            j ++;
        }
        outlet_list(x->f_out_infos, &s_list, it->second->getNumberOfSources()+3, avSource);
        free(avSource);
    }


    atom_setsym(avMute, hoa_sym_group);
    atom_setsym(avMute+1, hoa_sym_mute);
    for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup(); it ++)
    {
        atom_setlong(avMute+2, it->first);
        atom_setlong(avMute+3, it->second->getMute());
        outlet_list(x->f_out_infos, &s_list, 4, avMute);
    }
}

/**********************************************************/
/*                          Paint                         */
/**********************************************************/

void hoa_map_paint(t_hoa_map *x, t_object *view)
{
    t_rect rect;
    ebox_get_rect_for_view((t_ebox *)x, &rect);
	x->rect = rect;

	hoa_map_drawBackground(x, view, &rect);
    hoa_map_drawRectSelection(x, view, &rect);
    hoa_map_drawSources(x, view, &rect);
    hoa_map_drawGroups(x, view, &rect);
}

void hoa_map_drawBackground(t_hoa_map *x,  t_object *view, t_rect *rect)
{
    t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_background_layer, rect->width, rect->height);

    t_rgba black = rgba_addContrast(x->f_color_bg, -0.14);
	if (g)
    {
        egraphics_set_color_rgba(g, &x->f_color_bg);
        egraphics_circle(g, rect->width / 2., rect->width / 2., (rect->width / 2.) * (1. / MIN_ZOOM * x->f_zoom_factor) - 1.);
        egraphics_fill(g);

        /* Circles */
        double radius  = x->f_zoom_factor * rect->width / 10.;
        for(int i = 5; i > 0; i--)
        {

            egraphics_set_line_width(g, 2);
            egraphics_set_color_rgba(g, &x->f_color_bg);
            egraphics_circle(g, rect->width / 2 - 0.5, rect->width / 2 - 0.5, (double)i * radius - 1.);
            egraphics_stroke(g);

            egraphics_set_line_width(g, 1);
            egraphics_set_color_rgba(g, &black);
            egraphics_circle(g, rect->width / 2, rect->width / 2, (double)i * radius - 1.);
            egraphics_stroke(g);

        }

        double ecart = x->f_zoom_factor * rect->width / 2.;
        if(ecart < 10 && ecart >= 5)
            ecart *= 2;
        else if(ecart < 5 && ecart > 2.5)
            ecart *= 4;
        else if(ecart < 2.5)
            ecart *= 8;

        ecart = (int)ecart;
		for(double i = 0; i < rect->width / 2.; i += ecart)
        {
            egraphics_set_line_width(g, 2);
            egraphics_set_color_rgba(g, &x->f_color_bg);
            egraphics_move_to(g, 0. - 0.5, rect->width / 2. - i - 0.5);
            egraphics_line_to(g, rect->width - 0.5, rect->width / 2. - i - 0.5);
            egraphics_stroke(g);
            egraphics_move_to(g, 0. - 0.5, rect->width / 2. + i - 0.5);
            egraphics_line_to(g, rect->width - 0.5, rect->width / 2. + i - 0.5);
            egraphics_stroke(g);
            egraphics_move_to(g, rect->width / 2. - i - 0.5, 0. - 0.5);
            egraphics_line_to(g, rect->width / 2. - i - 0.5, rect->width - 0.5);
            egraphics_stroke(g);
            egraphics_move_to(g, rect->width / 2. + i - 0.5, 0. - 0.5);
            egraphics_line_to(g, rect->width / 2. + i - 0.5, rect->width - 0.5);
            egraphics_stroke(g);

            egraphics_set_line_width(g, 1);
            egraphics_set_color_rgba(g, &black);
            egraphics_move_to(g, 0., rect->width / 2. - i);
            egraphics_line_to(g, rect->width, rect->width / 2. - i);
            egraphics_stroke(g);
            egraphics_move_to(g, 0., rect->width / 2. + i);
            egraphics_line_to(g, rect->width, rect->width / 2. + i);
            egraphics_stroke(g);
            egraphics_move_to(g, rect->width / 2. - i, 0.);
            egraphics_line_to(g, rect->width / 2. - i, rect->width);
            egraphics_stroke(g);
            egraphics_move_to(g, rect->width / 2. + i, 0.);
            egraphics_line_to(g, rect->width / 2. + i, rect->width);
            egraphics_stroke(g);
        }

		ebox_end_layer((t_ebox *)x, hoa_sym_background_layer);
	}
	ebox_paint_layer((t_ebox *)x, hoa_sym_background_layer, 0., 0.);
}

void hoa_map_drawSources(t_hoa_map *x,  t_object *view, t_rect *rect)
{
	t_etext *jtl;
	t_rgba sourceColor;
	char description[250];

    float w = rect->width;
    float h = rect->height;
    t_pt ctr = {w*0.5f, h*0.5f};
	t_pt sourceDisplayPos, groupDisplayPos, textDisplayPos;

	t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_sources_layer, rect->width, rect->height);
	t_rgba color_sel = rgba_addContrast(x->f_color_bg, -0.14);
    double font_size = ebox_getfontsize((t_ebox *)x);
    double source_size = font_size / 2.5;

	if (g)
    {
        jtl = etext_layout_create();
        egraphics_set_line_width(g, 1.);

		for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource() ; it ++)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                sourceDisplayPos.x = (it->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.y;
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                sourceDisplayPos.x = (it->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
            }
            else
            {
                sourceDisplayPos.x = (it->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
            }

            sourceColor.red = it->second->getColor()[0];
            sourceColor.green = it->second->getColor()[1];
            sourceColor.blue = it->second->getColor()[2];
            sourceColor.alpha = it->second->getColor()[3];

            if(it->second->getDescription().c_str()[0])
                sprintf(description,"%i : %s", (int)it->first, it->second->getDescription().c_str());
            else
                sprintf(description,"%i", (int)it->first);

            textDisplayPos.x = sourceDisplayPos.x - 2. * source_size;
            textDisplayPos.y = sourceDisplayPos.y - source_size - font_size - 1.;

            etext_layout_settextcolor(jtl, &sourceColor);
            etext_layout_set(jtl, description, &x->j_box.b_font, textDisplayPos.x, textDisplayPos.y, font_size * 10., font_size * 2., ETEXT_LEFT, ETEXT_JCENTER, ETEXT_NOWRAP);
            etext_layout_draw(jtl, g);

            if (x->f_selected_source && x->f_selected_source->getIndex() == it->first)
            {
                egraphics_set_color_rgba(g, &color_sel);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size * 1.5);
                egraphics_fill(g);

                map<ulong, Source::Group*>& groupsOfSources = it->second->getGroups();
                for(Source::group_iterator ti = groupsOfSources.begin() ; ti != groupsOfSources.end() ; ti ++)
                {
                    egraphics_move_to(g, sourceDisplayPos.x, sourceDisplayPos.y);

                    if(x->f_coord_view == hoa_sym_view_xy)
                    {
                        groupDisplayPos.x = (ti->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.y;
                    }
                    else if(x->f_coord_view == hoa_sym_view_xz)
                    {
                        groupDisplayPos.x = (ti->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
                    }
                    else
                    {
                        groupDisplayPos.x = (ti->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
                    }

                    egraphics_line_to(g, groupDisplayPos.x, groupDisplayPos.y);
                    egraphics_stroke(g);
                }
            }

            if(!it->second->getMute())
            {
                egraphics_set_color_rgba(g, &sourceColor);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size);
                egraphics_fill(g);
            }
            else
            {
                egraphics_set_color_rgba(g, &sourceColor);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size);
                egraphics_fill(g);
                egraphics_set_color_rgba(g, &rgba_red);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size);
                egraphics_stroke(g);
                egraphics_move_to(g, sourceDisplayPos.x + Math<float>::abscissa(source_size * 1., HOA_PI2 / 2.), sourceDisplayPos.y + Math<float>::ordinate(source_size * 1., HOA_PI2 / 2.));
                egraphics_line_to(g, sourceDisplayPos.x + Math<float>::abscissa(source_size * 1., HOA_PI2 * 5. / 2.), sourceDisplayPos.y + Math<float>::ordinate(source_size * 1., HOA_PI * 5. / 4.));
                egraphics_stroke(g);
            }
        }
		etext_layout_destroy(jtl);

		ebox_end_layer((t_ebox *)x, hoa_sym_sources_layer);
    }
	ebox_paint_layer((t_ebox *)x, hoa_sym_sources_layer, 0., 0.);
}

void hoa_map_drawGroups(t_hoa_map *x,  t_object *view, t_rect *rect)
{
	t_etext *jtl;
	t_rgba sourceColor;
	char description[250] = {0};

	t_pt sourceDisplayPos, groupDisplayPos, textDisplayPos;

    float w = rect->width;
    float h = rect->height;
    t_pt ctr = {w*0.5f, h*0.5f};

	t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_groups_layer, w, h);
    t_rgba color_sel = rgba_addContrast(x->f_color_bg, -0.14);
    double font_size = ebox_getfontsize((t_ebox *)x);
    double source_size = ebox_getfontsize((t_ebox *)x) / 2.;


	if (g)
    {
        jtl = etext_layout_create();
        egraphics_set_line_width(g, 2.);

		for(Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup() ; it ++)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                sourceDisplayPos.x = (it->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.y;
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                sourceDisplayPos.x = (it->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
            }
            else
            {
                sourceDisplayPos.x = (it->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.x;
                sourceDisplayPos.y = (-it->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
            }

            sourceColor.red = it->second->getColor()[0];
            sourceColor.green = it->second->getColor()[1];
            sourceColor.blue = it->second->getColor()[2];
            sourceColor.alpha = it->second->getColor()[3];

            if(it->second->getDescription().c_str()[0])
                sprintf(description,"%i : %s", (int)it->first, it->second->getDescription().c_str());
            else
                sprintf(description,"%i", (int)it->first);

            textDisplayPos.x = sourceDisplayPos.x - 2. * source_size;
            textDisplayPos.y = sourceDisplayPos.y - source_size - font_size - 1.;

            etext_layout_settextcolor(jtl, &sourceColor);
            etext_layout_set(jtl, description, &x->j_box.b_font, textDisplayPos.x, textDisplayPos.y, font_size * 10., font_size * 2., ETEXT_LEFT, ETEXT_JLEFT, ETEXT_NOWRAP);
            etext_layout_draw(jtl, g);

            if (x->f_selected_group && x->f_selected_group->getIndex() == it->first)
            {
                egraphics_set_color_rgba(g, &color_sel);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size * 1.5);
                egraphics_fill(g);

                map<ulong, Source*>& sourcesOfGroup = it->second->getSources();
                for(Source::source_iterator ti = sourcesOfGroup.begin() ; ti != sourcesOfGroup.end() ; ti ++)
                {
                    egraphics_move_to(g, sourceDisplayPos.x, sourceDisplayPos.y);

                    if(x->f_coord_view == hoa_sym_view_xy)
                    {
                        groupDisplayPos.x = (ti->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.y;
                    }
                    else if(x->f_coord_view == hoa_sym_view_xz)
                    {
                        groupDisplayPos.x = (ti->second->getAbscissa() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
                    }
                    else
                    {
                        groupDisplayPos.x = (ti->second->getOrdinate() * x->f_zoom_factor + 1.) * ctr.x;
                        groupDisplayPos.y = (-ti->second->getHeight() * x->f_zoom_factor + 1.) * ctr.y;
                    }

                    egraphics_line_to(g, groupDisplayPos.x, groupDisplayPos.y);
                    egraphics_stroke(g);
                }
            }
            egraphics_set_color_rgba(g, &sourceColor);

            if(!it->second->getMute())
            {
                egraphics_set_color_rgba(g, &sourceColor);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size * 1.);
                egraphics_stroke(g);
                etext_layout_draw(jtl, g);
            }
            else
            {
                egraphics_set_color_rgba(g, &rgba_red);
                egraphics_circle(g, sourceDisplayPos.x, sourceDisplayPos.y, source_size);
                egraphics_stroke(g);
                for(int j = 0; j < 2; j++)
                {
                    egraphics_move_to(g, sourceDisplayPos.x, sourceDisplayPos.y);
                    egraphics_line_to(g, sourceDisplayPos.x + Math<float>::abscissa(source_size * 1., HOA_2PI * j / 2. + HOA_PI2 / 2.), sourceDisplayPos.y + Math<float>::ordinate(source_size * 1., HOA_2PI * j / 2. + HOA_PI2 / 2.));
                    egraphics_stroke(g);
                }
            }
        }
		etext_layout_destroy(jtl);

		ebox_end_layer((t_ebox *)x, hoa_sym_groups_layer);
    }
	ebox_paint_layer((t_ebox *)x, hoa_sym_groups_layer, 0., 0.);
}

void hoa_map_drawRectSelection(t_hoa_map *x,  t_object *view, t_rect *rect)
{
	t_elayer *g;
    g = ebox_start_layer((t_ebox *)x, gensym("rect_selection_layer"), rect->width, rect->height);
    t_rgba color_sel = rgba_addContrast(x->f_color_bg, -0.14);
	if (g)
    {
		if (x->f_rect_selection_exist)
        {
			egraphics_set_line_width(g, 1);
			egraphics_set_color_rgba(g, &color_sel);
			egraphics_rectangle(g, x->f_rect_selection.x, x->f_rect_selection.y, x->f_rect_selection.width, x->f_rect_selection.height);
			egraphics_fill(g);
		}
		ebox_end_layer((t_ebox *)x, gensym("rect_selection_layer"));
	}
	ebox_paint_layer((t_ebox *)x, gensym("rect_selection_layer"), 0., 0.);
}

/**********************************************************/
/*                   Souris et clavier                    */
/**********************************************************/

void hoa_map_mousedown(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers)
{
    x->f_rect_selection_exist = -1;
    x->f_rect_selection.width = x->f_rect_selection.height = 0.;

    hoa_map_isElementSelected(x, pt);

    if(modifiers == EMOD_CMD) // popup (right-click)
    {
        t_pt pos = eobj_get_mouse_global_position(x);

        if(x->f_selected_group)
        {
            t_epopup* popup = epopupmenu_create((t_eobj *)x, hoa_sym_group);
            epopupmenu_setfont(popup, &x->j_box.b_font);
            epopupmenu_additem(popup, 0, "Group Menu", 0, 1);
            epopupmenu_addseperator(popup);
            epopupmenu_additem(popup, 1, "Remove group", 0, 0);
            epopupmenu_additem(popup, 2, "Remove group and sources", 0, 0);
            epopupmenu_additem(popup, 3, "Mute group", 0, 0);
            if (x->f_selected_group->getSubMute())
                epopupmenu_additem(popup, 4, "Unmute group", 0, 0);
            epopupmenu_popup(popup, pos, 0);
        }
        else if(x->f_selected_source)
        {
            t_epopup* popup = epopupmenu_create((t_eobj *)x, hoa_sym_source);
            epopupmenu_setfont(popup, &x->j_box.b_font);
            epopupmenu_additem(popup, 0, "Source Menu", 0, 1);
            epopupmenu_addseperator(popup);
            epopupmenu_additem(popup, 1, "Remove source", 0, 0);
            if(x->f_selected_source->getMute())
                epopupmenu_additem(popup, 2, "Unmute source", 0, 0);
            else
                epopupmenu_additem(popup, 2, "Mute source", 0, 0);
            epopupmenu_popup(popup, pos, 0);
        }
        else
        {
            t_epopup* popup = epopupmenu_create((t_eobj *)x, gensym("nothing"));
            epopupmenu_setfont(popup, &x->j_box.b_font);
            epopupmenu_additem(popup, 0, "Menu", 0, 1);
            epopupmenu_addseperator(popup);
            epopupmenu_additem(popup, 1, "Add source", 0, 0);
            epopupmenu_additem(popup, 2, "Clear all", 0, 0);
            epopupmenu_popup(popup, pos, 0);
        }
    }

    if(!x->f_selected_source && !x->f_selected_group)
    {
        x->f_rect_selection.x = pt.x;
        x->f_rect_selection.y = pt.y;
        x->f_rect_selection_exist = 1;
    }
}

void hoa_map_popup(t_hoa_map *x, t_symbol *s, long itemid)
{
	int causeOutput = 0;
	int causeRedraw = 0;
	int causeNotify = 0;

    if (s == NULL)
    {
        causeOutput = causeRedraw = causeNotify = 1;
    }
    else if(s == hoa_sym_group)
    {
        switch (itemid)
        {
            case 1:
            {
				t_atom av[3];
				atom_setlong(av, x->f_selected_group->getIndex());
				atom_setsym(av+1, hoa_sym_mute);
				atom_setlong(av+2, 1);
				outlet_list(x->f_out_groups, 0L, 3, av);
				x->f_manager->removeGroup(x->f_selected_group->getIndex());
				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
            case 2:
            {
				x->f_selected_group->setMute(true);
				hoa_map_output(x);
				hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
				x->f_manager->removeGroupWithSources(x->f_selected_group->getIndex());
				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
            case 3: // Mute group
            {
				x->f_selected_group->setMute(true);
				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
            case 4: // Unmute group
            {
				x->f_selected_group->setMute(false);
				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
        }
    }
    else if(s == hoa_sym_source)
    {
        switch (itemid)
        {
            case 1:
            {
				t_atom av[3];
				atom_setlong(av, x->f_selected_source->getIndex());
				atom_setsym(av+1, hoa_sym_mute);
				atom_setlong(av+2, 1);
				outlet_list(x->f_out_sources, 0L, 3, av);
				x->f_manager->removeSource(x->f_selected_source->getIndex());
				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
            case 2:
            {
                if(x->f_selected_source->getMute())
                    x->f_selected_source->setMute(false);
                else
                    x->f_selected_source->setMute(true);

				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
        }
    }
    else if(s ==gensym("nothing"))
    {
        switch (itemid)
        {
            case 1:
            {
                ulong index = 1;
                for (Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource() ; it ++)
                {
                    if (it->first != index)
                        break;
                    index ++;
                }
                x->f_manager->newSource(index);
;				causeOutput = causeRedraw = causeNotify = 1;
                break;
            }
            case 2:
            {
				hoa_map_clearAll(x);
				causeOutput = causeRedraw = causeNotify = 0;
                break;
            }
        }
    }

    if (causeRedraw)
	{
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
		ebox_redraw((t_ebox *)x);
		hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW);
	}

	if (causeNotify)
	{
		ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
		hoa_map_sendBindedMapUpdate(x, BMAP_NOTIFY);
	}

	if (causeOutput)
	{
		hoa_map_output(x);
		hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
	}
}

void hoa_map_mousedrag(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers)
{
	t_pt cursor;
    cursor.x = ((pt.x / x->rect.width * 2.) - 1.) / x->f_zoom_factor;
    cursor.y = ((-pt.y / x->rect.height * 2.) + 1.) / x->f_zoom_factor;

	int causeOutput = 0;
	int causeRedraw = 0;
	int causeNotify = 0;

	if (x->f_selected_source)
    {
        if(modifiers == EMOD_SHIFT)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                x->f_selected_source->setAzimuth(Math<float>::azimuth(cursor.x, cursor.y));
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                double source_radius = Math<float>::radius(x->f_selected_source->getAbscissa(), x->f_selected_source->getHeight());
                double mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));

                x->f_selected_source->setAbscissa(Math<float>::abscissa(source_radius, mouse_azimuth));
                x->f_selected_source->setHeight(Math<float>::ordinate(source_radius, mouse_azimuth));
            }
            else
            {
                double source_radius = Math<float>::radius(x->f_selected_source->getOrdinate(), x->f_selected_source->getHeight());
                double mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));

                x->f_selected_source->setOrdinate(Math<float>::abscissa(source_radius, mouse_azimuth));
                x->f_selected_source->setHeight(Math<float>::ordinate(source_radius, mouse_azimuth));
            }

            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_ALT)
        {
            x->f_selected_source->setRadius(Math<float>::radius(cursor.x, cursor.y));
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_CTRL)
        {
            if (x->f_coord_view == hoa_sym_view_xy || x->f_coord_view == hoa_sym_view_xz)
                x->f_selected_source->setAbscissa(cursor.x);
            else
                x->f_selected_source->setOrdinate(cursor.x);
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_CTRL+EMOD_SHIFT)
        {
            if (x->f_coord_view == hoa_sym_view_xy)
                x->f_selected_source->setOrdinate(cursor.y);
            else
                x->f_selected_source->setHeight(cursor.y);
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                x->f_selected_source->setCoordinatesCartesian(cursor.x, cursor.y);
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                x->f_selected_source->setCoordinatesCartesian(cursor.x, x->f_selected_source->getOrdinate(), cursor.y);
            }
            else
            {
                x->f_selected_source->setCoordinatesCartesian(x->f_selected_source->getAbscissa(), cursor.x, cursor.y);
            }
            causeOutput = causeRedraw = causeNotify = 1;
        }
    }
    else if (x->f_selected_group)
    {
        if(modifiers == EMOD_SHIFT)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                x->f_selected_group->setRelativeAzimuth(Math<float>::azimuth(cursor.x, cursor.y));
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                if (x->f_mouse_was_dragging)
                {
                    t_pt source_display;
                    double source_radius, source_azimuth, mouse_azimuth, mouse_azimuth_prev;
                    mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));
                    mouse_azimuth_prev = Math<float>::wrap_twopi(Math<float>::azimuth(x->f_cursor_position.x, x->f_cursor_position.y));

                    map<ulong, Source*>& sourcesOfGroup = x->f_selected_group->getSources();
                    for(Source::source_iterator it = sourcesOfGroup.begin() ; it != sourcesOfGroup.end() ; it ++)
                    {
                        source_display.x = it->second->getAbscissa();
                        source_display.y = it->second->getHeight();
                        source_radius = Math<float>::radius(source_display.x, source_display.y);
                        source_azimuth = Math<float>::azimuth(source_display.x, source_display.y);
                        source_azimuth += mouse_azimuth - mouse_azimuth_prev;

                        it->second->setAbscissa(Math<float>::abscissa(source_radius, source_azimuth));
                        it->second->setHeight(Math<float>::ordinate(source_radius, source_azimuth));
                    }
                }
            }
            else
            {
                if (x->f_mouse_was_dragging)
                {
                    t_pt source_display;
                    double source_radius, source_azimuth, mouse_azimuth, mouse_azimuth_prev;
                    mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));
                    mouse_azimuth_prev = Math<float>::wrap_twopi(Math<float>::azimuth(x->f_cursor_position.x, x->f_cursor_position.y));

                    map<ulong, Source*>& sourcesOfGroup = x->f_selected_group->getSources();
                    for(Source::source_iterator it = sourcesOfGroup.begin() ; it != sourcesOfGroup.end() ; it ++)
                    {
                        source_display.x = it->second->getOrdinate();
                        source_display.y = it->second->getHeight();
                        source_radius = Math<float>::radius(source_display.x, source_display.y);
                        source_azimuth = Math<float>::azimuth(source_display.x, source_display.y);
                        source_azimuth += mouse_azimuth - mouse_azimuth_prev;

                        it->second->setOrdinate(Math<float>::abscissa(source_radius, source_azimuth));
                        it->second->setHeight(Math<float>::ordinate(source_radius, source_azimuth));
                    }
                }
            }
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_ALT)
        {
            x->f_selected_group->setRelativeRadius(Math<float>::radius(cursor.x, cursor.y));
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_ALT+EMOD_SHIFT)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                x->f_selected_group->setRelativeRadius(Math<float>::radius(cursor.x, cursor.y));
                x->f_selected_group->setRelativeAzimuth(Math<float>::azimuth(cursor.x, cursor.y));
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                if (x->f_mouse_was_dragging)
                {
                    t_pt source_display;
                    double source_radius, source_azimuth, mouse_azimuth, mouse_azimuth_prev, mouse_radius, mouse_radius_prev;
                    mouse_radius = pd_clip_min(Math<float>::radius(cursor.x, cursor.y), 0);
                    mouse_radius_prev = pd_clip_min(Math<float>::radius(x->f_cursor_position.x, x->f_cursor_position.y), 0);
                    mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));
                    mouse_azimuth_prev = Math<float>::wrap_twopi(Math<float>::azimuth(x->f_cursor_position.x, x->f_cursor_position.y));

                    map<ulong, Source*>& sourcesOfGroup = x->f_selected_group->getSources();
                    for(Source::source_iterator it = sourcesOfGroup.begin() ; it != sourcesOfGroup.end() ; it ++)
                    {
                        source_display.x = it->second->getAbscissa();
                        source_display.y = it->second->getHeight();
                        source_radius = Math<float>::radius(source_display.x, source_display.y);
                        source_radius += mouse_radius - mouse_radius_prev;
                        source_azimuth = Math<float>::azimuth(source_display.x, source_display.y);
                        source_azimuth += mouse_azimuth - mouse_azimuth_prev;

                        it->second->setAbscissa(Math<float>::abscissa(source_radius, source_azimuth));
                        it->second->setHeight(Math<float>::ordinate(source_radius, source_azimuth));
                    }
                }
            }
            else
            {
                if (x->f_mouse_was_dragging)
                {
                    t_pt source_display;
                    double source_radius, source_azimuth, mouse_azimuth, mouse_azimuth_prev, mouse_radius, mouse_radius_prev;
                    mouse_radius = pd_clip_min(Math<float>::radius(cursor.x, cursor.y), 0);
                    mouse_radius_prev = pd_clip_min(Math<float>::radius(x->f_cursor_position.x, x->f_cursor_position.y), 0);
                    mouse_azimuth = Math<float>::wrap_twopi(Math<float>::azimuth(cursor.x, cursor.y));
                    mouse_azimuth_prev = Math<float>::wrap_twopi(Math<float>::azimuth(x->f_cursor_position.x, x->f_cursor_position.y));

                    map<ulong, Source*>& sourcesOfGroup = x->f_selected_group->getSources();
                    for(Source::source_iterator it = sourcesOfGroup.begin() ; it != sourcesOfGroup.end() ; it ++)
                    {
                        source_display.x = it->second->getOrdinate();
                        source_display.y = it->second->getHeight();
                        source_radius = Math<float>::radius(source_display.x, source_display.y);
                        source_radius += mouse_radius - mouse_radius_prev;
                        source_azimuth = Math<float>::azimuth(source_display.x, source_display.y);
                        source_azimuth += mouse_azimuth - mouse_azimuth_prev;

                        it->second->setOrdinate(Math<float>::abscissa(source_radius, source_azimuth));
                        it->second->setHeight(Math<float>::ordinate(source_radius, source_azimuth));
                    }
                }
            }
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_CTRL)
        {
            if (x->f_coord_view == hoa_sym_view_xy || x->f_coord_view == hoa_sym_view_xz)
                x->f_selected_group->setAbscissa(cursor.x);
            else
                x->f_selected_group->setOrdinate(cursor.x);
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else if(modifiers == EMOD_CTRL+EMOD_SHIFT)
        {
            if (x->f_coord_view == hoa_sym_view_xy)
                x->f_selected_group->setOrdinate(cursor.y);
            else
                x->f_selected_group->setHeight(cursor.y);
            causeOutput = causeRedraw = causeNotify = 1;
        }
        else
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                x->f_selected_group->setCoordinatesCartesian(cursor.x, cursor.y);
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                x->f_selected_group->setAbscissa(cursor.x);
                x->f_selected_group->setHeight(cursor.y);
            }
            else
            {
                x->f_selected_group->setOrdinate(cursor.x);
                x->f_selected_group->setHeight(cursor.y);
            }
            causeOutput = causeRedraw = causeNotify = 1;
        }
    }
    else
    {
		x->f_rect_selection.width = pt.x - x->f_rect_selection.x;
		x->f_rect_selection.height = pt.y - x->f_rect_selection.y;
		ebox_invalidate_layer((t_ebox *)x, gensym("rect_selection_layer"));
		ebox_redraw((t_ebox *)x);
		causeOutput = causeRedraw = causeNotify = 0;
    }

	x->f_cursor_position.x = cursor.x;
    x->f_cursor_position.y = cursor.y;
	x->f_mouse_was_dragging = 1;

	if (causeNotify)
	{
		ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
		hoa_map_sendBindedMapUpdate(x, BMAP_NOTIFY);
	}

	if (causeRedraw)
	{
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
		ebox_redraw((t_ebox *)x);
		hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW);
	}

	if (causeOutput)
	{
		hoa_map_output(x);
		hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
	}
}

void hoa_map_mouseup(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers)
{
	x->f_mouse_was_dragging = 0;

	t_pt screen_source_coord;

	int causeOutput = 0;
	int causeRedraw = 0;
	int causeNotify = 0;

    if(x->f_rect_selection_exist)
    {
        ulong indexOfNewGroup = 1;
        for (Source::group_iterator it = x->f_manager->getFirstGroup() ; it != x->f_manager->getLastGroup() ; it ++)
        {
            if (it->first != indexOfNewGroup)
                break;
            indexOfNewGroup ++;
        }

        double x1 = ((x->f_rect_selection.x / x->rect.width * 2.) - 1.) / x->f_zoom_factor;
        double x2 = (((x->f_rect_selection.x + x->f_rect_selection.width) / x->rect.width * 2.) - 1.) / x->f_zoom_factor;
        double y1 = ((-x->f_rect_selection.y / x->rect.height * 2.) + 1.) / x->f_zoom_factor;
        double y2 = (((-x->f_rect_selection.y - x->f_rect_selection.height) / x->rect.height * 2.) + 1.) / x->f_zoom_factor;

        bool newGroupCreated = false;
        Source::Group* tmp = x->f_manager->getGroup(indexOfNewGroup);
        if (!tmp)
        {
            tmp = x->f_manager->createGroup(indexOfNewGroup);
            newGroupCreated = true;
        }
        for(Source::source_iterator it = x->f_manager->getFirstSource() ; it != x->f_manager->getLastSource() ; it ++)
        {
            if(x->f_coord_view == hoa_sym_view_xy)
            {
                screen_source_coord.x = it->second->getAbscissa();
                screen_source_coord.y = it->second->getOrdinate();
            }
            else if(x->f_coord_view == hoa_sym_view_xz)
            {
                screen_source_coord.x = it->second->getAbscissa();
                screen_source_coord.y = it->second->getHeight();
            }
            else
            {
                screen_source_coord.x = it->second->getOrdinate();
                screen_source_coord.y = it->second->getHeight();
            }

            if(((screen_source_coord.x > x1 && screen_source_coord.x < x2) || (screen_source_coord.x < x1 && screen_source_coord.x > x2)) && ((screen_source_coord.y > y1 && screen_source_coord.y < y2) || (screen_source_coord.y < y1 && screen_source_coord.y > y2)))
            {
                tmp->addSource(it->second);
                causeOutput = causeRedraw = causeNotify = 1;
            }
        }

        if (newGroupCreated)
        {
            if (!x->f_manager->addGroup(tmp))
            {
                delete tmp;
            }
        }
    }

    x->f_rect_selection_exist = x->f_rect_selection.width = x->f_rect_selection.height = 0;

	ebox_invalidate_layer((t_ebox *)x, hoa_sym_rect_selection_layer);
	ebox_redraw((t_ebox *)x);

	if (causeNotify)
	{
		ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
		hoa_map_sendBindedMapUpdate(x, BMAP_NOTIFY);
	}

	if (causeRedraw)
	{
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
		ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
		ebox_redraw((t_ebox *)x);
		hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW);
	}

	if (causeOutput)
	{
		hoa_map_output(x);
		hoa_map_sendBindedMapUpdate(x, BMAP_OUTPUT);
	}
}

void hoa_map_mousewheel(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers, double x_inc, double y_inc)
{
    if(modifiers == EMOD_ALT)
    {
		double newZoom = x->f_zoom_factor + y_inc / 100.;
        x->f_zoom_factor = pd_clip_minmax(newZoom, MIN_ZOOM, MAX_ZOOM);

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
        ebox_redraw((t_ebox *)x);
	}
}

void hoa_map_mousemove(t_hoa_map *x, t_object *patcherview, t_pt pt, long modifiers)
{
	hoa_map_isElementSelected(x, pt);

    if( x->f_selected_source ||  x->f_selected_group)
        ebox_set_cursor((t_ebox *)x, 4);
    else
        ebox_set_cursor((t_ebox *)x, 1);

    ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
    ebox_redraw((t_ebox *)x);
}

t_symbol* hoa_map_stringFormat(const char *s)
{
    char desc[MAXPDSTRING];
    char str[MAXPDSTRING];
    char *pch;

    sprintf(str, "%s", s);
    pch = strtok(str, " ,.-");
    sprintf(desc, "%s", pch);
    pch = strtok(NULL, " ,.-");
    while(pch != NULL)
    {
        strcat(desc, "_");
        strcat(desc, pch);
        pch = strtok(NULL, " ,.-");
    }

    return gensym(desc);
}

void hoa_map_preset(t_hoa_map *x, t_binbuf *d)
{
    binbuf_addv(d, "s", hoa_sym_sources_preset);
    for(Source::source_iterator it = x->f_manager->getFirstSource(); it != x->f_manager->getLastSource(); it++)
    {
        binbuf_addv(d, "sffff", hoa_sym_source, (float)it->first,
                    (float)it->second->getAbscissa(),
                    (float)it->second->getOrdinate(),
                    (float)it->second->getHeight());

        binbuf_addv(d, "fffff", (float)it->second->getMute(),
                    (float)it->second->getColor()[0],
                    (float)it->second->getColor()[1],
                    (float)it->second->getColor()[2],
                    (float)it->second->getColor()[3]);

        if(it->second->getDescription().size())
            binbuf_addv(d, "s", hoa_map_stringFormat(it->second->getDescription().c_str()));
        else
            binbuf_addv(d, "s", hoa_sym_null);
    }

    for(Source::group_iterator it = x->f_manager->getFirstGroup(); it != x->f_manager->getLastGroup(); it++)
    {
        binbuf_addv(d, "sf", hoa_sym_group, (float)it->first);
        binbuf_addv(d, "f", (float)it->second->getNumberOfSources());

        map<ulong, Source*>& sourcesOfGroup = it->second->getSources();
        for(Source::source_iterator ti = sourcesOfGroup.begin() ; ti != sourcesOfGroup.end() ; ti++)
        {
            binbuf_addv(d, "f", (float)ti->first);
        }

        binbuf_addv(d, "ffff",
                    (float)it->second->getColor()[0],
                    (float)it->second->getColor()[1],
                    (float)it->second->getColor()[2],
                    (float)it->second->getColor()[3]);

        if(it->second->getDescription().size())
            binbuf_addv(d, "s", hoa_map_stringFormat(it->second->getDescription().c_str()));
        else
            binbuf_addv(d, "s", hoa_sym_null);
    }
}

void hoa_map_sourcesPreset(t_hoa_map *x, t_symbol *s, short ac, t_atom *av)
{
    int index;
    int nsources;
    if(ac && av)
    {
        x->f_manager->clear();
        short i = 0;
        while(i < ac)
        {
            if(atom_gettype(av+i) == A_SYM && atom_getsym(av+i) == hoa_sym_source
               && atom_gettype(av+i+1) == A_FLOAT
               && atom_gettype(av+i+2) == A_FLOAT
               && atom_gettype(av+i+3) == A_FLOAT
               && atom_gettype(av+i+4) == A_FLOAT)
            {
                index = atom_getlong(av+i+1);
                Source* tmp = x->f_manager->newSource(index);
                tmp->setCoordinatesCartesian(atom_getfloat(av+i+2), atom_getfloat(av+i+3), atom_getfloat(av+i+4));

                if(atom_gettype(av+i+5) == A_FLOAT && atom_getfloat(av+i+5) == 0)
                    tmp->setMute(false);
                else
                    tmp->setMute(true);

                if(atom_gettype(av+i+6) == A_FLOAT
                   && atom_gettype(av+i+7) == A_FLOAT
                   && atom_gettype(av+i+8) == A_FLOAT
                   && atom_gettype(av+i+9) == A_FLOAT)
                    tmp->setColor(atom_getfloat(av+i+6), atom_getfloat(av+i+7), atom_getfloat(av+i+8), atom_getfloat(av+i+9));

                if(atom_gettype(av+i+10) == A_SYM && atom_getsym(av+i+10) != gensym("(null)"))
                    tmp->setDescription(hoa_map_stringFormat(atom_getsym(av+i+10)->s_name)->s_name);
                else
                    tmp->setDescription("");

                i += 11;
            }
            else if(atom_gettype(av+i) == A_SYM && atom_getsym(av+i) == hoa_sym_group
               && atom_gettype(av+i+1) == A_FLOAT
               && atom_gettype(av+i+2) == A_FLOAT)
            {
                index = atom_getlong(av+i+1);
                nsources = atom_getlong(av+i+2);

                bool newGroupCreated = false;
                Source::Group* tmp = x->f_manager->getGroup(index);
                if (!tmp)
                {
                    tmp = x->f_manager->createGroup(index);
                    newGroupCreated = true;
                }

                for(int j = 0; j < nsources; j++)
                {
                    if(ac > i+3+j && atom_gettype(av+i+3+j) == A_FLOAT)
                    {
                        Source* src = x->f_manager->getSource(atom_getlong(av+i+3+j));
                        if (src)
                            tmp->addSource(src);
                    }
                }

                if(ac > i+6+nsources
                   && atom_gettype(av+i+3+nsources) == A_FLOAT
                   && atom_gettype(av+i+4+nsources) == A_FLOAT
                   && atom_gettype(av+i+5+nsources) == A_FLOAT
                   && atom_gettype(av+i+6+nsources) == A_FLOAT)
                    tmp->setColor(atom_getfloat(av+i+3+nsources), atom_getfloat(av+i+4+nsources), atom_getfloat(av+i+5+nsources), atom_getfloat(av+i+6+nsources));

                if(ac > i+7+nsources && atom_gettype(av+i+7+nsources) == A_SYM && atom_getsym(av+i+7+nsources) != hoa_sym_null)
                    tmp->setDescription(hoa_map_stringFormat(atom_getsym(av+i+7+nsources)->s_name)->s_name);
                else
                    tmp->setDescription("");

                if (newGroupCreated)
                {
                    if (!x->f_manager->addGroup(tmp))
                    {
                        delete tmp;
                    }
                }
                i += (7+nsources);
            }
            else
                i ++;
        }
    }

    ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
    ebox_redraw((t_ebox *)x);
    hoa_map_output(x);
    hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW | BMAP_OUTPUT | BMAP_NOTIFY);
}

void hoa_map_interpolate(t_hoa_map *x, short ac, t_atom *av, short ac2, t_atom* av2, t_atom ratio)
{
    int index;
    float exist2;
    int nsources;
    float theta = atom_getfloat(&ratio);

    if(ac && av)
    {
        x->f_manager->clear();
        short i = 0;
        while(i < ac)
        {
            if(atom_gettype(av+i) == A_SYM && atom_getsym(av+i) == hoa_sym_source
               && atom_gettype(av+i+1) == A_FLOAT
               && atom_gettype(av+i+2) == A_FLOAT
               && atom_gettype(av+i+3) == A_FLOAT
               && atom_gettype(av+i+4) == A_FLOAT)
            {
                index = atom_getlong(av+i+1);
                exist2 = 0;
                Source* tmp = x->f_manager->newSource(index);
                for(int j = 0; j < ac2; j++)
                {
                    if(ac2 > j+10)
                    {
                        if(atom_gettype(av2+j) == A_SYM && atom_getsym(av2+j) == hoa_sym_source
                           && atom_gettype(av2+j+1) == A_FLOAT
                           && atom_gettype(av2+j+2) == A_FLOAT
                           && atom_gettype(av2+j+3) == A_FLOAT
                           && atom_gettype(av2+j+4) == A_FLOAT
                           && index == atom_getlong(av2+(ulong)j+1))
                        {
                            exist2 = 1;
                            float abscissa = atom_getfloat(av+i+2) * (1.f - theta) + atom_getfloat(av2+j+2) * theta;
                            float ordinate = atom_getfloat(av+i+3) * (1.f - theta) + atom_getfloat(av2+j+3) * theta;
                            float height = atom_getfloat(av+i+4) * (1.f - theta) + atom_getfloat(av2+j+4) * theta;
                            tmp->setCoordinatesCartesian(abscissa, ordinate, height);
                            break;
                        }
                    }
                }
                if(!exist2)
                {
                    tmp->setCoordinatesCartesian(atom_getfloat(av+i+2), atom_getfloat(av+i+3), atom_getfloat(av+i+4));
                }
                if(atom_gettype(av+i+5) == A_FLOAT && atom_getfloat(av+i+5) == 0)
                    tmp->setMute(false);
                else
                    tmp->setMute(true);

                if(atom_gettype(av+i+6) == A_FLOAT
                   && atom_gettype(av+i+7) == A_FLOAT
                   && atom_gettype(av+i+8) == A_FLOAT
                   && atom_gettype(av+i+9) == A_FLOAT)
                    tmp->setColor(atom_getfloat(av+i+6), atom_getfloat(av+i+7), atom_getfloat(av+i+8), atom_getfloat(av+i+9));

                if(atom_gettype(av+i+10) == A_SYM && atom_getsym(av+i+10) != hoa_sym_null)
                    tmp->setDescription(hoa_map_stringFormat(atom_getsym(av+i+10)->s_name)->s_name);
                else
                    tmp->setDescription("");
                i += 11;
            }
            else if(atom_gettype(av+i) == A_SYM && atom_getsym(av+i) == hoa_sym_group
               && atom_gettype(av+i+1) == A_FLOAT
               && atom_gettype(av+i+2) == A_FLOAT)
            {
                index = atom_getlong(av+i+1);
                nsources = atom_getlong(av+i+2);

                bool newGroupCreated = false;
                Source::Group* tmp = x->f_manager->getGroup(index);
                if (!tmp)
                {
                    tmp = x->f_manager->createGroup(index);
                    newGroupCreated = true;
                }

                for(int j = 0; j < nsources; j++)
                {
                    if(ac > i+3+j && atom_gettype(av+i+3+j) == A_FLOAT)
                    {
                        Source* src = x->f_manager->getSource(atom_getlong(av+i+3+j));
                        if (src)
                            tmp->addSource(src);
                    }
                }

                if(ac > i+6+nsources && atom_gettype(av+i+3+nsources) == A_FLOAT
                   && atom_gettype(av+i+4+nsources) == A_FLOAT
                   && atom_gettype(av+i+5+nsources) == A_FLOAT
                   && atom_gettype(av+i+6+nsources) == A_FLOAT)
                    tmp->setColor(atom_getfloat(av+i+3+nsources), atom_getfloat(av+i+4+nsources), atom_getfloat(av+i+5+nsources), atom_getfloat(av+i+6+nsources));

                if(ac > i+7+nsources && atom_gettype(av+i+7+nsources) == A_SYM && atom_getsym(av+i+7+nsources) != hoa_sym_null)
                    tmp->setDescription(hoa_map_stringFormat(atom_getsym(av+i+7+nsources)->s_name)->s_name);
                else
                    tmp->setDescription("");

                if (newGroupCreated)
                {
                    if (!x->f_manager->addGroup(tmp))
                    {
                        delete tmp;
                    }
                }
                i += (7+nsources);
            }
            else
                i ++;
        }
    }

    ebox_notify((t_ebox *)x, NULL, hoa_sym_modified, NULL, NULL);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_sources_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_groups_layer);
    ebox_redraw((t_ebox *)x);
    hoa_map_output(x);
    hoa_map_sendBindedMapUpdate(x, BMAP_REDRAW | BMAP_OUTPUT | BMAP_NOTIFY);
}

