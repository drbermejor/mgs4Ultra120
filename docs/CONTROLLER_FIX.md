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

This keeps system/controller setup outside the public patch. The workaround is
disabled by default in alpha.6. Enable it only if the game incorrectly switches
away from an otherwise working controller. Keyboard/mouse and hybrid controller
plus mouse/gyro users should keep `ControllerProfileFixEnabled=0` because
locking the controller profile can interfere with mouse input.

## Enabling it after an update

Alpha.5 deliberately resets this workaround to disabled during managed setup
and updates. Users who previously needed it must enable it again:

- Windows or Linux GUI: enable **Controller profile fix** and save.
- Manual configuration: set the following under `[Input]` in
  `mgs4_ultrawide.ini`:

  ```ini
  ControllerProfileFixEnabled=1
  ```

Keep the value at `0` for keyboard/mouse or hybrid controller plus mouse/gyro
input.

Hot unplug/replug was exercised on the tested Linux/Proton setup: the game
reported disconnect, released the preserved profile, detected reconnection and
resumed native XInput calls without restarting. Windows validation across more
controller models is still requested.
