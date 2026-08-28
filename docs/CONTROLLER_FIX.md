# Controller profile fix

The PC port can detect a native XInput controller correctly and later switch
its internal active-device profile to keyboard/mouse after unrelated input or
a cinematic transition. Symptoms reported with several controller families
include non-responsive sticks, buttons that work only while the mouse moves,
and keyboard button prompts replacing gamepad prompts.

The optional fix hooks the game's own profile-selection function. It learns
the native controller family selected by the game, rejects only a spurious
switch to profile 0 while at least one controller remains connected, and clears
the latch after all controllers disconnect.

It does **not**:

- install a driver or system service;
- create an Xbox 360/One/Elite virtual device;
- convert keyboard or mouse input;
- poll the controller or synthesize events;
- require Steam Input to be enabled or disabled globally.

This keeps system/controller setup outside the public patch. Use one native
controller path and avoid duplicate virtual devices. Keyboard/mouse users
should disable `ControllerProfileFixEnabled`.

Hot unplug/replug was exercised on the tested Linux/Proton setup: the game
reported disconnect, released the preserved profile, detected reconnection and
resumed native XInput calls without restarting. Windows validation across more
controller models is still requested.
