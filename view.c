/*
 * Cage: A Wayland kiosk.
 *
 * Copyright (C) 2018-2021 Jente Hidskes
 *
 * See the LICENSE file accompanying this file.
 */

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "output.h"
#include "poc_layout.h"
#include "seat.h"
#include "server.h"
#include "view.h"
#if CAGE_HAS_XWAYLAND
#include "xwayland.h"
#endif

void
view_configure_poc_layout(struct cg_server *server)
{
	const char *value = getenv("CAGE_LINGUUM_BROWSER_WIDTH");
	int width;

	if (!value || !*value) {
		server->poc_browser_width = 0;
		return;
	}

	if (!cg_poc_layout_parse_width(value, &width)) {
		wlr_log(WLR_ERROR, "Ignoring invalid CAGE_LINGUUM_BROWSER_WIDTH=%s", value);
		server->poc_browser_width = 0;
		return;
	}

	server->poc_browser_width = width;
}

bool
view_set_poc_browser_width(struct cg_server *server, int width)
{
	char value[16];
	int parsed_width;

	if (snprintf(value, sizeof(value), "%d", width) < 0 || !cg_poc_layout_parse_width(value, &parsed_width)) {
		return false;
	}

	if (server->poc_browser_width == parsed_width) {
		return true;
	}

	server->poc_browser_width = parsed_width;
	view_position_all(server);
	wlr_log(WLR_INFO, "POC browser width changed to %d", parsed_width);
	return true;
}

char *
view_get_title(struct cg_view *view)
{
	const char *title = view->impl->get_title(view);
	if (!title) {
		return NULL;
	}
	return strndup(title, strlen(title));
}

static enum cg_poc_surface_role
view_detect_poc_role(struct cg_view *view)
{
	char *title = view_get_title(view);
	enum cg_poc_surface_role role = cg_poc_layout_classify_title(view->server->poc_browser_width > 0, title);

	free(title);
	return role;
}

void
view_update_poc_role(struct cg_view *view)
{
	enum cg_poc_surface_role previous = view->poc_role;
	enum cg_poc_surface_role next = view_detect_poc_role(view);

	view->poc_role = next;
	if (next != previous) {
		char *title = view_get_title(view);
		wlr_log(WLR_INFO, "POC view role changed from %d to %d (title=%s)", previous, next, title ? title : "");
		free(title);
	}

	if (view->scene_tree) {
		view_position(view);
	}
}

bool
view_is_primary(struct cg_view *view)
{
	return view->impl->is_primary(view);
}

bool
view_is_transient_for(struct cg_view *child, struct cg_view *parent)
{
	return child->impl->is_transient_for(child, parent);
}

void
view_activate(struct cg_view *view, bool activate)
{
	view->impl->activate(view, activate);
	wlr_foreign_toplevel_handle_v1_set_activated(view->foreign_toplevel_handle, activate);
}

static bool
view_extends_output_layout(struct cg_view *view, struct wlr_box *layout_box)
{
	int width, height;
	view->impl->get_geometry(view, &width, &height);

	return (layout_box->height < height || layout_box->width < width);
}

static void
view_maximize(struct cg_view *view, struct wlr_box *layout_box)
{
	view->lx = layout_box->x;
	view->ly = layout_box->y;

	if (view->scene_tree) {
		wlr_scene_node_set_position(&view->scene_tree->node, view->lx, view->ly);
	}

	view->impl->maximize(view, layout_box->width, layout_box->height);
}

static void
view_center(struct cg_view *view, struct wlr_box *layout_box)
{
	int width, height;
	view->impl->get_geometry(view, &width, &height);

	view->lx = (layout_box->width - width) / 2;
	view->ly = (layout_box->height - height) / 2;

	if (view->scene_tree) {
		wlr_scene_node_set_position(&view->scene_tree->node, view->lx, view->ly);
	}
}

void
view_position(struct cg_view *view)
{
	struct wlr_box layout_box;
	wlr_output_layout_get_box(view->server->output_layout, NULL, &layout_box);
	struct cg_poc_rect output = {
		.x = layout_box.x,
		.y = layout_box.y,
		.width = layout_box.width,
		.height = layout_box.height,
	};
	struct cg_poc_rect target;

	if (cg_poc_layout_rect(output, view->server->poc_browser_width, view->poc_role, &target)) {
		struct wlr_box target_box = {
			.x = target.x,
			.y = target.y,
			.width = target.width,
			.height = target.height,
		};
		view_maximize(view, &target_box);
		return;
	}

	if (view_is_primary(view) || view_extends_output_layout(view, &layout_box)) {
		view_maximize(view, &layout_box);
	} else {
		view_center(view, &layout_box);
	}
}

void
view_position_all(struct cg_server *server)
{
	struct cg_view *view;
	wl_list_for_each (view, &server->views, link) {
		view_position(view);
	}
}

void
view_unmap(struct cg_view *view)
{
	wl_list_remove(&view->link);

	wl_list_remove(&view->request_activate.link);
	wl_list_remove(&view->request_close.link);
	wlr_foreign_toplevel_handle_v1_destroy(view->foreign_toplevel_handle);
	view->foreign_toplevel_handle = NULL;

	wlr_scene_node_destroy(&view->scene_tree->node);

	view->wlr_surface->data = NULL;
	view->wlr_surface = NULL;
}

void
handle_surface_request_activate(struct wl_listener *listener, void *data)
{
	struct cg_view *view = wl_container_of(listener, view, request_activate);

	wlr_scene_node_raise_to_top(&view->scene_tree->node);
	seat_set_focus(view->server->seat, view);
}

void
handle_surface_request_close(struct wl_listener *listener, void *data)
{
	struct cg_view *view = wl_container_of(listener, view, request_close);
	view->impl->close(view);
}

void
view_map(struct cg_view *view, struct wlr_surface *surface)
{
	view->scene_tree = wlr_scene_subsurface_tree_create(&view->server->scene->tree, surface);
	if (!view->scene_tree)
		goto fail;
	view->scene_tree->node.data = view;

	view->wlr_surface = surface;
	surface->data = view;
	view_update_poc_role(view);

#if CAGE_HAS_XWAYLAND
	/* We shouldn't position override-redirect windows. They set
	   their own (x,y) coordinates in handle_wayland_surface_map. */
	if (view->type != CAGE_XWAYLAND_VIEW || xwayland_view_should_manage(view))
#endif
	{
		view_position(view);
	}

	wl_list_insert(&view->server->views, &view->link);

	view->foreign_toplevel_handle = wlr_foreign_toplevel_handle_v1_create(view->server->foreign_toplevel_manager);
	if (!view->foreign_toplevel_handle)
		goto fail;

	view->request_activate.notify = handle_surface_request_activate;
	wl_signal_add(&view->foreign_toplevel_handle->events.request_activate, &view->request_activate);
	view->request_close.notify = handle_surface_request_close;
	wl_signal_add(&view->foreign_toplevel_handle->events.request_close, &view->request_close);

	seat_set_focus(view->server->seat, view);
	return;

fail:
	wl_resource_post_no_memory(surface->resource);
}

void
view_destroy(struct cg_view *view)
{
	struct cg_server *server = view->server;

	if (view->wlr_surface != NULL) {
		view_unmap(view);
	}

	view->impl->destroy(view);

	/* If there is a previous view in the list, focus that. */
	bool empty = wl_list_empty(&server->views);
	if (!empty) {
		struct cg_view *prev = wl_container_of(server->views.next, prev, link);
		seat_set_focus(server->seat, prev);
	}
}

void
view_init(struct cg_view *view, struct cg_server *server, enum cg_view_type type, const struct cg_view_impl *impl)
{
	view->server = server;
	view->type = type;
	view->poc_role = CG_POC_SURFACE_DEFAULT;
	view->impl = impl;
}

struct cg_view *
view_from_wlr_surface(struct wlr_surface *surface)
{
	assert(surface);
	return surface->data;
}
