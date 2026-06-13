#include <mc/wm/resolver.h>
#include <mc/wm/target.h>

#include <string.h>

#define RESOLVER_MAX_VTABS 16

static const MC_WMVtab *registered[RESOLVER_MAX_VTABS];
static size_t registered_count;
static MC_WM *current;

static const MC_WMVtab *find_vtab(const char *name) {
    for (size_t i = 0; i < registered_count; i++) {
        if (strcmp(registered[i]->name, name) == 0) {
            return registered[i];
        }
    }

    return NULL;
}

MC_Error mc_wm_resolver_register(const MC_WMVtab *vtab) {
    if (vtab == NULL) {
        return MCE_INVALID_INPUT;
    }

    if (find_vtab(vtab->name) != NULL) {
        return MCE_OK;
    }

    if (registered_count >= RESOLVER_MAX_VTABS) {
        return MCE_OUT_OF_MEMORY;
    }

    registered[registered_count++] = vtab;

    return MCE_OK;
}

void mc_wm_resolver_set(MC_WM *wm) {
    current = wm;
}

void mc_wm_resolver_forget(MC_WM *wm) {
    if (current == wm) {
        current = NULL;
    }
}

static MC_Error create_owned(const MC_WMVtab *vtab, MC_WMRef **wm) {
    MC_WM *owner;
    MC_Error status = mc_wm_init(&owner, vtab);
    if (status != MCE_OK) {
        return status;
    }

    *wm = mc_wm_get_ref(owner);
    return MCE_OK;
}

MC_Error mc_wm_resolve(MC_WMRef **wm) {
    if (wm == NULL) {
        return MCE_INVALID_INPUT;
    }

    if (current != NULL) {
        *wm = mc_wm_ref(mc_wm_get_ref(current));
        return MCE_OK;
    }

    if (registered_count == 0) {
        return MCE_NOT_FOUND;
    }

    return create_owned(registered[registered_count - 1], wm);
}

MC_Error mc_wm_resolve_as(const char *impl, MC_WMRef **wm) {
    if (impl == NULL || wm == NULL) {
        return MCE_INVALID_INPUT;
    }

    if (current != NULL && strcmp(mc_wm_impl_name(mc_wm_get_ref(current)), impl) == 0) {
        *wm = mc_wm_ref(mc_wm_get_ref(current));
        return MCE_OK;
    }

    const MC_WMVtab *vtab = find_vtab(impl);
    if (vtab == NULL) {
        return MCE_NOT_FOUND;
    }

    return create_owned(vtab, wm);
}
