# camcap
Capture from camera, using OpenCV, filter on change, and store images to disk

Example of usage:
```
➜ mkdir images

➜ camcap http://192.168.1.2 
iConnected to camera: FFMPEG 1440x1080 (25 fps)
Establishing baseline...
2024-09-04 12:45:27.824 score: 0.0157407
2024-09-04 12:45:28.791 score: 0.0159047
...
```

Images are stored in the 'images' directory, if level of change in two consecutive images passes some arbitrary threshold.

