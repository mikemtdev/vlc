/*****************************************************************************
 * filter.h
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
 * Copyright (C) 2020 Videolabs
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_GL_FILTER_H
#define VLC_GL_FILTER_H

#include <vlc_tick.h>

#include "picture.h"

struct vlc_gl_filter;

struct vlc_gl_tex_size {
    unsigned width;
    unsigned height;
};

struct vlc_gl_input_meta {
    vlc_tick_t pts;
    unsigned plane;
};

typedef int
vlc_gl_filter_open_fn(struct vlc_gl_filter *filter,
                      const config_chain_t *config,
                      const struct vlc_gl_format *glfmt,
                      struct vlc_gl_tex_size *size_out);

#define set_callback_opengl_filter(open) \
    { \
        vlc_gl_filter_open_fn *fn = open; \
        (void) fn; \
        set_callback(fn); \
    }

struct vlc_gl_filter_ops {
    /**
     * Draw the result of the filter to the current framebuffer
     */
    int (*draw)(struct vlc_gl_filter *filter, const struct vlc_gl_picture *pic,
                const struct vlc_gl_input_meta *meta);

    /**
     * Free filter resources
     */
    void (*close)(struct vlc_gl_filter *filter);
};

/**
 * OpenGL filter, in charge of a rendering pass.
 */
struct vlc_gl_filter {
    vlc_object_t obj;
    module_t *module;

    struct vlc_gl_t *gl;
    const struct vlc_gl_api *api;
    const struct vlc_gl_format *glfmt_in;

    struct {
        /**
         * An OpenGL filter may either operate on the input RGBA picture, or on
         * individual input planes (without chroma conversion) separately.
         *
         * In practice, this is useful for deinterlace filters.
         *
         * This flag must be set by the filter module (default is false).
         */
        bool filter_planes;

        /**
         * A blend filter draws over the input picture (without reading it).
         *
         * Meaningless if filter_planes is true.
         *
         * This flag must be set by the filter module (default is false).
         */
        bool blend;

        /**
         * Request MSAA level.
         *
         * This value must be set by the filter module (default is 0, which
         * means disabled).
         *
         * Meaningless if filter_planes is true.
         *
         * The actual MSAA level may be overwritten to 0 if multisampling is
         * not supported, or to a higher value if another filter rendering on
         * the same framebuffer requested a higher MSAA level.
         */
        unsigned msaa_level;
    } config;

    const struct vlc_gl_filter_ops *ops;
    void *sys;
};

#endif
