#ifndef SOUNDSYSTEM_ISOUNDSYSTEM_H
#define SOUNDSYSTEM_ISOUNDSYSTEM_H

#include "lib/ILibSystem.h"

/**
*	@defgroup SoundSystem FMOD based sound system.
*
*	@{
*/

namespace soundsystem
{
/**
*	A sound system that can be used to play back sounds. Sounds are non-looping.
*/
class ISoundSystem : public ILibSystem
{
public:

	/**
	*	Initializes the sound system. Should be called on startup.
	*	@return true on success, false otherwise.
	*/
	virtual bool Initialize() = 0;

	/**
	*	Shuts down the sound system. Should be called on shutdown.
	*/
	virtual void Shutdown() = 0;

	/**
	*	Must be called every frame.
	*/
	virtual void RunFrame() = 0;

	/**
	*	Plays a sound by name. The filename is relative to the game's sound directory, and is looked up using the filesystem.
	*	@param pszFilename Sound filename.
	*	@param flVolume Volume. Expressed as a range between [0, 1].
	*	@param iPitch Pitch amount. Expressed as a range between [0, 255].
	*/
	virtual void PlaySound( const char* pszFilename, float flVolume, int iPitch ) = 0;

	/**
	*	Stops all sounds that are currently playing.
	*/
	virtual void StopAllSounds() = 0;
};
}

/**
*	ISoundSystem interface name.
*/
#define ISOUNDSYSTEM_NAME "ISoundSystemV001"

/** @} */

#endif //SOUNDSYSTEM_ISOUNDSYSTEM_H