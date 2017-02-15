# PhotoMosaic

Large multi-screen photomosaic with transitions and realtime mosaic generation.

Ideas for making the algorithm better:

- Take the average across all the tiles and subtract it before doing matching, then divide by the standard deviation? This should make centralized elements work better.
- Disassociate the drawn tiles from the tile objects: when the number of reference tiles is less than the number of photo tiles, use a smaller texture rather than filling up with repeating elements. This is helpful for situations where the texture set is fixed instead of expanding.
- Start by assigning tiles based solely on overall brightness.
- Does the smaller number of elements situation make the matching easier? We could do WxHxN comparisons where N is the number target tiles.
- For 2732x2048 at 30x30 pixels, that's 91x68 pixels or 6188 tiles. Potential matches for a given tile are probably 2% of the space or 100 tiles. This becomes a fairly small assignment problem for something like CSA, done in a few seconds. But this is more likely to have an error, and creates a bigger codebase.

Simpler changes:

- Highpass size should be a ratio of total size.
- Tiles should be built without resizing if possible. Or: resizing should be done in a way that takes number of sub-tile samples into account.
- Use maximum amount of time to solve photomosaic rather plus max iterations.
- Put the thread in the right place. Threaded image loading should, maybe, happen separately from threaded mosaic solving...? If we take a photo with the camera we can get the image immediately.
- Separate downsampling from the buildTiles code.
- Remove global variables.
- There's a bug when the size is not integer divisible by the tile size.