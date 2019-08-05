	...
	...
	...
static const struct of_device_id wm8753_of_match[] = {
	{.compatible = "wlf, wm8753", },
	{},
};
MODULE_DEVICE_TABLE(of, wm8753_of_match);

static struct spi_driver wm8753_spi_driver = {
	.driver = {
		.name = "wm8753",
		.owner = THIS_MODULE,
		.of_match_table = wm8753_of_match,
	},
	.probe = wm8753_spi_probe,
	.remove = wm8753_spi_remove,
};

static struct i2c_driver wm8753_i2c_driver = {
	.driver = {
		.name = "wm8753",
		.owner = THIS_MODULE,
		.of_match_table = wm8753_of_match,
	},
	.probe = wm8753_i2c_probe,
	.remove = wm8753_i2c_remove,
	.id_table = wm8753_i2c_id,
};
	...
	...
	...