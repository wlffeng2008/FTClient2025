#指令名称, #指令内容, #是否需要手动pass, #指令写入参数, #返回核对参数(返回值与该值比对一致为ok), #指令执行超时时间(s,0为默认), # 按钮文字
获取mac地址, dev_get_mac, 0, null, null, 30, 读取
获取版本号, dev_get_version, 0, null, V1.0.90.241030, 30, 读取
获取电量值, dev_get_battery, 0, null, null, 30, 读取
获取4G信号, module4g_signal, 0, null, null, 30, 读取
TF卡检测, dev_tf_detection, 0, null, null, 30, TF卡检测
蓝牙检测, dev_ble_detection, 0, null, null, 30, 蓝牙检测
音频检测, audio_detection, 0, null, null, 30, 音频检测
灯光检测, dev_led_detection, 0, null, null, 30, 灯光检测
PIR检测, dev_pir_detection, 0, null, null, 60, PIR检测
按键检测, dev_key_detection, 0, null, null, 60, 按键检测
夜间全彩, video_colorful_detection, 0, null, null, 30, 全彩测试
OTA升级, ota_upgrade_start, 0, http://47.106.117.102:8888/download/ota_TS103K_1.0.97_20241126.bin, null, 30, 切换
