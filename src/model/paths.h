// paths.h -- resolve the plugin's own data directory at runtime (from THIS DLL's location) instead of a
// hardcoded absolute path, so config / profiles / cache / assets load on ANY machine, not just the dev box.
// plugin_dir() == "<windower>\plugins\AioHud" (the folder holding assets/ + the data/ files).
#pragma once

namespace aio {

const char* plugin_dir();                                   // cached ; "" if the module path can't be resolved
void        plugin_path(char* out, int cap, const char* rel);   // out := "<plugin_dir>\<rel>" (rel uses \ separators)
const char* plugin_path_r(const char* rel);                 // same, into one of 8 rotating static buffers (for inline load calls)
void        plugin_path_w(wchar_t* out, int cap, const wchar_t* rel);   // wide variant (out := L"<plugin_dir>\<rel>") for the Win32 -W font APIs

} // namespace aio
