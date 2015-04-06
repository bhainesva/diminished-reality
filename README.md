Compile our program with
    make

Edit the video name in the source. When it opens click around to create contour points and then hit esc to see the drawn contour. Hit esc again to play the vid with a mask drawn and updated around the object.

PatchMatch - compile minimal version standalone with:
    make patchmatch
Use the standalone version to figure out exactly how it works and what it does. Easier said then done; I might end up having to ask Barnes about it. Also, I'll be looking into getting Matlab installed so we can use the optimized, nonminimal versions of the PM code.

The source and the target images are the things that PM will be operating on, ie finding the NNF that will be used to fill in the contoured area.
I believe the source is entire image minus the contour, and the target will be dilated contour area minus the contour? Not entirely sure on this bit.
