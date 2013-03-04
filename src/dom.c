/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "main.h"
#include "dom.h"

static gboolean dom_auto_insert(Client* c, Element* element);
static gboolean dom_editable_focus_cb(Element* element, Event* event, Client* c);
static Element* dom_get_active_element(Document* doc);


void dom_check_auto_insert(Client* c)
{
    Document* doc   = webkit_web_view_get_dom_document(c->gui.webview);
    Element* active = dom_get_active_element(doc);
    if (!dom_auto_insert(c, active)) {
        HtmlElement* element = webkit_dom_document_get_body(doc);
        if (!element) {
            element = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_document_element(doc));
        }
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true, c
        );
    }
}

/**
 * Indicates if the given dom element is an editable element like text input,
 * password or textarea.
 */
gboolean dom_is_editable(Element* element)
{
    if (!element) {
        return FALSE;
    }

    char* tagname = webkit_dom_element_get_tag_name(element);
    if (!g_ascii_strcasecmp(tagname, "textarea")) {
        return TRUE;
    }
    char *type = webkit_dom_element_get_attribute(element, "type");
    if (!g_ascii_strcasecmp(tagname, "input")
        && g_ascii_strcasecmp(type, "submit")
        && g_ascii_strcasecmp(type, "reset")
        && g_ascii_strcasecmp(type, "image")
    ) {
        return TRUE;
    }

    return FALSE;
}

static gboolean dom_auto_insert(Client* c, Element* element)
{
    if (dom_is_editable(element)) {
        vp_set_mode(c, VP_MODE_INSERT, FALSE);
        return TRUE;
    }
    return FALSE;
}

static gboolean dom_editable_focus_cb(Element* element, Event* event, Client* c)
{
    webkit_dom_event_target_remove_event_listener(
        WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true
    );
    if (CLEAN_MODE(c->state.mode) != VP_MODE_INSERT) {
        EventTarget* target = webkit_dom_event_get_target(event);
        dom_auto_insert(c, (void*)target);
    }
    return FALSE;
}

static Element* dom_get_active_element(Document* doc)
{
    Document* d     = NULL;
    Element* active = webkit_dom_html_document_get_active_element((void*)doc);
    char* tagname  = webkit_dom_element_get_tag_name(active);

    if (!g_strcmp0(tagname, "FRAME")) {
        d = webkit_dom_html_frame_element_get_content_document(WEBKIT_DOM_HTML_FRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    if (!g_strcmp0(tagname, "IFRAME")) {
        d = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    return active;
}
