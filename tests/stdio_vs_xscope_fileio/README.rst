example: fileio_feature_close
=============================

This example shows how to use open and close files with xscope_fileio. 
 
Build example
-------------
Run the following command from the current directory: 

.. code-block:: console

  cmake -G "Unix Makefiles" -B build
  xmake -C build


Running example
---------------

.. warning::

  Make sure ``xscope_fileio`` is installed.
  

Xscope test:

.. code-block:: console

  python run_example.py --adapter-id "your-adapter-id"

Stdio test:

.. code-block:: console

  xrun --xscope bin/stdio_vs_xscope.xe

Output
------

The output will be several files in the current directory inside the output folder. 
