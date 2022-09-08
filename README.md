# Cube-443_OSC-Tweezer
The tweezer  with  Simple oscilloscope  / 带有示波功能的简单镊子

## 硬件开源地址：[https://oshwhub.com/an_ye/cube-443_-shi-bo-nie-zi](https://oshwhub.com/an_ye/cube-443_-shi-bo-nie-zi)

## ADC差分输入带宽500Mhz , 由于65MSPS版本ADC缺货，采样率目前限制在30M左右

![](https://image.lceda.cn/pullimage/IOXrYDXw5zKor74D8qfbERfzB7PgLr1lAFisTa4o.jpeg)

![](https://image.lceda.cn/pullimage/WHvOA4xFxJ0nBkd4fLX5YLlbj4mYPZUi9IUmjZtk.jpeg)

![](https://image.lceda.cn/pullimage/Cib4ycKxuxZ9aj7nVQvAesIs0Vze9iPj4G22dC11.jpeg)

![](https://image.lceda.cn/pullimage/C9iZmSYddcv01WCS3hs06oxpPU2nq84hHJJ9wRja.jpeg)

![](https://image.lceda.cn/pullimage/yzMJZyMQrdR2CDMquQNuw6RVLryEtFYf9BbOOXyc.jpeg)

代码开源仓库：「[https://github.com/panyihang/Cube-443_OSC-Tweezer](https://github.com/panyihang/Cube-443_OSC-Tweezer)」
<span class="colour" style="color: rgb(45, 194, 107);">如需合作请联系邮箱 <u>[\<span class="colour" style="color: rgb\(255, 39, 233\);"\>\<u\>root@an-ye.top\</u\>\</span\>](mailto:root@an-ye.top)</u></span>
<span class="colour" style="color: rgb(230, 126, 35);">Q群 565264047 欢迎来玩呀～</span>

# 0x00:前言

第一版被官方库坑了，编码器封装错误，需翘脚飞线解决，飞线后感觉不太稳
![](https://image.lceda.cn/pullimage/wCwH8vWdWJBYp7zYXT6eWTqOzZakTpLxfcdEVLCC.jpeg)

电池容量为60mah，具体尺寸如下
![](https://image.lceda.cn/pullimage/0HdVp7e0zFRfTZV45eLscIMsHl86jVWWl4zt5uxd.jpeg)![](https://image.lceda.cn/pullimage/SmjTsgL9MSXKD9oIoFQQvJogn8qZ4gLjlvazRBwx.jpeg)
适配电池版的外壳

屏幕参数
1\.47寸 st7789 ips 分辨率172\*320 ，实际驱动修改为174\*322
![](https://image.lceda.cn/pullimage/HHQPfVquIOyD2mN2ChpinCGhDfZT3qdYOeIVPnxC.jpeg)

#

# 0x01:硬件版本说明

「2022.08.07 Ver0.2」调整布线
「2022.09.01 Ver0.3」修复编码器封装BUG
「2022.09.02 Ver1.3」新增两层版本，新增彩色丝印，修改ADC处部分电阻

![](https://image.lceda.cn/pullimage/ONpCkgh636Aprr3h3if8ReHtLfAyHe9V88aVBeNW.png)

# 0x02:硬件

主控MCU：

* 树莓派 RP2040
* 核心电压 1.2V
* 系统频率 266Mhz

ADC：

* AD9235BCPZ-65（由于65M版本SMT无货，实际使用AD9235BCPZ-45）
* 输入信号范围 0V - 3.45V

屏幕：

* 1.47寸 IPS TFT
* 分辨率172*320
* 屏幕未知原因发烫，可能是批次问题

存储：

* 128Mb SPI Flash
* 用于存放固件
* 可存放波形文件

# 0x03:软件

编译：

* 编译器为树莓派提供的c sdk
* 使用vs code的PlatformIO扩展进行开发
* 使用的PIO开发包为WizIO-PICO
* -Ofast 优化

为提高效率，修改了编译用的c sdk ，使用官方SDK理论上可以正常编译（现已基本替代，可直接编译）
<span class="colour" style="color: rgb(224, 62, 45);">注：谨慎运行clean all命令，否则会因找不到freeRTOS相关头文件导致编译不过，建议使用clean清除编译</span>
GUI基于LVGL8.2.0，UI使用NXP的Gui-Guider生成并修改

# 0x04:外壳

图为 粉色 PETG 打印，料受潮瑕疵略多
旋钮壳用了个诡异的方法，用LCEDA画出来了
![](https://image.lceda.cn/pullimage/0eXrj6c3xqdacwoqPzgXiB0XNeF0JcG5MdjWpc5c.jpeg)
需要打印的模型：
![](https://image.lceda.cn/pullimage/BSx8tGX5rarcDwXDdHDspvfADFhkSWBfdZCR6esx.jpeg)

# 0x05:烧录说明

「快捷方法」

* 首次上电前确保FLASH为空片
* 使用USB连接PC
* 自动识别到大容量存储设备
* 将`APPLICATION.uf2`复制到新增的存储设备，等待自动重启

「常规方法」

* 外壳全部拆开


* 按住板载按钮
* 使用USB连接PC
* 松开板载按钮
* 识别到大容量存储设备
* 将`APPLICATION.uf2`复制到新增的存储设备，等待自动重启

# 0x06:固件更新说明

「快捷方法」(需运行本项目已有固件）（超频失败也可使用）

* 准备镊子 / 牙签 / 细铜线 * 1
* 使用USB连接PC
* 原固件正常运行
* 戳下图的小孔，确保按钮被按下，按下时间 >= 200ms
* 自动识别到大容量存储设备
* 将`APPLICATION.uf2`复制到新增的存储设备，等待自动重启![](https://image.lceda.cn/pullimage/CDKBozfjsr5rfL6QeRJXzkQUSmwX4khUKesuWi3c.jpeg)

「常规方法」（免拆外壳）

* 准备镊子 / 牙签 / 细铜线 * 1
* 使用USB连接PC
* 戳图中小孔，确保按钮被一直按下
* 拨动两次侧面的拨动开关
* 自动识别到大容量存储设备
* 将`APPLICATION.uf2`复制到新增的存储设备，等待自动重启
* 缺点：较难操作

「最常规的方法」

* 外壳全部拆开
* 按住按钮
* 使用USB连接PC
* 自动识别到大容量存储设备
* 将`APPLICATION.uf2`复制到新增的存储设备，等待自动重启

# 0x07:装配说明

需要材料：

* 镊子主板 * 1
* 上下壳 * 1
* 旋钮壳 * 1
* M3*8mm  沉头螺丝 * 2
* 镊子尖 * 2 （不存在)

![](https://image.lceda.cn/pullimage/JSbtb759PQxzMyDgwfemRkvtcfPtIZBG9sepmiJv.jpeg)

#

# 0x08:充电 & 续航

经测试，在266Mhz @ 1.3V ，背光最大亮度的情况下，约20min后电压不足，开始反复重启
![](https://image.lceda.cn/pullimage/wGnGFjSgIcaR7szpA4ZS4ppJnVcdOz3xIf5UgDlc.jpeg)
充电需10min，目前充电IC存在BUG，在充满后会不停切换充电和待机

#

# 0x09:温度

外部供电时屏幕整体温度较高，驱动芯片处烫手
![](https://image.lceda.cn/pullimage/YSsyqVFYHS3tW7TEMmt4kDplwQ1PDQL4AMUhnKIg.jpeg)
电池供电暂未发现明显发热点

# ![](https://image.lceda.cn/pullimage/SJ2Dk8c3CkMwKHe2ETWOZbxHqb1iP43hDKRhqXqo.jpeg)

# 0x0A:重量

![](https://image.lceda.cn/pullimage/IJ0c3xyXZrJpaOgjavBhXrkdjRGsNXxxUFCMND9A.jpeg)
主体18g，emmm 太轻了，下个项目考虑加大电池容量，并加入电桥功能

# 0x0B:提醒

信号源建议有接地，否则测量不准
