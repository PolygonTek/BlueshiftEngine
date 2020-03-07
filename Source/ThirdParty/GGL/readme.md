GenGGL
============================

GenGGL is platform independent OpenGL API C glue code generator written in ruby.

It will help you to write OpenGL C/C++ code without concerning OpenGL versions and platforms.

Features
----------------------------

* Platform independent bindings to specific OpenGL profile and version. (you can specify it with following arguments)
* Parsing [OpenGL registry](http://www.opengl.org/registry) XML files as source to generate up-to-date API.
* There are debug functionalities for OpenGL function calls.

Usage
----------------------------
First off, you need ruby 2.0 in system for use. 

~~~
$ cd genggl
$ ruby genggl.rb <api> <version> <output-dir>
~~~

You can specify the profile that is one of the "CORE", "COMPATIBILITY", "ES" and version. 
After that, you could get all the output files in GGL folder

Issues
----------------------------
SSL connection error on Windows platform -> [Solution](https://gist.github.com/fnichol/867550)

Usage about ggl (generated C code)
----------------------------
* `ggl_init`: initialize ggl function pointers for use.
* `ggl_rebind`: re-bind ggl function pointers for debug purpose
* you can check extensions easily with provided extension struct. (ex. `gglext._GL_EXT_texture3D`)
