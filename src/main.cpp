#include <SFML/Config.hpp>

#include "app/Application.h"

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
