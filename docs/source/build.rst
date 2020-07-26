Build AssetKit
================================

| **AssetKit** does not have any external dependencies.

CMake (All platforms):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ mkdir build
  $ cd build
  $ cmake .. # [Optional] -DAK_SHARED=ON
  $ make
  $ sudo make install # [Optional]

**make** will build **AssetKit** into **build** folder.
If you don't want to install **AssetKit** to your system's folder you can get static and dynamic libs in this folder.

**CMake Options:**

.. code-block:: CMake
  :linenos:

  option(AK_SHARED "Shared build" ON)
  option(AK_STATIC "Static build" OFF)
  option(AK_USE_C99 "" OFF) # C11 
  option(AK_USE_TEST "Enable Tests" OFF) # for make check - make test

**Use with your CMake project example**

.. code-block:: CMake
  :linenos:

  cmake_minimum_required(VERSION 3.8.2)
  
  project(<Your Project Name>)
  
  add_executable(${PROJECT_NAME} src/main.c)
  target_link_libraries(${LIBRARY_NAME} PRIVATE assetkit)
  
  add_subdirectory(external/assetkit/)

Unix (Autotools):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ sh autogen.sh
  $ ./configure
  $ make
  $ make check            # run tests (optional)
  $ [sudo] make install   # install to system (optional)

**make** will build **AssetKit** to **.libs** sub folder in project folder.
If you don't want to install **AssetKit** to your system's folder you can get static and dynamic libs in this folder.

**NOTE:**: Change install name if required; after build is finished, **make** automatically runs `sh ./post-build.sh` script. 
It changes install names. You may want to edit build scripts and `post-build.sh` script if you want to build AssetKit with existing libraries. 
Default behavior is that AssetKit will look up sub libraries inside 
`.libs` folder, if you only need to change `.libs` name then change it in `post-build.sh` script file.

Windows (MSBuild):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Windows related build files, project files are located in `win` folder,
make sure you are inside in assetkit/win folder.

Code Analysis are enabled, it may take awhile to build.

.. code-block:: bash
  :linenos:

  $ cd win
  $ .\build.bat

if *msbuild* is not worked (because of multi versions of Visual Studio)
then try to build with *devenv*:

.. code-block:: bash
  :linenos:

  $ devenv assetkit.sln /Build Release

Currently tests are not available on Windows.

Documentation (Sphinx):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**AssetKit** uses sphinx framework for documentation, it allows lot of formats for documentation. To see all options see sphinx build page:

https://www.sphinx-doc.org/en/master/man/sphinx-build.html

Example build:

.. code-block:: bash
  :linenos:

  $ cd assetkit/docs
  $ sphinx-build source build
