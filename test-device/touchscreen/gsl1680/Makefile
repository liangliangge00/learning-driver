#
# Makefile for the GSL1680 touchscreen driver.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_TOUCHSCREEN_GSL1680) += gsl_point_id.o gsl_ReadArrayData_c.o gsl1680.o
$(obj)/gsl_point_id.o: $(srctree)/$(obj)/gsl_point_id
	cp $(srctree)/$(obj)/gsl_point_id $(obj)/gsl_point_id.o

