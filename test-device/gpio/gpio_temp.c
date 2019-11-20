#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
...

struct gpio_drvdata {
    /* gpio号 */
    int gpio_num;
    ...
    ...
};

static int __init gpio_init(void)
{
   struct gpio_drvdata *ddata;
   int ret;
   
   ddata = kzalloc(sizeof(*ddata), GFP_KERNEL);
   if (!ddata)
}

static void __exit gpio_exit(void)
{

}

module_init(gpio_init);
module_exit(gpio_exit);
