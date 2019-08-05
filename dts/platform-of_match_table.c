	...
	...
	...
static const struct of_device_id a1234_i2c_of_match[] = {
	{.compatible = "acme,a1234-i2c-bus", },
	{},
};
MODULE_DEVICE_TABLE(of, a1234_i2c_of_match);

static struct platform_driver i2c_a1234_driver = {
	.driver = {
		.name = "a1234-i2c-bus",
		.owner = THIS_MODULE,
		.of_match_table = a1234_i2c_of_match,
	},
	.probe = i2c_a1234_probe,
	.remove = i2c_a1234_remove,
};
module_platform_driver(i2c_a1234_driver);
	...
	...
	...