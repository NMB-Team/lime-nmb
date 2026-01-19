---
<div align="center">
<img src="./lime.png" alt="Lime"/>
</div>
---

> [!WARNING]
> This library does not support `Flash`, `AIR`, `Electron`, `Neko` targets. <br/>
> Also it works only with Haxe 4.3.x or greater.
> For best compatibility and performance, we recommend using the latest stable version of Haxe. <br/>
> If you need those, please use the [original version](https://github.com/openfl/lime) of the library instead. <br/>

---

<div align="center">

# Lime is a flexible, lightweight layer for Haxe cross-platform developers.

</div>

### Lime supports native and HTML5 targets with unified support for:

- Windowing
- Input
- Events
- Audio
- Render contexts
- Network access
- Assets

### Lime does not include a renderer, but exposes the current context:

- Cairo
- Canvas
- DOM
- GL

<div align="center">

#### The GL context is based upon the WebGL standard, implemented for both OpenGL and OpenGL ES as needed.

#### Lime provides a unified audio API, but also provides access to OpenAL for advanced audio on native targets.

</div>

# License

Lime is free, open-source software under the [MIT license](LICENSE.md).

# Installation

First install the latest version of [Haxe](http://www.haxe.org/download).

# Development Builds

When there are changes, Lime is built nightly. Builds are available for download [here](https://github.com/openfl/lime/actions?query=branch%3Adevelop+is%3Asuccess).

To install a development build, use the "haxelib local" command:

    haxelib local lime-haxelib.zip

# Building from Source

Clone the Lime repository, as well as the submodules:

    git clone --recursive https://github.com/NMB-Team/lime-nmb

Tell haxelib where your development copy of Lime is installed:

    haxelib dev lime lime

The first time you run the "lime" command, it will attempt to build the Lime standard binary for your desktop platform as the command-line tools. To build these manually, use the following command (using "mac" or "linux" if appropriate):

    haxelib install format
    haxelib install hxp
    lime rebuild windows

You can build additional binaries, or rebuild binaries after making changes, using "lime rebuild":

    lime rebuild windows
    lime rebuild linux -64 -release -clean

You can also rebuild the tools if you make changes to them:

    lime rebuild tools

On a Windows machine, you should have Microsoft Visual Studio C++ (Express is just fine) installed. You will need Xcode on a Mac. To build on a Linux machine, you may need the following packages (or similar):

    sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev g++ g++-multilib gcc-multilib libasound2-dev libx11-dev libxext-dev libxi-dev libxrandr-dev libxinerama-dev

To switch away from a source build, use:

    haxelib dev lime

# Sample

You can build a sample Lime project with the following commands:

    lime create HelloWorld
    cd HelloWorld
    lime test neko

You can also list other projects that are available using "lime create".

# Targets

Lime currently supports the following targets:

    lime test windows
    lime test mac
    lime test linux
    lime test android
    lime test ios
    lime test html5
    lime test hl

Desktop builds are currently designed to be built on the same host OS
