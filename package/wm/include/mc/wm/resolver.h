#ifndef MC_WM_RESOLVER_H
#define MC_WM_RESOLVER_H

#include <mc/wm/wm.h>

MC_Error mc_wm_resolver_register(const MC_WMVtab *vtab);
void mc_wm_resolver_set(MC_WM *wm);
void mc_wm_resolver_forget(MC_WM *wm);

MC_Error mc_wm_resolve(MC_WMRef **wm);
MC_Error mc_wm_resolve_as(const char *impl, MC_WMRef **wm);

#endif
