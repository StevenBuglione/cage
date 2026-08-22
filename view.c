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

const char *
view_get_app_id(struct cg_view *view)
{
	return view->impl->get_app_id(view);
}

static enum cg_poc_surface_role
view_role_from_surface_kind(enum cg_surface_kind kind)
{
	switch (kind) {
	case CG_SURFACE_KIND_APP_VIEW:
		return CG_POC_SURFACE_WORKSPACE;
	case CG_SURFACE_KIND_OVERLAY:
		return CG_POC_SURFACE_CONTROLS;
	case CG_SURFACE_KIND_FIREFOX_VIEW:
	case CG_SURFACE_KIND_POPUP:
		return CG_POC_SURFACE_BROWSER;
	}
	return CG_POC_SURFACE_DEFAULT;
}

bool
view_accepts_input(const struct cg_view *view)
{
	return view && cg_surface_view_policy_accepts_input(&view->surface_policy);
}

static void
view_quarantine(struct cg_view *view)
{
	if (!view) {
		return;
	}
	if (view->surface_policy.state != CG_SURFACE_VIEW_QUARANTINED) {
		cg_surface_view_policy_quarantine(&view->surface_policy);
	}
	if (view->scene_tree) {
		wlr_scene_node_set_enabled(&view->scene_tree->node, false);
	}
	if (view->server->seat) {
		seat_clear_focus(view->server->seat, view);
	}
}

static bool
view_associate_surface(struct cg_view *view, struct wlr_surface *surface)
{
	struct cg_surface_controller *controller = &view->server->surface_controller;
	bool registry_required = controller->accepting;
	bool new_association = registry_required && view->surface_policy.state == CG_SURFACE_VIEW_UNMANAGED;

	if (!cg_surface_view_policy_associate(&view->surface_policy, registry_required, &view->server->surface_registry,
					      view_get_app_id(view), (uintptr_t) surface,
					      cg_surface_controller_now(controller))) {
		view_quarantine(view);
		return false;
	}
	if (!registry_required) {
		view->poc_role = CG_POC_SURFACE_DEFAULT;
		return true;
	}
	view->poc_role = view_role_from_surface_kind(view->surface_policy.identity.kind);
	if (new_association && !cg_surface_controller_notify_associated(controller, &view->surface_policy.identity)) {
		struct cg_surface_identity identity = view->surface_policy.identity;
		(void) cg_surface_registry_retire(&view->server->surface_registry, identity.scene_id,
						  identity.surface_id);
		view_quarantine(view);
		return false;
	}
	return true;
}

void
view_handle_surface_controller_event(const struct cg_surface_controller_event *event, void *data)
{
	struct cg_server *server = data;
	struct cg_view *view;

	if (!event || !server) {
		return;
	}
	wl_list_for_each (view, &server->views, link) {
		if (view->surface_policy.state != CG_SURFACE_VIEW_ASSOCIATED) {
			continue;
		}
		if (event->type == CG_SURFACE_CONTROLLER_RESET ||
		    (event->type == CG_SURFACE_CONTROLLER_RETIRED &&
		     view->surface_policy.identity.scene_id == event->scene_id &&
		     view->surface_policy.identity.surface_id == event->surface_id)) {
			view_quarantine(view);
		}
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
	if (view->foreign_toplevel_handle) {
		wlr_foreign_toplevel_handle_v1_set_activated(view->foreign_toplevel_handle, activate);
	}
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
		if (cg_surface_view_policy_visible(&view->surface_policy)) {
			view_position(view);
		}
	}
}

void
view_unmap(struct cg_view *view)
{
	seat_clear_focus(view->server->seat, view);
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

	if (!view_accepts_input(view)) {
		return;
	}
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
	bool associated = view_associate_surface(view, surface);

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

	if (associated) {
		seat_set_focus(view->server->seat, view);
	}
	return;

fail:
	wl_resource_post_no_memory(surface->resource);
}

void
view_destroy(struct cg_view *view)
{
	struct cg_server *server = view->server;
	struct cg_surface_identity identity = view->surface_policy.identity;
	bool associated = view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED;

	if (view->wlr_surface != NULL) {
		view_unmap(view);
	}
	if (associated) {
		(void) cg_surface_registry_retire(&server->surface_registry, identity.scene_id, identity.surface_id);
		cg_surface_view_policy_quarantine(&view->surface_policy);
	}

	view->impl->destroy(view);

	/* Focus the first remaining view that is still allowed to receive input. */
	struct cg_view *candidate;
	wl_list_for_each (candidate, &server->views, link) {
		if (view_accepts_input(candidate)) {
			seat_set_focus(server->seat, candidate);
			break;
		}
	}
}

void
view_init(struct cg_view *view, struct cg_server *server, enum cg_view_type type, const struct cg_view_impl *impl)
{
	view->server = server;
	view->type = type;
	view->poc_role = CG_POC_SURFACE_DEFAULT;
	cg_surface_view_policy_init(&view->surface_policy);
	view->impl = impl;
}

struct cg_view *
view_from_wlr_surface(struct wlr_surface *surface)
{
	assert(surface);
	return surface->data;
}
