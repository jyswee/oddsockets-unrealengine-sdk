/**
 * OddSockets UE SDK - Module entry point
 *
 * UE loads the plugin dylib and looks for the module's InitializeModule
 * symbol; without IMPLEMENT_MODULE the editor refuses the plugin with
 * "module 'OddSockets' could not be initialized successfully after it was
 * loaded". The SDK needs no startup/shutdown hooks, so the default
 * implementation is enough.
 */

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, OddSockets)
