example: stdio_vs_xscope
========================

This test compares the performance of the standard C stdio
file I/O functions with the xscope_fileio functions.
It generates a table with the results of the tests.

``#define TEST_STDIO`` is used to rather test the standard C stdio file I/O functions or xscope_fileio functions. 

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
