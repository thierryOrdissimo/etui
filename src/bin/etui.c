/* Etui - Multi-document rendering library using the EFL
 * Copyright (C) 2013 Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>

#include <Eina.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>

#include <Etui.h>

static Etui_Rotation rotation = ETUI_ROTATION_0;
static float scale = 1.0f;

static const Ecore_Getopt _etui_options =
{
    "etui",
    "%prog [options] <filename>",
    PACKAGE_VERSION,
    "(C) 2013 Vincent Torri",
    "AGPL",
    "EFL-based multi-document viewer.",
    1,
    {
        ECORE_GETOPT_STORE_STR('e', "engine", "ecore-evas engine to use"),
        ECORE_GETOPT_CALLBACK_NOARGS('E', "list-engines", "list ecore-evas engines",
                                     ecore_getopt_callback_ecore_evas_list_engines, NULL),
        ECORE_GETOPT_CALLBACK_ARGS('g', "geometry", "geometry to use in x:y:w:h form.", "X:Y:W:H",
                                   ecore_getopt_callback_geometry_parse, NULL),
        ECORE_GETOPT_VERSION('V', "version"),
        ECORE_GETOPT_COPYRIGHT('R', "copyright"),
        ECORE_GETOPT_LICENSE('L', "license"),
        ECORE_GETOPT_HELP('h', "help"),
        ECORE_GETOPT_SENTINEL
    }
};

static Eina_Bool
_etui_signal_exit(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *ev EINA_UNUSED)
{
    printf("exiting signal...\n");
    ecore_main_loop_quit();
    return EINA_TRUE;
}

static Eina_Bool
_etui_key_down(void *data, int type EINA_UNUSED, void *event)
{
    Ecore_Event_Key *ev;

    ev = event;

    if (ev->key)
    {
        if ((!strcmp(ev->key, "Right")))
            etui_object_page_set(data, etui_object_page_get(data) + 1);
        else if (!strcmp(ev->key, "Left"))
            etui_object_page_set(data, etui_object_page_get(data) - 1);
        else if (!strcmp(ev->key, "Up"))
        {
            scale *= M_SQRT2;
            etui_object_page_scale_set(data, scale, scale);
        }
        else if (!strcmp(ev->key, "Down"))
        {
            int w,h;
            scale *= M_SQRT1_2;
            etui_object_page_scale_set(data, scale, scale);
            evas_object_geometry_get(data, NULL, NULL, &w, &h);
            printf(" bin ** %dx%d\n", w, h);
        }
        else if (!strcmp(ev->key, "less"))
        {
            rotation += 90;
            if (rotation > ETUI_ROTATION_270)
                rotation = ETUI_ROTATION_0;
            etui_object_page_rotation_set(data, rotation);
        }
        else if (!strcmp(ev->key, "greater"))
        {
            rotation -= 90;
            if (rotation < ETUI_ROTATION_0)
                rotation = ETUI_ROTATION_270;
            etui_object_page_rotation_set(data, rotation);
        }
        else if (!strcmp(ev->key, "q"))
            ecore_main_loop_quit();
    }

    return ECORE_CALLBACK_PASS_ON;
}

static void _etui_delete_request_cb(Ecore_Evas *ee EINA_UNUSED)
{
    ecore_main_loop_quit();
}

static void _etui_toc_display(const Eina_Array *items, int indent)
{
    Etui_Toc_Item *item;
    Eina_Array_Iterator iter;
    unsigned int i;

    if (!items || (eina_array_count(items) == 0))
        return;

    EINA_ARRAY_ITER_NEXT(items, i, item, iter)
    {
        int j;

        printf("$");
        for (j = 0; j < indent; j++) printf (" ");
        printf ("%s (%d): %d\n",
                item->title,
                (item->kind == ETUI_LINK_KIND_GOTO) ? item->dest.goto_.page : -1,
                item->kind);
        _etui_toc_display(item->child, indent + 2);
    }
}

int main(int argc, char *argv[])
{
    char buf[4096];
    Eina_Rectangle geometry = {0, 0, 480, 640};
    char *engine = NULL;
    unsigned char engines_listed = 0;
    unsigned char help = 0;
    Ecore_Getopt_Value values[] = {
        ECORE_GETOPT_VALUE_STR(engine),
        ECORE_GETOPT_VALUE_BOOL(engines_listed),
        ECORE_GETOPT_VALUE_PTR_CAST(geometry),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_NONE
    };
    Ecore_Evas *ee;
    Ecore_Event_Handler *handler;
    Evas *evas;
    Evas_Object *o;
    const char *prop;
    int args;
    int w;
    int h;

    if (!ecore_evas_init())
    {
        printf("can not init ecore_evas\n");
        return -1;
    }

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _etui_signal_exit, NULL);

    ecore_app_args_set(argc, (const char **)argv);
    args = ecore_getopt_parse(&_etui_options, values, argc, argv);
    if (args < 0)
        goto shutdown_ecore_evas;
    else if (help)
        goto shutdown_ecore_evas;
    else if (engines_listed)
        goto shutdown_ecore_evas;
    else if (args == argc)
    {
        printf("must provide at least one file to display\n");
        goto shutdown_ecore_evas;
    }

    if ((geometry.w == 0) || (geometry.h == 0))
    {
        if (geometry.w == 0) geometry.w = 480;
        if (geometry.h == 0) geometry.h = 640;
    }

    ee = ecore_evas_new(NULL, geometry.x, geometry.y, 0, 0, NULL);
    if (!ee)
    {
        printf("can not create Ecore_Evas\n");
        goto shutdown_ecore_evas;
    }

    ecore_evas_callback_delete_request_set(ee, _etui_delete_request_cb);
    evas = ecore_evas_get(ee);

    snprintf(buf, sizeof(buf), "Etui - %s\n", argv[args]);
    ecore_evas_title_set(ee, buf);
    ecore_evas_name_class_set(ee, "etui", "main");

    if (!etui_init())
        goto shutdown_ecore_evas;

    o = etui_object_add(evas);
    if (!etui_object_file_open(o, argv[args]))
    {
        printf("can not open file %s\n", argv[args]);
        goto shutdown_etui;
    }

    etui_object_page_set(o, 0);
    evas_object_geometry_get(o, NULL, NULL, &w, &h);
    printf(" $$$$ size : %dx%d\n", w, h);
    evas_object_move(o, 0, 0);
    evas_object_focus_set(o, EINA_TRUE);
    evas_object_show(o);
    ecore_evas_object_associate(ee, o, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

    handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _etui_key_down, o);

    printf("module : %s\n", etui_object_module_name_get(o));
    prop = etui_object_title_get(o);
    printf("title : %s\n", prop);
    printf("pages : %d\n", etui_object_document_pages_count(o));
    printf("size : %dx%d\n", w, h);

    if (strcmp("pdf", etui_object_module_name_get(o)) == 0)
    {
        const Etui_Module_Pdf_Info *info;

        info = etui_object_info_get(o);
        if (info)
        {
            printf("Author           : %s\n", info->author);
            printf("Subject          : %s\n", info->subject);
            printf("Keywords         : %s\n", info->keywords);
            printf("Creator          : %s\n", info->creator);
            printf("Producer         : %s\n", info->producer);
            printf("Creation date    : %s\n", info->creation_date);
            printf("Modification date: %s\n", info->modification_date);
            printf("Encryption       : %s\n", info->encryption);
        }
    }

    if (strcmp("cb", etui_object_module_name_get(o)) == 0)
    {
        const Etui_Module_Cb_Info *info;

        info = etui_object_info_get(o);
        if (info)
        {
            char *type;

            switch (info->type)
            {
                case ETUI_CB_CBZ:
                    type = "ebook CBZ";
                    break;
                case ETUI_CB_CBR:
                    type = "ebook CBR";
                    break;
                case ETUI_CB_CBA:
                    type = "ebook CBA";
                    break;
                case ETUI_CB_CB7:
                    type = "ebook CB7";
                    break;
                case ETUI_CB_CBT:
                    type = "ebook CBT";
                    break;
            }

            printf("type : %s\n", type);
        }
    }

    if (strcmp("djvu", etui_object_module_name_get(o)) == 0)
    {
        const Etui_Module_Djvu_Info *info;

        info = etui_object_info_get(o);
        if (info)
        {
            char *type;

            switch (info->page_type)
            {
                case ETUI_DJVU_PAGE_TYPE_UNKNOWN:
                    type = "Unknown";
                    break;
                case ETUI_DJVU_PAGE_TYPE_BITONAL:
                    type = "Bitonal";
                    break;
                case ETUI_DJVU_PAGE_TYPE_PHOTO:
                    type = "Photo";
                    break;
                case ETUI_DJVU_PAGE_TYPE_COMPOUND:
                    type = "Compound";
                    break;
                default:
                    type = "Unknown";
                    break;
            }

            printf("DPI : %d\n", info->page_dpi);
            printf("Gamma : %f\n", info->page_gamma);
            printf("type : %s\n", type);
        }
    }

    /* printf("\n"); */

    _etui_toc_display(etui_object_toc_get(o), 0);

#if 0
    {
        char *str;
        Eina_Rectangle rect = { 209, 680, 116, 24 };
        str = etui_object_page_text_extract(o, &rect);
        printf("text : \n**%s**\n", str);
        free(str);
    }

    {
        Eina_Array *rects;
        Eina_Array_Iterator iterator;
        unsigned int i;
        Eina_Rectangle *r;

        printf("text 'options'\n");
        rects = etui_object_page_text_find(o, "mott");
        if (!rects)
            printf("pas de rects\n");
        else
        {
            EINA_ARRAY_ITER_NEXT(rects, i, r, iterator)
                printf(" * %dx%d %dx%d\n", r->x, r->y, r->w, r->h);
            EINA_ARRAY_ITER_NEXT(rects, i, r, iterator)
                free(r);
        }

    }

    o = evas_object_rectangle_add(evas);
    evas_object_color_set(o, 128, 128, 0, 128);
    evas_object_move(o, 209, 680);
    evas_object_resize(o, 116, 24);
    evas_object_show(o);
#endif

    printf(" $$$$ size : %dx%d\n", w, h);
    ecore_evas_resize(ee, w, h);
    ecore_evas_show(ee);


    ecore_main_loop_begin();

    ecore_event_handler_del(handler);

    etui_shutdown();
    ecore_evas_shutdown();

    return 0;

  shutdown_etui:
    etui_shutdown();
  shutdown_ecore_evas:
    ecore_evas_shutdown();

    return -1;
}
