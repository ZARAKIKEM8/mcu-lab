# STM32 定时小风扇

工程名：Fan  
主控：STM32F103C8T6  
开发环境：STM32CubeIDE 1.18.1  
电机驱动：DRV8833  
显示：SSD1306 OLED，128×64，I2C

## 一、功能

- 定时时间 5～999 秒，每次加减 5 秒。
- 三档风速，PWM 占空比分别为 60%、80%、99%。
- 启动时先以满占空比运行 250 ms，帮助电机起转。
- 运行中按 PB11 可以直接换挡，风扇不会停止。
- 倒计时结束后关闭电机，蜂鸣器响一声，屏幕显示完成状态。
- 完成画面保持 2 秒，然后回到定时设置界面。

## 二、接线

| 模块 | 模块引脚 | STM32F103C8T6 |
|---|---|---|
| OLED | VCC | 3.3 V |
| OLED | GND | GND |
| OLED | SCL | PB6 |
| OLED | SDA | PB7 |
| DRV8833 | VM | USB 5 V |
| DRV8833 | GND | 公共 GND |
| DRV8833 | AIN1 | PA6 |
| DRV8833 | AIN2 | PA4 |
| DRV8833 | STBY | PA5 |
| DRV8833 | AOUT1、AOUT2 | 直流电机 |
| 蜂鸣器 | VCC | 3.3 V 或模块标注电压 |
| 蜂鸣器 | GND | 公共 GND |
| 蜂鸣器 | IO | PB12 |
| 时间加按键 | 一端 / 另一端 | PB1 / GND |
| 时间减按键 | 一端 / 另一端 | PB10 / GND |
| 调速按键 | 一端 / 另一端 | PB11 / GND |
| 启动按键 | 一端 / 另一端 | PB0 / GND |

所有 GND 必须共地。电机电源接 USB 5 V，不能用 ST-Link 的 3.3 V 给电机供电。

## 三、按键操作

- PB1：定时时间加 5 秒。
- PB10：定时时间减 5 秒。
- PB11：在 1、2、3 档之间切换。
- PB0：启动倒计时。

## 四、编译

1. 打开 STM32CubeIDE 1.18.1。
2. 选择 `File → Import → Existing Projects into Workspace`。
3. 选择本目录下的 `Fan` 工程。
4. 右键工程，选择 `Build Project`。
5. 编译结果位于 `Fan/Debug/Fan.elf`、`Fan/Debug/Fan.hex` 和 `Fan/Debug/Fan.bin`。

本工程没有依赖外部绝对路径。工程已经使用 STM32CubeIDE 1.18.1 自带的 GNU Arm 13.3 工具链编译通过。

## 五、烧录

使用 ST-Link 的 SWD 接口：

| ST-Link | 开发板 |
|---|---|
| 3.3 V | 3.3 V |
| GND | GND |
| SWDIO | PA13 |
| SWCLK | PA14 |

可以直接在 CubeIDE 中点击 `Run`，也可以在 STM32CubeProgrammer 中加载 `Fan/Debug/Fan.hex`。

开发板使用 USB 供电时，ST-Link 的 3.3 V 只作为参考电平，不要用 ST-Link 的 3.3 V 驱动电机。

## 六、程序结构

| 文件 | 内容 |
|---|---|
| `Core/Src/main.c` | 状态机、按键、定时、PWM 和蜂鸣器 |
| `Core/Src/oled.c` | SSD1306 显存和字符显示 |
| `Core/Src/stm32f1xx_hal_msp.c` | I2C、TIM2、TIM3 和 GPIO 底层初始化 |
| `Core/Src/stm32f1xx_it.c` | 系统节拍和 TIM2 中断入口 |
| `Fan.ioc` | 芯片、时钟和引脚配置 |

主要参数：

- 系统时钟：72 MHz。
- TIM2：1 秒中断，用于倒计时。
- TIM3：10 kHz PWM，用于电机调速。
- I2C1：100 kHz，用于 OLED。
- 电机启动助力：250 ms 满占空比。

## 七、检查顺序

1. 先只接 OLED，确认屏幕能显示设置界面。
2. 再接蜂鸣器，按启动后等待倒计时结束，确认响一声。
3. 最后接 DRV8833 和电机，先架空电机测试。
4. 若 OLED 黑屏，检查 VCC、GND、PB6、PB7，并确认模块地址为 0x78 或 0x7A。
5. 若电机不转，检查 VM 是否有 5 V、GND 是否共地、STBY 是否为高电平。

当前提交包完成了源码与链接检查，硬件演示前仍需按实际接线复核。
