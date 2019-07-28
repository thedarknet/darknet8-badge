#
# Main Makefile. This is basically the same as a component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


#Compile image file into the resulting firmware binary
#COMPONENT_EMBED_FILES := image.jpg

COMPONENT_EMBED_TXTFILES := $(PROJECT_PATH)/server_certs/ca_cert.pem

COMPONENT_SRCDIRS := . menus lib
