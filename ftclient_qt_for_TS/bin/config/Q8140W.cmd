#指令名称, #指令内容, #是否需要手动pass, #指令写入参数, #返回核对参数(返回值与该值比对一致为ok), #指令执行超时时间(s,0为默认)
读取版本号, dev_get_version, 0, null, 1.1.15, 30, 读取
读取电量, dev_get_battery, 0, null, null, 30, 读取
蓝牙检测, dev_ble_detection, 1, null, null, 30, 测试
TF卡检测, dev_tf_detection, 1, null, null, 30, 测试
灯光检测, dev_led_detection, 1, null, null, 30, 测试
WiFi吞吐, dev_wifi_throughput, 1, null, null, 30, 测试
拾音播放, audio_detection, 1, null, null, 30, 测试
按键检测, dev_key_detection, 1, null, null, 60, 测试
PIR检测, dev_pir_detection, 1, null, null, 60, 测试
夜间全彩, video_colorful_start, 1, null, null, 30, 切换