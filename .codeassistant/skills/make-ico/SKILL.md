---
name: build
description: Make icon file
---

Take the provided image (for example, `icon.png`) and run the command:
```
magick icon.png -define icon:auto-resize=512,256,128,64,48,32,24,16 icon.ico
```

The output will be the `icon.ico` file.
Icons for demo applications are stored in the `/demo/<demo-app>/resources/icons` directory.
The icons are registered in the file `/demo/<demo-app>/resources/app.qrc` and sorted alphabetically.
