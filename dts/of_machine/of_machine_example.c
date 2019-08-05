	...
	...
	...
static int exynos_cpufreq_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	
	exynos_info = kzalloc(sizeof(*exynos_info), GFP_KERNEL);
	if (!exynos_info) 
		return -ENOMEM;
	
	exynos_info->dev = &pdev->dev;
	
	if (of_machine_is_compatible("samsung,exynos4210")) {
		exynos_info->type = EXYNOS_SOC_4210;
		ret = exynos4210_cpufreq_init(exynos_info);
	} else if (of_machine_is_compatible("samsung,exynos4212")) {
		exynos_info->type = EXYNOS_SOC_4212;
		ret = exynos4x12_cpufreq_init(exynos_info);
	} else if (of_machine_is_compatible("samsung,exynos4412")) {
		exynos_info->type = EXYNOS_SOC_4412;
		ret = exynos4x12_cpufreq_init(exynos_info);
	} else if (of_machine_is_compatible("samsung, exynos5250")) {
		exynos_info->type = EXYNOS_SOC_5250;
		ret = exynos5250_cpufreq_init(exynos_info);
	} else {
		pr_error("%s:Unknow Soc type\n", __func__);
		return -ENODEV;
	}
	...
	...
	...
	
}