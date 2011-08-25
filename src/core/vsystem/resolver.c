/**
 * The Shadow Simulator
 *
 * Copyright (c) 2010-2011 Rob Jansen <jansen@cs.umn.edu>
 *
 * This file is part of Shadow.
 *
 * Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "resolver.h"
#include "sysconfig.h"
#include "utility.h"

resolver_tp resolver_create(int process_id) {
	resolver_tp r = malloc(sizeof(resolver_t));

	r->unique_id_counter = 0;
	r->addr_entry = g_hash_table_new(g_int_hash, g_int_equal);
	r->name_entry = g_hash_table_new(g_int_hash, g_int_equal);
	r->pid = process_id;

	return r;
}

void resolver_destroy(resolver_tp r) {
	if(r != NULL) {
		g_hash_table_foreach(r->addr_entry, (GHFunc)resolver_destroy_cb, NULL);
		g_hash_table_destroy(r->addr_entry);
		r->addr_entry = NULL;
		g_hash_table_destroy(r->name_entry);
		r->name_entry = NULL;
		free(r);
	}
}

void resolver_destroy_cb(int key, void* value, void *param) {
	if(value != NULL) {
		free(value);
	}
}

/* name MUST be null-terminated */
void resolver_add(resolver_tp r, char* name, in_addr_t addr, uint8_t prepend_unique_id, uint32_t KBps_down, uint32_t KBps_up) {
	resolver_entry_tp rentry;
	size_t hostname_len;

	if(strlen(name) < 1) {
		/* in this case we always add a unique id */
		name = "default.shadow";
		prepend_unique_id = 1;
	}

	if(prepend_unique_id) {
		hostname_len = strlen(name) + RESOLVER_ID_MAXLENSTR + 1;
		rentry = malloc(sizeof(resolver_entry_t) + hostname_len);
		snprintf(rentry->hostname, hostname_len, "%u.%s.%i", r->unique_id_counter++, name, r->pid);
	} else {
		hostname_len = strlen(name)+1;
		rentry = malloc(sizeof(resolver_entry_t) + hostname_len);
		snprintf(rentry->hostname, hostname_len, "%s", name);
	}

	rentry->addr = addr;
	rentry->KBps_down = KBps_down;
	rentry->KBps_up = KBps_up;

	g_hash_table_insert(r->addr_entry, int_key(rentry->addr), rentry);
	int key = g_str_hash(rentry->hostname);
	g_hash_table_insert(r->name_entry, int_key(key), rentry);
}

void resolver_remove_byname(resolver_tp r, char* name) {
	if(r != NULL) {
		int key = g_str_hash(name);
		resolver_entry_tp rentry = g_hash_table_lookup(r->name_entry, &key);
		g_hash_table_remove(r->name_entry, &key);

		if(rentry != NULL) {
			g_hash_table_remove(r->addr_entry, &rentry->addr);
			free(rentry);
		}
	}
}

void resolver_remove_byaddr(resolver_tp r, in_addr_t addr) {
	if(r != NULL) {
		resolver_entry_tp rentry = g_hash_table_lookup(r->addr_entry, &addr);
		g_hash_table_remove(r->addr_entry, &addr);

		if(rentry != NULL) {
			int key = g_str_hash(rentry->hostname);
			g_hash_table_remove(r->name_entry, &key);
			free(rentry);
		}
	}
}

in_addr_t* resolver_resolve_byname(resolver_tp r, char* name) {
	if(r != NULL) {
		int key = g_str_hash(name);
		resolver_entry_tp rentry = g_hash_table_lookup(r->name_entry, &key);

		if(rentry != NULL) {
			return &(rentry->addr);
		}
	}
	return NULL;
}

char* resolver_resolve_byaddr(resolver_tp r, in_addr_t addr) {
	if(r != NULL) {
		resolver_entry_tp rentry = g_hash_table_lookup(r->addr_entry, &addr);

		if(rentry != NULL) {
			return rentry->hostname;
		}
	}
	return NULL;
}

uint32_t resolver_get_minbw(resolver_tp r, in_addr_t addr) {
	if(r != NULL) {
		resolver_entry_tp rentry = g_hash_table_lookup(r->addr_entry, &addr);

		if(rentry != NULL) {
			return rentry->KBps_down < rentry->KBps_up ? rentry->KBps_down : rentry->KBps_up;
		}
	}
	return 0;
}

uint32_t resolver_get_upbw(resolver_tp r, in_addr_t addr) {
	if(r != NULL) {
		resolver_entry_tp rentry = g_hash_table_lookup(r->addr_entry, &addr);

		if(rentry != NULL) {
			return rentry->KBps_up;
		}
	}
	return 0;
}

uint32_t resolver_get_downbw(resolver_tp r, in_addr_t addr) {
	if(r != NULL) {
		resolver_entry_tp rentry = g_hash_table_lookup(r->addr_entry, &addr);

		if(rentry != NULL) {
			return rentry->KBps_down;
		}
	}
	return 0;
}
