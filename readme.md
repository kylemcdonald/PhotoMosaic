# PhotoMosaic

Large multi-screen photomosaic with transitions and realtime mosaic generation.

The loading code assumes that all icons are square.

- Break out the timer into a separate class.
- Create a Photomosaic class for capturing what the app is orchestrating right now.

Finishing for mobile:

- Re-add webcam input.
- Reintroduce threading where necessary.

Finishing for sharing:

- Add checks for number of channels and equal sized arguments.
- Add everything to a namespace.
- Need one more function that computes the final image using OpenCV.
