#include <SFML/Config.hpp>

#include "app/Application.h"

// On laptops with hybrid graphics (integrated + discrete GPU), Windows lets the
// NVIDIA/AMD driver pick which GPU runs each process from a database of known
// games. A small indie exe not in that database can silently be routed to the
// weak integrated GPU while a discrete one sits idle. Exporting these two
// symbols is the vendor-documented way to force the discrete GPU for this
// process instead. On single-GPU machines it has no effect. DWORD is just
// `unsigned long` here, so this needs no <Windows.h>.
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// The project vendors a specific SFML build (libs/SFML) compiled with the same
// toolset as the game. Guarding the version here turns "linked against the
// wrong SFML" from a confusing runtime crash into a clear compile-time error.
static_assert(
	SFML_VERSION_MAJOR == 3 &&
	SFML_VERSION_MINOR == 1 &&
	SFML_VERSION_PATCH == 0,
	"Tessera requires SFML 3.1.0.");

int main()
{
	Application application;
	application.Run();

	return 0;
}
