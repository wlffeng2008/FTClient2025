#指令名称, #指令内容, #是否需要手动pass, #指令写入参数, #返回核对参数(返回值与该值比对一致为ok), #指令执行超时时间(s,0为默认), # 按钮文字
读取版本号, read_version, 0, null, V1.0.0.35.250218, 1, 读取
灯打开, led_on, 1, null, null, 1, 测试
灯关闭, led_off, 1, null, null, 1, 测试
按键检测, key_detect, 1, null, null, 1, 测试
TF卡检测, tfcard_detect, 1, null, null, 1, 测试
wifi吞吐量, wifi_throughput, 1, null, null, 1, 测试
GPIO设置, gpio_set, 1, {"gpio": 23, "value": 1}, null, 1, 测试
