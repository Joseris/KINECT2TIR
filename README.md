# KINECT2TIR
## Description
This application utilizes Kinect depth camera data to emulate TrackIR 
hardware in several games.  It allows for head tracking in low light 
conditions without any extra head gear. Results may vary.

This program should work for Kinect for XBOX360 (tested) and Kinect for 
Windows (untested)

## Disclaimer
If this Application manipulates your game or computer in a harmful way, 
I claim no responsibility. I am not associated with NaturalPoint in any way.
I am distributing this Application free of charge, use at your own risk!
Some parts of this program were not made by me. Credit is given where 
credit is due.

## Installation
Install Kinect SDK v1 to your system.  (Kinect SDK v2 not supported)

Can be downloaded from Microsoft here:
http://www.microsoft.com/en-us/download/details.aspx?id=40278


Extract KINECT2TIR folder anywhere you like.  

## Uninstallation
Delete KINECT2TIR folder from your computer.

## Notes
* This program was designed for Windows7 or greater.
* This program requires DirectX installed on your system.
* This program requires to make changes to the hard drive (users folder) and 
  registry (NPClient.dll location).
* This is an early release and may not run under all circumstances.

## Usage
Run kinect2tir.exe
Press Install NPClient.dll (Needed only once!)
Adjust sensor so it is about 1 meter in front of you.
Load a Profile
Run TrackIR enabled Game
Enjoy!

## HotKeys
F12 - Recenter


## Version History
- What's new in v2.0b on 9/1/14?
 *Converted to KinectSDK v1
 *Fixed some smoothing and rendering bugs

- What's new in v1.0 on 9/14/12?
 *First version

## Known Bugs
* profile settings are incomplete
* some games may show an error when exiting

## Credits
Gabriele Fanelli for Head Pose Estimator
(http://www.vision.ee.ethz.ch/~gfanelli/)

OpenCV for Computer Vision library
(http://www.opencv.org)

FreeTrackNoIR for modified NPClient.dll
(http://facetracknoir.sourceforge.net/)

## Contact - Bug Reports
ToCA EDIT: http://www.tocaedit.com
You like this? I like donations!
Made by Racer_S
