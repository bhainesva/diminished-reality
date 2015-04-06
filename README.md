Compile with:

g++ <name>.cpp -o app \`pkg-config --cflags --libs opencv\`

Edit the video name in the source. When it opens click around to create contour points and then hit esc to see the drawn contour. Hit esc again to play the vid with a mask drawn and updated around the object.

PatchMatch:
Compile with g++ pm_minimial.cpp
Code needs to be integrated into our stuff. Requires figuring out exactly what it's outputting..
