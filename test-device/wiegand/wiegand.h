#ifndef __WIEGAND_H
#define __WIEGAND_H

#undef __ATTR
#define __ATTR(_name, _mode, _show, _store) { \
    .attr = {.name = __stringify(_name),      \
         .mode = _mode},        \
    .show    = _show,     \
    .store    = _store,          \
}

#define	WIEGAND_DEBUG_ON 1
#define wiegand_debug(fmt, arg...)	\
	do {	\
		if(WIEGAND_DEBUG_ON)	\
		printk("<<-wiegand-debug->> [%d]"fmt"\n",__LINE__, ##arg);	\
	} while(0)

#define DRIVER_NAME		"wiegand"
#define DRIVER_VERSION  "1.0"

#define DATA_MAX	(128)

enum wiegand_ctrl_mode {
	WIEGAND_SEND = 0,
	WIEGAND_RECEIVE = 1,
};

#endif
