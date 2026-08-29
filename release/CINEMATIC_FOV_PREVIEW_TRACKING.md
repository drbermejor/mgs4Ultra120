This issue is the single reporting location for the opt-in cinematic FOV
preview. Please do not open separate issues for individual scenes.

The recommended build remains
[v0.3.3-alpha.1](https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.1).
Use the preview only if you are willing to provide structured test results.

Preview release:
https://github.com/drbermejor/mgs4Ultra120/releases/tag/v0.3.3-alpha.2-cinematic-fov-preview

Expected limitation: a wider authored shot may reveal characters, objects,
geometry or animation transitions early at the expanded edges. This is scene
pop-in/early visibility, not the earlier projection/frustum culling regression.

Please include all of the following:

- operating system; on Linux include the distribution and Proton version;
- chapter, scene and checkpoint;
- output resolution and aspect ratio;
- `FOVMultiplier`;
- `CinematicFOVMultiplier`;
- whether the result changes with `ExperimentalCinematicFOV=0`;
- screenshot or short video;
- `mgs4_ultrawide.log` attached as a file;
- confirmation that only one `MGS4Ultra120.asi` is installed.

Reports without enough reproduction information may not be investigated.
