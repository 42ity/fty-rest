/*  =========================================================================
    web_src_http_proxy - root object

    Copyright (C) 2014 - 2018 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#ifndef WEB_SRC_HTTP_PROXY_H_INCLUDED
#define WEB_SRC_HTTP_PROXY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new web_src_http_proxy
FTY_REST_PRIVATE web_src_http_proxy_t *
    web_src_http_proxy_new (void);

//  Destroy the web_src_http_proxy
FTY_REST_PRIVATE void
    web_src_http_proxy_destroy (web_src_http_proxy_t **self_p);


//  @end

#ifdef __cplusplus
}
#endif

#endif
