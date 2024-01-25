Adding images to doxygen documentation

1. Change doxygen configuration file to specify images path:
 - In file 'Doxyfile.ncp_ll_api' edit 'IMAGE_PATH' parameter:
    Example:
        # The IMAGE_PATH tag can be used to specify one or more files or directories
        IMAGE_PATH             = ./doc_images/

2. Add images to the folder specified (other formats may work but `.png` is used)

3. In documentation add the following code inside a code comment to insert an image
in the place where it must appear.
  Example:
    \image html LL_init_sequence.png "Initialization"
    \image latex LL_init_sequence.png "Initialization" width=12cm

  Note that image must be defined both for 'html' and 'latex'(pdf), caption can be specified and
  width adjusted to fit correctly the page.
