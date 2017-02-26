# PhotoMosaic

Large multi-screen photomosaic with transitions and realtime mosaic generation.

The loading code assumes that all icons are square.

- Break out the timer into a separate class.
- Move buildTiles out of Tile class. Subsampling variable should only appear in one place.

Finishing for mobile:

- Re-add webcam input.
- Reintroduce threading wherever necessary.

Finishing for sharing:

- Add checks for number of channels and equal sized arguments.
- Add everything to a namespace.
