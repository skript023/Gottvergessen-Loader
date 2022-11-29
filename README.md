<h1 align="center">Gottvergessen Loader</h1>
<p align="center">
  <a href="https://github.com/skript023/Gottvergessen-Loader/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/skript023/Ellohim-Project.svg?style=flat-square"/>
   </a>
  <a href="https://github.com/skript023/Gottvergessen-Loader/actions">
      <img src="https://img.shields.io/github/workflow/status/skript023/Gottvergessen-Loader/CI/main?style=flat-square"/>
   </a>
  <br>
  DLL Injector remote through REST API strictly for educational purposes.
</p>

## Features
* ImGui–based user interface
* Log console
* Thread pool
* HTTP Request
* DLL downloader
* Check DLL version

## Building
To build Gottvergessen Loader you need:
* Visual Studio 2022
* [Premake 5.0](https://premake.github.io) in your PATH

To set up the build environment, run the following commands in a terminal:
```dos
git clone https://github.com/skript023/Gottvergessen-Loader.git --recurse-submodules
GenerateProjects.bat
```
Now, you will be able to open the solution, and simply build it in Visual Studio.