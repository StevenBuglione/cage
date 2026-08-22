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
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "output.h"
#include "seat.h"
#include "server.h"
#include "view.h"
#if CAGE_HAS_XWAYLAND
#include "xwayland.h"
#endif

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

bool
view_accepts_input(const struct cg_view *view)
{
	if (!view || !cg_surface_view_policy_accepts_input(&view->surface_policy)) {
		return false;
	}
	if (!view->server->surface_controller.accepting) {
		return true;
	}
	return view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED && view->scene_present && view->scene_visible &&
	       view->scene_accepts_input;
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
	view->scene_present = false;
	view->scene_visible = false;
	view->scene_accepts_input = false;
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
		return true;
	}
	if (new_association && !cg_surface_controller_notify_associated(controller, &view->surface_policy.identity)) {
		struct cg_surface_identity identity = view->surface_policy.identity;
		(void) cg_surface_registry_retire(&view->server->surface_registry, identity.scene_id,
						  identity.surface_id);
		view_quarantine(view);
		return false;
	}
	return true;
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

static void
view_hide_scene_state(struct cg_view *view)
{
	view->scene_present = false;
	view->scene_visible = false;
	view->scene_accepts_input = false;
	view->scene_z_index = 0;
	if (view->scene_tree) {
		wlr_scene_node_set_enabled(&view->scene_tree->node, false);
		wlr_scene_subsurface_tree_set_clip(&view->scene_tree->node, NULL);
	}
	if (view->server->seat) {
		seat_clear_focus(view->server->seat, view);
	}
}

void
view_apply_scene_state(struct cg_view *view)
{
	const struct cg_scene_record *record;
	const struct cg_scene_surface_state *state;
	struct cg_scene_rect resolved;
	struct wlr_box bounds;
	struct wlr_box clip;

	if (!view || view->surface_policy.state != CG_SURFACE_VIEW_ASSOCIATED) {
		return;
	}
	record = cg_scene_model_find(&view->server->scene_model, view->surface_policy.identity.scene_id);
	state = record ? cg_scene_snapshot_find_surface(&record->snapshot, view->surface_policy.identity.surface_id)
		       : NULL;
	if (!state ||
	    !cg_scene_model_resolve_surface(&view->server->scene_model, view->surface_policy.identity.scene_id,
					    view->surface_policy.identity.surface_id, &resolved)) {
		view_hide_scene_state(view);
		return;
	}
	bounds = (struct wlr_box) {
		.x = state->bounds.x,
		.y = state->bounds.y,
		.width = state->bounds.width,
		.height = state->bounds.height,
	};
	clip = (struct wlr_box) {
		.x = resolved.x - state->bounds.x,
		.y = resolved.y - state->bounds.y,
		.width = resolved.width,
		.height = resolved.height,
	};
	view->scene_present = true;
	view->scene_visible = state->visible;
	view->scene_accepts_input = state->accepts_input;
	view->scene_z_index = state->z_index;
	view_maximize(view, &bounds);
	wlr_scene_subsurface_tree_set_clip(&view->scene_tree->node, &clip);
	wlr_scene_node_set_enabled(&view->scene_tree->node, true);
	if (!view->scene_accepts_input && view->server->seat) {
		seat_clear_focus(view->server->seat, view);
	}
}

void
view_apply_surface_state(struct cg_server *server, cg_scene_id scene_id, cg_surface_id surface_id)
{
	struct cg_view *view;

	if (!server || scene_id == 0 || surface_id == 0) {
		return;
	}
	wl_list_for_each (view, &server->views, link) {
		if (view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED &&
		    view->surface_policy.identity.scene_id == scene_id &&
		    view->surface_policy.identity.surface_id == surface_id) {
			view_apply_scene_state(view);
			return;
		}
	}
}

static void
view_apply_scene_order_and_focus(struct cg_server *server, cg_scene_id scene_id)
{
	struct cg_view *ordered[CG_SCENE_SURFACE_CAPACITY];
	size_t count = 0;
	struct cg_view *view;
	const struct cg_scene_record *record = cg_scene_model_find(&server->scene_model, scene_id);

	wl_list_for_each (view, &server->views, link) {
		if (view->surface_policy.state != CG_SURFACE_VIEW_ASSOCIATED ||
		    view->surface_policy.identity.scene_id != scene_id || !view->scene_present ||
		    !view->scene_visible) {
			continue;
		}
		size_t position = count;
		while (position > 0 && (ordered[position - 1]->scene_z_index > view->scene_z_index ||
					(ordered[position - 1]->scene_z_index == view->scene_z_index &&
					 ordered[position - 1]->surface_policy.identity.surface_id >
						 view->surface_policy.identity.surface_id))) {
			ordered[position] = ordered[position - 1];
			position--;
		}
		ordered[position] = view;
		count++;
	}
	for (size_t index = 0; index < count; index++) {
		wlr_scene_node_raise_to_top(&ordered[index]->scene_tree->node);
	}
	struct cg_view *focused = seat_get_focus(server->seat);
	if (!record || !record->snapshot.has_focused_surface) {
		if (focused && focused->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED &&
		    focused->surface_policy.identity.scene_id == scene_id) {
			seat_clear_focus(server->seat, focused);
		}
		return;
	}
	for (size_t index = 0; index < count; index++) {
		if (ordered[index]->surface_policy.identity.surface_id == record->snapshot.focused_surface_id) {
			seat_set_focus(server->seat, ordered[index]);
			return;
		}
	}
	if (focused && focused->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED &&
	    focused->surface_policy.identity.scene_id == scene_id) {
		seat_clear_focus(server->seat, focused);
	}
}

void
view_handle_surface_controller_event(const struct cg_surface_controller_event *event, void *data)
{
	struct cg_server *server = data;
	struct cg_view *view;

	if (!event || !server) {
		return;
	}
	if (server->seat && server->resize_session.active &&
	    (event->type == CG_SURFACE_CONTROLLER_RESET ||
	     ((event->type == CG_SURFACE_CONTROLLER_RETIRED || event->type == CG_SURFACE_CONTROLLER_SCENE_DESTROYED ||
	       event->type == CG_SURFACE_CONTROLLER_SCENE_APPLIED ||
	       event->type == CG_SURFACE_CONTROLLER_OUTPUT_RESIZED) &&
	      server->resize_session.hit.scene_id == event->scene_id &&
	      (event->type != CG_SURFACE_CONTROLLER_RETIRED ||
	       server->resize_session.hit.surface_id == event->surface_id)))) {
		(void) seat_cancel_resize(server->seat);
	}
	wl_list_for_each (view, &server->views, link) {
		if (view->surface_policy.state != CG_SURFACE_VIEW_ASSOCIATED) {
			continue;
		}
		if (event->type == CG_SURFACE_CONTROLLER_RESET ||
		    ((event->type == CG_SURFACE_CONTROLLER_RETIRED ||
		      event->type == CG_SURFACE_CONTROLLER_SCENE_DESTROYED) &&
		     view->surface_policy.identity.scene_id == event->scene_id &&
		     (event->type == CG_SURFACE_CONTROLLER_SCENE_DESTROYED ||
		      view->surface_policy.identity.surface_id == event->surface_id))) {
			view_quarantine(view);
			continue;
		}
		if ((event->type == CG_SURFACE_CONTROLLER_SCENE_APPLIED ||
		     event->type == CG_SURFACE_CONTROLLER_OUTPUT_RESIZED) &&
		    view->surface_policy.identity.scene_id == event->scene_id) {
			view_apply_scene_state(view);
		}
	}
	if (event->type == CG_SURFACE_CONTROLLER_SCENE_APPLIED || event->type == CG_SURFACE_CONTROLLER_OUTPUT_RESIZED) {
		view_apply_scene_order_and_focus(server, event->scene_id);
	}
}

void
view_position(struct cg_view *view)
{
	struct wlr_box layout_box;
	wlr_output_layout_get_box(view->server->output_layout, NULL, &layout_box);

	if (view->server->surface_controller.accepting && view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED) {
		view_apply_scene_state(view);
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
	if (server->surface_controller.accepting) {
		struct wlr_box layout_box;
		wlr_output_layout_get_box(server->output_layout, NULL, &layout_box);
		if (server->seat && server->resize_session.active) {
			const struct cg_scene_record *active =
				cg_scene_model_find(&server->scene_model, server->resize_session.hit.scene_id);
			if (!active || active->output_width != (uint32_t) layout_box.width ||
			    active->output_height != (uint32_t) layout_box.height) {
				(void) seat_cancel_resize(server->seat);
			}
		}
		for (size_t index = 0; index < CG_SCENE_CAPACITY; index++) {
			if (server->scene_model.scenes[index].occupied) {
				(void) cg_scene_model_resize_output(
					&server->scene_model, server->scene_model.scenes[index].snapshot.scene_id,
					(uint32_t) layout_box.width, (uint32_t) layout_box.height);
			}
		}
	}
	wl_list_for_each (view, &server->views, link) {
		if (cg_surface_view_policy_visible(&view->surface_policy)) {
			view_position(view);
		}
	}
	if (server->surface_controller.accepting) {
		for (size_t index = 0; index < CG_SCENE_CAPACITY; index++) {
			if (server->scene_model.scenes[index].occupied) {
				cg_scene_id scene_id = server->scene_model.scenes[index].snapshot.scene_id;
				view_apply_scene_order_and_focus(server, scene_id);
			}
		}
	}
}

void
view_unmap(struct cg_view *view)
{
	if (view->server->seat && view->server->resize_session.active &&
	    view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED &&
	    view->server->resize_session.hit.scene_id == view->surface_policy.identity.scene_id &&
	    view->server->resize_session.hit.surface_id == view->surface_policy.identity.surface_id) {
		(void) seat_cancel_resize(view->server->seat);
	}
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
	if (view->surface_policy.state == CG_SURFACE_VIEW_UNMANAGED) {
		wlr_scene_node_raise_to_top(&view->scene_tree->node);
	}
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
	if (view->server->surface_controller.accepting || view->type != CAGE_XWAYLAND_VIEW ||
	    xwayland_view_should_manage(view))
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

	if (associated && view->server->surface_controller.accepting &&
	    view->surface_policy.state == CG_SURFACE_VIEW_ASSOCIATED) {
		view_apply_scene_order_and_focus(view->server, view->surface_policy.identity.scene_id);
	} else if (associated) {
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
	cg_surface_view_policy_init(&view->surface_policy);
	view->impl = impl;
}

struct cg_view *
view_from_wlr_surface(struct wlr_surface *surface)
{
	assert(surface);
	return surface->data;
}
