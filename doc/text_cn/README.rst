==================
Multi-Mode keyboard
==================

.. contents::
   :depth: 2

概述
=======

本文档旨在给用户介绍基于Realtek Zephyr SDK和RTL87x2G系列soc开发多模键盘的方案。键盘目前使用的是zmk架构，zmk全称是Zephyr Mechanical Keyboard, 具有较为完备的keyboard功能，它由开源社区开发和维护。

关于zephyr的介绍可参考：`https://docs.zephyrproject.org/latest <https://docs.zephyrproject.org/latest/>`_

关于zmk的介绍可参考：`https://zmk.dev <https://zmk.dev/>`_

支持功能
-----------

-  支持BLE/2.4G/USB三种通信模式
-  BLE/2.4G/USB三种模式的最高上报率为125Hz/4KHz/8KHz
-  支持Full/High speed USB
-  支持GPIO和Keyscan按键，Keyscan最多可扩展到240个按键(12x20)
-  支持PWM输出控制LED
-  支持电量检测
-  支持基于USB的DFU升级

Zephyr SDK 准备
-----------

.. note:: 
   - 本文档介绍如何在主机上交叉编译和生成 zephyr 二进制映像。请注意，当前的 Realtek Zephyr SDK 环境基于 Windows 10。因此，接下来的指南将基于 Windows。对于 MacOS 和 Linux 用户，请参阅 Zephyr 官方文档。

1. | 参考zephyr官方的文档，，安装相关的依赖。
   | `https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies`_。
   | 需要确保下面表格中列出的工具均已安装成功：

.. csv-table::
   :header: 工具名, 最低版本
   :widths: 15 15
   :align: center

   CMake, 3.30.5
   Python, 3.8
   Devicetree compiler, 1.4.6
   Zephyr sdk bundle, 0.16.8
   Chocolatery, /
   west, /
   Ninja, /
   Gperf, /
   Git, /
   Wget, /
   7zip, /

2. | 下载Realtek Zephyr SDK
   | Realtek Zephyr SDK总共包含四个repo `https://github.com/rtkconnectivity <https://github.com/rtkconnectivity/>`_

   **zephyr:** 基于 `https://github.com/zephyrproject-rtos/zephyr <https://github.com/zephyrproject-rtos/zephyr/>`_ 以及Realtek的适配

   **hal_realtek:** Realtek soc 的hal模块

   **zmk:** 基于 `https://github.com/zmkfirmware/zmk <https://github.com/zmkfirmware/zmk/>`_ 以及Realtek的适配

   **realtek-zephyr-project:** 存放基于zephyr os实现的Realtek的应用实例

   获取Realtek Zephyr SDK, 打开windows自带的cmd.exe, 参考以下命令

   通过git clone命令来拉取ZMK repo

   .. code-block:: c

      git clone --branch zmk_rtk https://github.com/rtkconnectivity/zmk.git

   需要把zmk/app作为manifest repo workplace

   .. code-block:: c

      west init -l zmk/app

   拉取zmk编译所需要的zephyr, hal_realtek等其他repos

   .. code-block:: c

      west update 

   基于board:rtl8762gn_evb，查看编译的配置选项

   .. code-block:: c

      cd zmk/app

      west build -p -b rtl8762gn_evb -t menuconfig -- -DSHIELD=rtk_keyboard 

   Zephyr通过多层级的Kconfig机制来管理内核，驱动层以及应用层所需的配置，用户可以通过menuconfig在线修改相关配置

.. figure:: ../figures/zmk_menuconfig.*
   :align: center
   :scale: 35%
   :name: zmk编译配置选项

   zmk编译配置选项

编译示例工程，编译出的zmk.bin以及zmk.hex在路径build/zephyr下

.. code-block:: c

  west build -b rtl8762gn_evb -- -DSHIELD=rtk_keyboard 

环境需求
==========

.. tabs::

    .. tab:: 硬件环境需求

          1. RTL87x2G EVB（母板+子板）
          2. J-link
          3. FT232_serial_port_board

    .. tab:: 工具需求

          1. BeeMPTool_kits: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\MPTool`
          2. DebugAnalyzer: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\DebugAnalyzer`
          3. CFUDownloadTool: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\CFUDownloadTool`
          4. MPPackTool: :file:`BEE4-SDK-KEYBOARD-vx.x.x\\tools\\BeeMPTool\\BeeMPTool\\tools\\MPPackTool`
          5. Serial Terminal tool(such as mobaxterm)

工具需求说明
--------------

在SDK的 :file:`\\tools` 目录下，安装包是.zip格式的压缩包，用户需要对其解压。串口工具需要用户自行下载，用于显示zephyr log

下载固件到IC
~~~~~~~~~~~~~~~

除了上文编译出的 :file:`zmk.bin` 和 :file:`zmk.hex`, RTL87x2G还需要烧录以下images，

.. csv-table::
   :header: Image名, 描述
   :widths: 15 80
   :align: center

   System Config file, 记录有关硬件配置和蓝牙配置的信息，如配置 BT 地址、更改链路数等。配置文件可使用 MPTool 生成
   OTA Header file, 定义了flash布局，由MPTool生成
   BOOT Patch Image, Realtek发布，ROM code中保留了Patch函数入口，通过Patch可以优化Boot flow、修改secure ROM code原有行为、扩充ROM code功能等
   System Patch Image, Realtek发布，ROM code中保留了Patch函数入口，通过Patch可以修改non-secure ROM code原有行为、扩充ROM code功能等
   BT Stack Patch Image, Realtek发布，通过Patch可扩充ROM code功能，增加BT Controller中stack相关feature的支持

对于zephyr app image, 有两种烧录方式，一种是通过MPTool烧录，一种是通过 “west flash” 烧录。


- MPtool烧录

可以将编译出的zmk二进制文件提供MPTool烧录到flash中，地址参考 :file:`flash_map.h`， 同时需要与 :file:`zmk\\app\\boards\\arm\\xxx.dts` 中对flash布局的定义保持一致。 此外用户可以根据自己需求使用MPTool中
的FlashMapGenerateTool修改flash map。

.. figure:: ../figures/download_images_directory.*
   :align: center
   :scale: 50%
   :name: zmk download images

   zmk download images
      
zmk中目前使用的flash map作如下说明：

   * ZMK default flash layout header file: :file:`zmk\\app\\download_images\\flash_map.h`
   * ZMK default flash layout configuration file: :file:`zmk\\app\\download_images\\flash_map.ini`

.. csv-table:: ZMK Flash layout
  :header: ZMK layout with a total flash size of 1MB,Size(byte),Start Address
  :widths: 130 50 50
  :name: ZMK Flash layout
  
  Reserved,4K,0x04000000
  OEM Header,4K,0x04001000
  Bank0 Boot Patch,32K,0x04002000
  Bank1 Boot Patch,32K,0x0400A000
  OTA Bank0,488K,0x04012000
  - OTA Header,4K,0x04012000
  - System Patch code,32K,0x04013000
  - BT Lowerstack Patch code,72K,0x0401B000
  - BT Host code,0K,0x0402D000
  - APP code,308K,0x0402D000
  - APP Config File,0K,0x0407A000
  - APP data1,0K,0x0407A000
  - APP data2,0K,0x0407A000
  - APP data3,0K,0x0407A000
  - APP data4,0K,0x0407A000
  - APP data5,0K,0x0407A000
  - APP data6,72K,0x0407A000
  OTA Bank1,0K,0x0408C000
  Bank0 Secure APP code,0K,0x0408C000
  Bank0 Secure APP Data,0K,0x0408C000
  Bank1 Secure APP code,0K,0x0408C000
  Bank1 Secure APP Data,0K,0x0408C000
  OTA Temp,308K,0x0408C000
  FTL,16K,0x040D9000
  APP Defined Section,0K,0x040DD000

.. important::
  - To adjust flash layout, flow the steps listed on the :ref:`Generating Flash Map` in :Doc:`Quick Start <../../../quick_start/text_en/README>`
  - After flash layout adjustment, you must flow the steps listed on the :ref:`Generating OTA header` and :ref:`Generating System Config File` with new flash layout file.

有两种方式可以将编译出的zmk二进制文件烧录到flash中，一是以user data的方式烧录：

.. figure:: ../figures/user_data_download.*
   :align: center
   :scale: 50%
   :name: user data download method

   user data download method

二是通过运行 :file:`zmk\app\tools\AppData\ZmkApp\run.bat`，将zmk.bin打包成带mp header的格式，以app image通过MPTool烧录

.. figure:: ../figures/mp_tool_download.*
   :align: center
   :scale: 50%
   :name: mpheader download method

   mpheader download method

- west flash烧录

需要配置好jlink并将jlink连接到cpu, 输入命令“west flash”即可将zmk image下载到flash中

.. code-block:: c

  west flash

LOG
~~~~~~~~~~~~~~~

zmk的log通过uart终端工具显示，可通过CONFIG_ZMK_LOG_LEVEL调整打印等级，默认为4，等级的说明如下：

.. csv-table::
   :header: API, Level, Value
   :widths: 20 15 15
   :align: center

   LOG_DBG, DEBUG, 4
   LOG_INF, INFO, 3
   LOG_WRN, WARN, 2
   LOG_ERR, ERROR, 1

.. note:: 
   - CONFIG_ZMK_LOG_LEVEL的调整可控制打印等级，如调整为3，所有LOG_DBG的内容不会打印

Update
~~~~~~~~~~~~~~

1. | 用户通过MPTool烧录appdata6 image。
   | 路径: :file:`BEE4-SDK-Zephyr \\zmk\\app\\download_images\\appdata6\\app_MP_sdk_XX.bin`

2. | 用户需要将指定的PIN拉高(目前zmk使用的是P2_5), reset IC, 执行appdata6，进入usb dfu升级模式。

MPPack Tool
^^^^^^^^^^^^^^^

1. | 用户通过MPPackTool对设备升级文件进行打包处理。

2. 双击运行 :file:`MPPackTool.exe`， :guilabel:`IC Type` 选择 :guilabel:`RTL87x2G_VB`，选择 :guilabel:`ForCFU`，点击 :guilabel:`Browse`，选择需要升级的文件，以 :file:`Bank0 APP Image` 为例，如下图所示。

.. figure:: ../figures/MPPackTool_Interface.*
   :align: center
   :scale: 70%
   :name: MPPackTool界面

   MPPackTool界面

.. hint:: 
   待升级的文件大小不能超过设置的OTA Tmp区大小（参考工程里的 :file:`flash_map.h`），如果待升级总文件的大小超出OTA Tmp区，需要分批次打包，然后逐包升级。由于Patch，Stack等文件很少更新，用户一般只需要单独打包APP Image即可。

3. | 文件加载完成后，点击 :guilabel:`Confirm`，会生成 :file:`ImaPacketFile.offer.bin` 和 :file:`ImgPacketFile.payload.bin` 两份文件。

.. note:: 
   - 勾选 :guilabel:`save patch`，可以选择生成CFU文件的保存目录，不勾选默认存在根目录下。
   - 打包量产烧录文件以及其他更详细的使用说明，请参阅SDK工具目录下的用户指南，也可以前往 `RealMCU <https://www.realmcu.com/en/Home/DownloadList/c175760b-088e-43d9-86da-1fc9b3f07ec3>`_ 平台获取相应工具，并查阅提供的文档。

CFUDownloadTool
^^^^^^^^^^^^^^^^

1. | 用户可以通过CFUDownloadTool对设备进行程序升级。

2. 打开 :file:`CFUTOOLSettings.ini` 文件，对升级设备进行参数设置，如下所示。

   1. RTL87x2G升级方式采用CFU_VIA_USB_HID
   2. Keyboard：Vid=0x0BDA，Pid=0x4762
   3. Dongle：Vid=0x0BDA，Pid=0x4762

.. highlight:: rst

::

   [CFU_VIA_USB_HID]
   Vid=0x0bda
   Pid=0x4762
   UsagePage=0xff0b
   UsageTlc=0x0104

   [CFU_EARBUD_VIA_BT_HID]
   Vid=0x005d
   Pid=
   UsagePage=0xff0b
   UsageTlc=0x0104

   [CFU_EARBUD_VIA_DONGLE]
   Vid=0x0bda
   Pid=0x4762
   UsagePage=0xff07
   UsageTlc=0x0212

   [ICTypeSelect]
   TYPE=1

   [CFUTypeSelect]
   Type=0

   [MainSetting]
   ImageDir=BEE4-SDK-keyboard\applications\trimode_keyboard\proj\mdk\images\app\cfu
   TransDelay=0
   TransTimeout=200
   ForceReset=1

   [DEVICE]
   SerialNumber=

.. highlight:: none

.. hint:: 
   - 如果配置的Vid和Pid与Keyboard/Dongle设置不一致，CFUDownloadTool会识别不到设备。
   - TransDelay可设置两笔数据包之间的延迟时间。
   - TransTimeout可设置response超时时间。
   - 通过设置SerialNumber，可以在VID、PID相同时，区分升级的是dongle还是keyboard，如果不填写，则代表不区分。

3. 双击运行 :guilabel:`CFUDownloadTool.exe`，如下图所示。

   - :guilabel:`IC Type` 选择: :guilabel:`RTL87x2G`
   - :guilabel:`CFU Type` 选择: :guilabel:`CFU via USB HID`

.. figure:: ../figures/CFU_Download_Tool_interface.*
   :align: center
   :scale: 80%
   :name: CFU Download Tool界面

   CFU Download Tool界面

4. 将设备与电脑连接，如果设备识别成功，如下图所示。

   1. “Found 1 device”在界面右侧显示。
   2. “FwVersion”表示当前Keyboard/Dongle的App Image版本。
   3. “Current Bank 2”表示Single Bank升级方案（当前仅支持此方案）。

.. figure:: ../figures/CFU_Download_Tool_Device_identification_interface.*
   :align: center
   :scale: 70%
   :name: 图片-CFU Download Tool设备识别界面

   CFU Download Tool设备识别界面

5. 在 :guilabel:`CFU Image` 处加载需要升级的文件所在文件夹，如 \ :ref:`图片-CFU Download Tool设备识别界面`\  和 \ :ref:`图片-CFU文件`\  所示。

.. figure:: ../figures/CFUDownload_Files.*
   :align: center
   :name: 图片-CFU文件

   CFU文件

6. 点击 :guilabel:`Download`，进度条会显示当前程序下载进度，下载完成显示“OK”。

硬件连线
===========

.. _multi-mode keyboard RTL87x2G EVB:

RTL87x2G EVB
---------------

EVB评估板提供了用户开发和应用调试的硬件环境。EVB由主板和子板组成。它有下载模式和工作模式，具体使用请参考 \ :Doc:`快速入门 <../../../../doc/quick_start/text_cn/README>`\  的硬件开发环境这一章节。

RTL87x2G 三模键盘样机
--------------------

.. figure:: ../figures/Keyboard_Wiring_Diagram.*
   :align: center
   :scale: 30%
   :name: 三模键盘样机引线

   三模键盘样机引线

下载模式
~~~~~~~~~~~

1. 设备上电前，用户需要先将引出的Log Pin接地，设备的Log Pin、GND Pin和 \ :ref:`FT232串口转接板`\  的GND要共地；引出的Tx接串口转接板的Rx，Rx接串口转接板的Tx；VBAT接3.3V电源，串口转接板连接PC进行供电。

   .. figure:: ../figures/FT232_serial_port_board.*
      :align: center
      :scale: 35%
      :name: FT232串口转接板

      FT232串口转接板

2. 进入下载模式后，请使用MPTool进行程序烧录。需注意：

   1. 芯片在上电后会读取Log Pin的电平信号，如果电平为低，则Bypass Flash，进入烧录模式，否则运行应用层程序。
   2. 因为芯片烧录需要使用1M波特率，务必使用FT232的串口转接板，否则可能会出现UART Open Fail的现象。

Log接线
~~~~~~~~~~

设备引出来的UART Tx Pin连接串口转接板的Rx，GND连接串口转接板的GND，电脑需要和设备连接进行供电，连接完成后，请打开串口终端工具，查看Log输出。

.. hint:: 
   - zephyr log目前是通过UART2_TX打印，具体配置请参考 :file:`rtl87xxg-pinctrl.dtsi` 中对于UART2的配置。
   - 注意串口波特率默认设置为2M，串口工具打开串口时需要同步设置为2M，否则Log会出现乱码。

软件设计介绍
==============

本章主要介绍Zephyr Tri-Mode keyboard解决方案的软件相关技术参数和行为规范，为Tri-Mode keyboard的所有功能提供软件概述，包括三种模式（USB/2.4G/BLE）、按键、电量检测和充电、灯效、产测等行为规范，用于指导Tri-Mode keyboard的开发和追踪软件测试中遇到的问题。

源代码目录
-------------

ZMK中的源文件目录如下：

.. highlight:: rst

::

   └── Project: zmk           
       └── app                                      
            └── boards                             boards defines the PCB that includes the MCU
                └── arm 
                     ├── rtl8762gn_evb
                          ├── xxx.defconfig
                          ├── xxx.dts
                          ├── xxx.yaml
                          └── ...
                     ├── rtl8762gku_kb
                     ├── rtl8762gtu_kb
                     └── ...
                ├── interconnects                  use for composite keyboards which the controller board is seperated with switch footprints
                └── shields                        containing all the keys, RGB LEDs, encoders etc
                  └── rtk_keyboard
                        ├── rtk_keyboard.keymap
                        ├── rtk_keyboard.conf
                        ├── rtk_keyboard.overlay
                        └── ...
                  └── ...
            └── dts                                device tree source: use human-readable text to describe hardware
               └── behaviors                       support keyboard different behaviors
                  ├── key_press.dtsi
                  └── ...
               └── bindings
                  └── xxx.yaml                     define the parameters used in dts
            ├── include 
            ├── keymap-module                      Looking for keymap related files when compiling
            ├── module                             support extended driver
            └── scripts                            support west commands and Python's requirements
               ├── requirements.text
               └── west-commands.yml
            └── src                                includes app source code
               ├── behaviors                       keyboard behaviors code realization
               ├── display                         
               ├── events                          monitor events which app concerns
               ├── leds       
               ├── mp_test                         support realtek mp test
               ├── ppt                             support realtek 2.4g transeive
               ├── settings
               ├── split
               ├── activity.c
               ├── app_wdt.c                       watchdog config
               ├── backlight.c
               ├── battery.c
               ├── behavior_queue.c
               ├── behavior.c 
               ├── ble.c                           ble config
               ├── combo.c 
               ├── conditional_layer.c 
               ├── endpoints.c                     manage transport methods
               ├── event_manager.c 
               ├── ext_power_generic.c
               ├── hid_indicators.c 
               ├── hid_listenser.c 
               ├── hid.c 
               ├── hog.c                          bt transeive code
               ├── keymap.c                       process keymap
               ├── kscan.c                        
               ├── main.c
               ├── matrix_transform.c 
               ├── mode_monitor.c                 monitor keyboard working mode
               ├── keyboard.c
               ├── ppt.c 
               ├── rgb_underglow.c 
               ├── sensors.c 
               ├── stdlib.c
               ├── usb_cdc.c                      support usb communication device Class
               ├── usb_hid.c
               ├── usb.c 
               ├── workqueue.c 
               └── wpm.c 
            ├── tests                             include keyboard features tests
            ├── tools                             include tools for realtek usb dfu
            ├── CMakeLists.txt
            ├── Kconfig
            ├── VERSION                           define app version
            ├── west.yml                          

.. highlight:: none

软件架构
-----------

系统软件架构如下图所示。

.. figure:: ../figures/zmk_software_architecture.*
   :align: center
   :scale: 70%
   :name: ZMK软件架构图

* **Platform**: 包括OTA、Flash、:term:`FTL` 等。
* **IO Drivers**: 提供对RTL87x2G外设接口的应用层访问。
* **OSIF**: 实时操作系统的抽象层。

任务和优先级
--------------

如下图所示，应用程序共创建了六个任务：

.. figure:: ../figures/zmk_task_and_priority.*
   :align: center
   :scale: 80%

   Tasks and Priority

各任务描述及优先级如下表：

.. csv-table::
   :header: 任务, 描述, 优先级
   :widths: 15 30 10
   :align: center

   Realtek PM, 管理 :term:`DLPS` 退出操作, -16
   Logging, 管理Log机制, 14
   Rx Thread, 处理:term:`HCI`收到的数据, 0
   BT Controller stack, 实现 :term:`HCI`以下的BLE协议栈, -8
   Long workqueue, BLE HOST处理ECC command, 10
   System WorkQueue, 系统工作队列, -1
   Low priority workqueue, 用来处理较低优先级的任务，如电池电量更新，灯效, 10
   HID Over GATT, 管理BLE数据的发送, 5
   Idle, 运行后台任务，包括 :term:`DLPS`, 14

.. note::
  - 可以创建多个应用任务，并相应地分配内存资源。
  - Zephyr RTOS任务分为协作式任务和抢占式任务。协作式任务不可被抢占，抢占式任务可以被更高优先级的抢占式任务或者协作式任务抢占。
  - A cooperative thread has a negative priority value. Once it becomes the current thread, a cooperative thread remains the current thread until it performs an action that makes it unready.
  - A preemptible thread has a non-negative priority value. Once it becomes the current thread, a preemptible thread may be supplanted at any time if a cooperative thread, or a preemptible thread of higher or equal priority, becomes ready.

程序初始化处理流程
--------------------

键盘上电后，IC的初始化过程如下图所示

.. figure:: ../figures/boot_flow.*
   :align: center
   :scale: 80%
   :name: 程序初始化流程

   程序初始化流程

zephyr中通过 :func:`SYS_INIT` 来管理各个模块的初始化，初始化等级依次为 `EARLY`, `PRE_KERNEL_1`, `PRE_KERNEL_2`, `POST_KERNEL`, `APPLICATION`，执行完后进入 :func:`main` 函数

   .. code-block:: c

      /**
      * @brief Register an initialization function.
      *
      * The function will be called during system initialization according to the
      * given level and priority.
      *
      * @param init_fn Initialization function.
      * @param level Initialization level. Allowed tokens: `EARLY`, `PRE_KERNEL_1`,
      * `PRE_KERNEL_2`, `POST_KERNEL`, `APPLICATION` and `SMP` if
      * @kconfig{CONFIG_SMP} is enabled.
      * @param prio Initialization priority within @p _level. 
      */
      SYS_INIT(init_fn, level, prio)


Main函数
~~~~~~~~~~~

:func:`main` 函数中初始化过程包括：、app dlps check函数注册，reset reason检查，keyscan初始化，BLE/2.4G/USB三种模式相关的初始化。

.. list-table:: main初始化流程相关函数
   :header-rows: 1

   * - :term:`API`
     - 功能模块

   * - app_dlps_check_cb_register()
     - app注册dlps check function, 用来管控进入dlps的时机。

   * - single_tone_init()
     - 如果reset reason是SWITCH_TO_TEST_MODE, 执行single tone相关的初始化。

   * - zmk_kscan_init()
     - 初始化kscan模块。

   * - zmk_ble_init()
     - 当处于BLE mode时，执行ble相关初始化。

   * - zmk_ppt_init()
     - 当处于2.4G mode时，执行2.4g相关初始化。

键盘按键处理流程
---------------------

ZMK中按键按下后，会触发kscan中断，在中断中会对当前按键做处理，过滤鬼键以及重复按键，通过callback的形式通知app :func:`zmk_kscan_callback`，从keymap中寻找对应的 `behavior` 以及HID键值, 通过 `endpoint` 选择BLE/2.4G/USB中一种传输方式发送键盘数据。

.. figure:: ../figures/keypress_handle_flow.*
   :align: center
   :name: ZMK Keypress Handling Flow CN

   ZMK Keypress Handling Flow

状态机
---------

BLE/2.4G/USB三种传输模式切换
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

模式拨片位置的确定
^^^^^^^^^^^^^^^^^^^^

模式切换的拨片能拨到三个档位，OFF档、BLE mode档和2.4G mode档。相关管脚在 :file:`rtk_keyboard.overlay` 中定义：

.. code-block:: c

   mode_monitor: mode_monitor {
        compatible = "zmk,mode-monitor-pin";
        ble-gpios = <&gpioa 9 (GPIO_ACTIVE_HIGH | RTL87X2G_GPIO_INPUT_DEBOUNCE_MS(8))>;
        ppt-gpios = <&gpioa 8 (GPIO_ACTIVE_HIGH | RTL87X2G_GPIO_INPUT_DEBOUNCE_MS(8))>;
        detect-usb-gpios = <&gpiob 28 (GPIO_ACTIVE_HIGH | RTL87X2G_GPIO_INPUT_DEBOUNCE_MS(20))>;
   };

根据 ble_gpios 和 ppt_gpios 的电平高低情况判断当前拨片位置：

   - ble_gpios 电平为低，ppt_gpios为高，拨片位置在BLE mode档。

   - ble_gpios 电平为高，ppt_gpios为低，拨片位置在2.4G mode档。

   - ble_gpios 电平为高，ppt_gpios为高，拨片位置在中间OFF档。

根据 detect_usb_gpios 的电平高低判断当前USB是否插入，detect_usb_gpios 为高电平时表示USB插入。其相关的判断和处理在 :file:`mode_monitor.c` 中。

插入USB即为USB模式
^^^^^^^^^^^^^^^^^^^^

:file:`endpoint.c` 中定义了传输模式的优先级，默认配置如下：

.. code-block:: c

   static enum zmk_transport preferred_transport =
      ZMK_TRANSPORT_USB; /* Used if multiple endpoints are ready */

具体分为以下三种情况：

   - 当USB没有被插入时，键盘模式由拨片的位置决定。

   - 当USB插入后，且USB枚举成功，键盘会优先使用USB发送数据。

   - 当USB插入后，但USB枚举失败，键盘仍然会处于当前模式不变化，仅进行充电。

2.4G状态机
~~~~~~~~~~~~~

.. figure:: ../figures/PPT_mode_state_switching_condition.*
   :align: center
   :scale: 70%
   :name: 2.4G模式状态转换

   2.4G模式状态转换

.. list-table:: 2.4G模式状态转换条件
   :widths: 5 60
   :header-rows: 1

   * - 序号
     - 说明

   * - 1
     - Power On after 2.4G driver init

   * - 2
     - When APP calls ppt_pair in idle status

   * - 3
     - When pairing time out, SYNC_EVENT_PAIR_TIMEOUT event received

   * - 4
     - When pairing successfully, SYNC_EVENT_PAIRED event received

   * - 5
     - When 2.4G link is lost in paired status, SYNC_EVENT_CONNECT_LOST event received

   * - 6
     - When APP calls ppt_reconnect in idle status

   * - 7
     - When connecting time out, SYNC_EVENT_PAIR_TIMEOUT event received

   * - 8
     - When connecting successfully in paired status, keyboard_PPT_STATUS_CONNECTED event received

   * - 9
     - When connecting successfully, keyboard_PPT_STATUS_CONNECTED event received

   * - 10
     - When 2.4G link lost in connected status, SYNC_EVENT_CONNECT_LOST event received

   * - 11
     - When low power voltage detected in idle status

   * - 12
     - When normal power voltage detected in low power status

定时器
---------

软件定时器
~~~~~~~~~~~~~

App使用k_timer相关的api管理软件定时器，详情可参考 :file:`zephyr/kernel/timer.c`，目前键盘用到的软件定时器如下表所示。

.. list-table:: 软件定时器说明
   :widths: 5 20 30
   :header-rows: 1

   * - 序号
     - 软件定时器
     - 说明

   * - 1
     - activity_timer
     - 用来监测系统active时间, 按键按下后为active

   * - 2
     - app_wdt_timer
     - 定时喂狗

   * - 3
     - battery_timer
     - 电量检测，1min检测一次

   * - 4
     - leds_gpio_ctrl_timer
     - 灯效控制

   * - 5
     - usb_mode_monitor_timer
     - usb插拔检测

.. _tri-mode keyboard硬件定时器:

硬件定时器
~~~~~~~~~~~~~

有两种硬件定时器可以供app使用，8个普通的HW Timer和4个Enhance Timer，具体的特性、区别和使用方式参考 :file:`zephyr/drivers/counter/counter_rtl87x2g_timer.c`，目前ZMK中暂未使用硬件定时器。

BLE模式
----------

BLE初始化
~~~~~~~~~~~

键盘处于BLE蓝牙模式，上电是需要对蓝牙相关内容进行初始化，包括如下：

1. :func:`zmk_ble_init` 函数是ble初始化入口，包含了ble设备初始化，bt信息存储，callback注册等：

.. code-block:: c

   bt_enable(NULL);
   #if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    err = settings_register(&profiles_handler);
    if (err) {
        LOG_ERR("Failed to setup the profile settings handler (err %d)", err);
        return err;
    }

    k_work_init_delayable(&ble_save_work, ble_save_profile_work);
    settings_load_subtree("ble");
    settings_load_subtree("bt");

   #endif
   bt_conn_cb_register(&conn_callbacks);
   bt_conn_auth_cb_register(&zmk_ble_auth_cb_display);
   bt_conn_auth_info_cb_register(&zmk_ble_auth_info_cb_display);
   zmk_ble_ready(0);

HID服务
~~~~~~~~~~

BLE模式下主要的服务为HID service。

BLE广播
~~~~~~~~~~

ZMK应用程序通过调用 :func:`update_advertising` 更新广播，广播内容如下。

.. code-block:: c

   static const struct bt_data zmk_ble_ad[] = {
      BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
      BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC1, 0x03),
      BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
      BT_DATA_BYTES(BT_DATA_UUID16_SOME, 0x12, 0x18, /* HID Service */
                     0x0f, 0x18                       /* Battery Service */
                     ),
   };

其中 DEVICE_NAME_LEN 和 DEVICE_NAME 默认如下:

.. code-block:: c

   #define C_DEVICE_NAME  CONFIG_BT_DEVICE_NAME
   #define C_DEVICE_NAME_LEN    sizeof(C_DEVICE_NAME) - 1 

.. note:: 
   设备名最长不能超过16字节；
   与苹果设备配对时，需要CONFIG_BT_PRIVACY=y

BLE配对和连接
~~~~~~~~~~~~~~~

BLE配对
^^^^^^^^^^

键盘触发配对有以下几种情况：

   1. 键盘没有配对信息时：

      1. 键盘上电会默认会发配对广播
      2. 支持多设备连接，通过按组合键 :kbd:`fn+1/2/3` 切换当前设备, 最多支持5个设备；通过CONFIG_BT_MAX_PAIRED，CONFIG_BT_MAX_CONN调整，默认都为3

   2. | 键盘已经配对过，有保存配对信息：
      | 按组合键 :kbd:`fn+4` ，键盘会生成新的static random address，进入配对模式，发配对广播。键盘和某设备配对上后，设备不解除配对的情况下，键盘可以重新与该设备配对。

BLE回连
^^^^^^^^^^

键盘通过发送回连广播包，用于键盘保存有配对信息时迅速和对端设备建立连接。

在以下三种情况下，键盘会发送回连广播包进行回连：

   1. 键盘上电，如果键盘成功配对过并且保存配对信息，在上电初始化完成后发送回连广播尝试回连。
   2. 键盘和对端配对连接上后，发生了非预期断线（不是任何一方主动断线），键盘会发送回连广播尝试回连。
   3. 键盘和对端配对连接上后，有一方主动断线，重新使用键盘时（按键），会发送回连广播尝试回连。

BLE连接参数
~~~~~~~~~~~~~

默认的连接参数在 :file:`zephyr/include/zephyr/bluetooth/conn.h` 中。

.. code-block:: c

   #define BT_GAP_INIT_CONN_INT_MIN                0x0018  /* 30 ms    */
   #define BT_GAP_INIT_CONN_INT_MAX                0x0028  /* 50 ms    */

   /** Default LE connection parameters:
   *    Connection Interval: 30-50 ms
   *    Latency: 0
   *    Timeout: 4 s
   */
   #define BT_LE_CONN_PARAM_DEFAULT BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, \
                     BT_GAP_INIT_CONN_INT_MAX, \
                     0, 400)

BLE数据发送
~~~~~~~~~~~~~

键盘数据通过接口 :mod:`zmk_hog_send_keyboard_report(struct zmk_hid_keyboard_report *keyboard_report)` 进行发送。其中结构体 :mod:`zmk_hid_keyboard_report` 定义如下：

.. code-block:: c

   struct zmk_hid_keyboard_report {
      uint8_t report_id;
      struct zmk_hid_keyboard_report_body body;
   } __packed;

其他相关功能
~~~~~~~~~~~~~~
其他BT相关的资讯请参考 `https://docs.zephyrproject.org/latest/connectivity/bluetooth/index.html <https://docs.zephyrproject.org/latest/connectivity/bluetooth/index.html/>`_

2.4G模式
-----------

2.4G初始化
~~~~~~~~~~~~~

键盘处于2.4G模式，上电是需要对2.4G相关内容进行初始化，包括如下：

:func:`zmk_ppt_init` 函数中：

   .. code-block:: c

      keyboard_ppt_init();
      keyboard_ppt_enable();
      keyboard_ppt_pair();

其中 :func:`keyboard_ppt_init` 是对2.4G相关的初始化；:func:`keyboard_ppt_enable` 是使能2.4G模块.

其中 :func:`keyboard_ppt_init` 中主要包括了：

   1. | 配置2.4­­­G角色。
      | 键盘：master，接收器：slave。

   2. 添加一些callback函数，包括接收到数据后调用的callbak函数 :mod:`ppt_app_receive_msg_cb`；发送完数据后调用的callback函数 :mod:`ppt_app_send_msg_cb`；2.4G事件处理的callback函数 :func:`ppt_app_sync_event_cb`，包括的事件有：

      1. SYNC_EVENT_PAIRED：2.4G配对成功，该事件产生后，会马上产生 SYNC_EVENT_CONNECTED 事件。
      2. SYNC_EVENT_PAIR_TIMEOUT：2.4G配对超时。
      3. SYNC_EVENT_CONNECTED：2.4G成功建立连接，回连成功会产生该事件，配对成功会先产生 SYNC_EVENT_PAIRED 再产生该事件。
      4. SYNC_EVENT_CONNECT_TIMEOUT：2.4G回连超时。
      5. SYNC_EVENT_CONNECT_LOST：链路异常断线。

   3. 获取绑定信息。

      .. code-block:: c

         ppt_app_global_data.is_ppt_bond = ppt_check_is_bonded()

   4. | 设置配对时的rssi限制。
      | :mod:`sync_pair_rssi_set(-65)`：表示rssi必须大于-65dbm才允许配对，2.4g master和slave两端都可以各自单独配置。

   5. 设置2.4G的连接参数。

      1. 设置数据包通信间隔和重传间隔：:func:`keyboard_ppt_set_sync_interval`。
      2. 设置心跳包间隔：:mod:`sync_master_set_hb_param(2, PPT_DEFAULT_HEARTBEAT_INTERVAL_TIME, 0)`。
      3. 设置CRC校验参数，默认校验长度为8 bit：:func:`sync_crc_set(8, 0x07, 0xff)`。

   6. 设置不同2.4G传输类型数据的缓存buffer深度：2.4g driver可以缓存一些发送的数据，不同的数据类型有各自的buffer。

      .. code-block:: c

         /**
          *  Different message types have different queue size, from left to right correspond to SYNC_MSG_TYPE_ONESHOT,
          *  SYNC_MSG_TYPE_FINITE_RETRANS, SYNC_MSG_TYPE_INFINITE_RETRANS, and SYNC_MSG_TYPE_DYNAMIC_RETRANS, respectively.
          */
         uint8_t msg_quota[SYNC_MSG_TYPE_NUM] = {0, 3, 3, 3};
         sync_msg_set_quota(msg_quota);

      :mod:`{0, 2, 2, 2}` 表示 **SYNC_MSG_TYPE_ONESHOT**, **SYNC_MSG_TYPE_FINITE_RETRANS**, **SYNC_MSG_TYPE_INFINITE_RETRANS**, **SYNC_MSG_TYPE_DYNAMIC_RETRANS** 四种数据类型的缓存buffer深度分别设置为0, 3, 3, 3。

   7. | 设置2.4G tx power。
      | 当宏 **FEATURE_SUPPORT_APP_CFG_PPT_TX_POWER** 置1时，可以通过 :mod:`sync_tx_power_set(false, PPT_TX_POWER_DBM_MAX, PPT_TX_POWER_DBM_MIN)` 设置2.4G tx power。否则tx power由config file中的配置决定。

2.4G配对和连接
~~~~~~~~~~~~~~~~

2.4G配对
^^^^^^^^^^^^

zmk程序中调用 :func:`keyboard_ppt_pair` 发起配对，持续1秒。如果配对成功，会依次产生 SYNC_EVENT_PAIRED 和 SYNC_EVENT_CONNECTED 两个事件通知app并进行相应的处理。如果1秒内没有配对成功，会产生 SYNC_EVENT_PAIR_TIMEOUT 事件，会重新尝试配对，尝试的次数可以通过宏定义 **PPT_PAIR_TIME_MAX_COUNT** 修改，默认为30次，即配对时长为30秒。

目前上电如果没有配对信息，默认会发起配对。

2.4G回连
^^^^^^^^^^^^

键盘程序中调用 :func:`keyboard_ppt_reconnect` 发起回连，持续1秒。如果回连成功，会产生 SYNC_EVENT_CONNECTED 事件。如果1秒内没有回连成功，会产生 SYNC_EVENT_CONNECT_TIMEOUT 事件，会重新尝试回连，尝试的次数可以通过宏定义 **PPT_RECONNECT_TIME_MAX_COUNT** 修改，默认为4次，也就是4秒。

以下几种情况会触发回连：

   1. 上电后，2.4G有配对信息，会尝试回连。
   2. 当2.4G链路异常断线产生了 SYNC_EVENT_CONNECT_LOST 事件，会尝试回连。

发包间隔和上报率
^^^^^^^^^^^^^^^^^^^

键盘（2.4G master）上电时在 :func:`keyboard_ppt_init` 通过调用 :func:`keyboard_ppt_set_sync_interval` 来设置2.4G正常数据通信时的发包间隔，发包间隔根据当前设置的上报率来配置，比如上报率为1KHz，发包间隔就设置为1000us。

接收器（2.4G slave）不需要设置发包间隔，接收器和键盘配对上后，会根据键盘的参数来调整。

如果在2.4G使能后要调整发包间隔，必须保证键盘在2.4G idle状态，即需要断线，且不进行配对或回连，然后重新设置发包间隔即可，接口为 :func:`keyboard_ppt_set_sync_interval`。

心跳包
^^^^^^^^^^

在2.4G建立连接以后，且没有数据交互时，2.4G会定期交互心跳包，以维持连接。以最后一笔数据交互为起始点，经过一段时间（10ms）没有发生新的数据交互（非空包），会开始通过心跳包维持连接。

键盘（2.4G master）上电时在 :func:`keyboard_ppt_init` 通过调用 :func:`sync_master_set_hb_param` 来设置2.4G心跳包的发包间隔，默认值为250ms。接收器（2.4G slave）不需要设置心跳包间隔。

.. code-block:: c

   /* set 2.4G connection heart beat interval */
   sync_master_set_hb_param(2, PPT_DEFAULT_HEARTBEAT_INTERVAL_TIME,0);

CRC校验
^^^^^^^^^^^

键盘（2.4G master）的校验长度默认使用8 bit，可以在 :func:`keyboard_ppt_init` 通过调用 :func:`sync_crc_set` 来配置。如果要在2.4g使用过程中重新配置校验长度，必须先调用 :func:`keyboard_ppt_stop_sync` 断开2.4g连接。校验长度推荐使用16 bit，在实际使用中会更安全，但是应用层最大可传输长度会减少1 Byte，并且功耗略有增加。

2.4G断线
~~~~~~~~~~~~

在2.4G连接建立后，发生以下三种情况会认为连接已经断开：

   1. 有数据持续交互时，持续一段时间（3*发包间隔）没有交互成功。
   2. 无数据交互时，通过心跳包维持连接，以心跳包时刻起一段时间（心跳包间隔+3*发包间隔）没有交互成功。
   3. board.h中宏定义 **FEATURE_SUPPORT_NO_ACTION_DISCONN** 设置为1时，打开无操作断线的功能，当键盘处于连接状态时，且一段时间没有被使用后，会主动断线。可以通过按键进行回连。无操作断线的时间可以通过 :file:`swtimer.h` 中的宏定义 **NO_ACTION_DISCON_TIMEOUT** 来修改，默认时间为1分钟。

除主动断线外，其他异常断线情况都会收到 SYNC_EVENT_CONNECT_LOST 事件，会尝试回连。

2.4G数据传输
~~~~~~~~~~~~~~~~

2.4G传输类型
^^^^^^^^^^^^^^^^^^

2.4G的传输类型有以下4种：

   1. SYNC_MSG_TYPE_ONESHOT：只发送一次，不重传。
   2. SYNC_MSG_TYPE_FINITE_RETRANS：有限次数的重传发送，通过 :func:`sync_msg_set_finite_retrans` 来配置重传次数。
   3. SYNC_MSG_TYPE_INFINITE_RETRANS：无限重传的发送。
   4. SYNC_MSG_TYPE_DYNAMIC_RETRANS：动态重传的发送，会一直重传，直到有新数据需要发送。

.. code-block:: c

   typedef enum
   {
      SYNC_MSG_TYPE_ONESHOT,
      SYNC_MSG_TYPE_FINITE_RETRANS,
      SYNC_MSG_TYPE_INFINITE_RETRANS,
      SYNC_MSG_TYPE_DYNAMIC_RETRANS,
      SYNC_MSG_TYPE_NUM,
      SYNC_MSG_TYPE_ALL = SYNC_MSG_TYPE_NUM
   } sync_msg_type_t;

应用层数据类型
^^^^^^^^^^^^^^^^

接收器在收到2.4G数据后，需要根据不同的应用数据类型（比如keyboard数据，consumer数据等等），通过USB往不同的通道（可能endpoint，report id等等不同）发送。为了区分不同的应用数据类型，键盘端将发送的数据的前一个或两个字节作为Header，以表征应用数据类型和数据内容。Header内容参考 :mod:`ppt_sync_app_header_t`。

应用层数据长度
^^^^^^^^^^^^^^^^

在一个2.4G 发包间隔中，键盘和接收器可以同时给对方发送数据，键盘发送给接收器称之为上行，接收器发送给键盘称之为下行，2.4G在一个发包间隔中上行和下行的应用层数据长度总和如下：

   - 发包间隔250us（上报率4KHz）：18 bytes

   - 发包间隔500us（上报率2KHz）：70 bytes

   - 发包间隔1ms（上报率1KHz）：127 bytes

2.4G数据长度最大不能超过127 bytes。当2.4G需要发送的数据，超过一个发包间隔内发包长度时，会占用后续的发包时间直到把当前数据全部发送完成，相当于根据数据长度，动态调整了发包间隔。

应答和重传
^^^^^^^^^^^^

2.4G交互是需要应答的，如果一方没有收到ACK，则认为这笔包没有发送成功，根据不同的2.4G数据传输类型和重传配置进行重传：

   1. SYNC_MSG_TYPE_ONESHOT：发送失败不重传。
   2. SYNC_MSG_TYPE_FINITE_RETRANS：有限次数的重传发送，重传次数通过 :mod:`sync_msg_set_finite_retrans` 来配置。
   3. SYNC_MSG_TYPE_INFINITE_RETRANS：无限重传。
   4. SYNC_MSG_TYPE_DYNAMIC_RETRANS：动态重传，当没有新的数据送到2.4G driver tx buffer中，tx buffer中的数据会无限重传，当有新数据送到2.4G driver tx buffer中时，tx buffer原本待发送的数据如果已经发送超过一次（重传0次）会停止重传，移出tx buffer。也就是数据重传的次数范围为0到无限次。

键盘（2.4G master）在 :func:`keyboard_ppt_set_sync_interval` 通过调用 :func:`sync_time_set` 来设置2.4G数据包重传间隔，调用接口时第一项参数选择 SYNC_TIME_PARAM_CONNECT_INTERVAL_HIGH，第二项参数即可设置具体的重传时间，默认是250us。当2.4G发包间隔超过250us时，每个发包间隔内有多次重传的机会（发包间隔/250us -1），比如发包间隔为1ms，当数据发送失败时，有3次重传机会，每250us重传一次。

键盘的数据对每一笔数据的响应要求比较高，推荐使用无限重传方式

Tx Power设置
~~~~~~~~~~~~~~~

当 :file:`keyboard_ppt_app.c` 中宏 **FEATURE_SUPPORT_APP_CFG_PPT_TX_POWER** 设置为1时（默认为0），可以单独设置2.4G模式的tx power，否则tx power由config file中的配置决定。

USB模式
----------

USB状态
~~~~~~~~~~

USB的所有状态如下：

.. code-block:: c

   enum usb_dc_status_code {
	/** USB error reported by the controller */
	USB_DC_ERROR,
	/** USB reset */
	USB_DC_RESET,
	/** USB connection established, hardware enumeration is completed */
	USB_DC_CONNECTED,
	/** USB configuration done */
	USB_DC_CONFIGURED,
	/** USB connection lost */
	USB_DC_DISCONNECTED,
	/** USB connection suspended by the HOST */
	USB_DC_SUSPEND,
	/** USB connection resumed by the HOST */
	USB_DC_RESUME,
	/** USB interface selected */
	USB_DC_INTERFACE,
	/** Set Feature ENDPOINT_HALT received */
	USB_DC_SET_HALT,
	/** Clear Feature ENDPOINT_HALT received */
	USB_DC_CLEAR_HALT,
	/** Start of Frame received */
	USB_DC_SOF,
	/** Initial USB connection status */
	USB_DC_UNKNOWN
   };

初始化
~~~~~~~~~

调用 :func:`zmk_usb_init` 该函数初始化 USB 核心子系统并启用相应的硬件，使其可以开始在 USB 总线上发送和接收数据，并产生中断。

   1. 使能usb bus：:mod:`usb_vbus_set(true)`。
   2. 注册并设置回调函数：:mod:`usb_register_status_callback(forward_status_cb)` 和 :mod:`usb_dc_set_status_callback(forward_status_cb)`，注册USB前级状态改变的回调函数。
   3. 连接usb设备：:mod:`usb_dc_attach` ，用于连接 USB 设备的功能。成功后，USB PLL 将被启用，USB 设备现在可以在 USB 总线上收发数据并产生中断。
   4. 初始化USB传输：:mod:`usb_transfer_init`。
   5. 初始化USB endpoint：:func:`usb_dc_ep_configure`。
   6. 配置USB endpoint callback：:func:`usb_dc_ep_set_callback(USB_CONTROL_EP_XX, usb_handle_control_transfer)`，用于设置回调函数，以通知应用程序已收到并可用的数据，或已在所选端点上完成传输；如果应用程序代码不需要回调，则为 NULL。回调状态代码由 :mode:`usb_dc_ep_cb_status_code` 描述。
   7. 使能USB endpoint：:func:`usb_dc_ep_enable`。

USB描述符初始化
^^^^^^^^^^^^^^^^^^

在 :file:`usb_descriptor.c` 和 :file:`core.c` 中，包含了USB描述符的配置，其中大部分常用配置都以CONFIG的形式开出，供用户配置。

.. code-block:: c

   #define CONFIG_USB_DEVICE_VID 0x0BDA
   #define CONFIG_USB_DEVICE_PID 0x4762

   struct common_descriptor common_desc = {
      /* Device descriptor */
      .device_descriptor = {
         .bLength = sizeof(struct usb_device_descriptor),
         .bDescriptorType = USB_DESC_DEVICE,
   #ifdef CONFIG_USB_DEVICE_BOS
         .bcdUSB = sys_cpu_to_le16(USB_SRN_2_1),
   #else
         .bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),
   #endif
   #ifdef CONFIG_USB_COMPOSITE_DEVICE
         .bDeviceClass = USB_BCC_MISCELLANEOUS,
         .bDeviceSubClass = 0x02,
         .bDeviceProtocol = 0x01,
   #else
         .bDeviceClass = 0,
         .bDeviceSubClass = 0,
         .bDeviceProtocol = 0,
   #endif
         .bMaxPacketSize0 = USB_MAX_CTRL_MPS,
         .idVendor = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_VID),
         .idProduct = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_PID),
         .bcdDevice = sys_cpu_to_le16(USB_BCD_DRN),
         .iManufacturer = USB_DESC_MANUFACTURER_IDX,
         .iProduct = USB_DESC_PRODUCT_IDX,
         .iSerialNumber = USB_DESC_SERIAL_NUMBER_IDX,
         .bNumConfigurations = 1,
      },
      /* Configuration descriptor */
      .cfg_descr = {
         .bLength = sizeof(struct usb_cfg_descriptor),
         .bDescriptorType = USB_DESC_CONFIGURATION,
         /*wTotalLength will be fixed in usb_fix_descriptor() */
         .wTotalLength = 0,
         .bNumInterfaces = 0,
         .bConfigurationValue = 1,
         .iConfiguration = 0,
         .bmAttributes = USB_SCD_RESERVED |
               COND_CODE_1(CONFIG_USB_SELF_POWERED,
                     (USB_SCD_SELF_POWERED), (0)) |
               COND_CODE_1(CONFIG_USB_DEVICE_REMOTE_WAKEUP,
                     (USB_SCD_REMOTE_WAKEUP), (0)),
         .bMaxPower = CONFIG_USB_MAX_POWER,
      },
   };
   struct usb_string_desription string_descr = {
      .lang_descr = {
         .bLength = sizeof(struct usb_string_descriptor),
         .bDescriptorType = USB_DESC_STRING,
         .bString = sys_cpu_to_le16(0x0409),
      },
      /* Manufacturer String Descriptor */
      .utf16le_mfr = {
         .bLength = USB_STRING_DESCRIPTOR_LENGTH(
               CONFIG_USB_DEVICE_MANUFACTURER),
         .bDescriptorType = USB_DESC_STRING,
         .bString = CONFIG_USB_DEVICE_MANUFACTURER,
      },
      /* Product String Descriptor */
      .utf16le_product = {
         .bLength = USB_STRING_DESCRIPTOR_LENGTH(
               CONFIG_USB_DEVICE_PRODUCT),
         .bDescriptorType = USB_DESC_STRING,
         .bString = CONFIG_USB_DEVICE_PRODUCT,
      },
      /* Serial Number String Descriptor */
      .utf16le_sn = {
         .bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USB_DEVICE_SN),
         .bDescriptorType = USB_DESC_STRING,
         .bString = CONFIG_USB_DEVICE_SN,
      },
   };

   设备描述符中的bcdUSB，CONFIG_USB_COMPOSITE_DEVICE，idVendor，idProduct可配置；配置描述符中的bmAttributes，bMaxPower可配置。

USB接口初始化
^^^^^^^^^^^^^^^

通过 :file:`core.c` 进行usb interface相关的初始化，包括USB接口描述符，USB端点描述符和HID描述符的初始化，以及set/get report和set/get protocol的回调函数的注册。

* USB接口描述符为：

   .. code-block:: c
      #if defined(CONFIG_USB_HID_BOOT_PROTOCOL)
      #define INITIALIZER_IF							\
         {								\
            .bLength = sizeof(struct usb_if_descriptor),		\
            .bDescriptorType = USB_DESC_INTERFACE,			\
            .bInterfaceNumber = 0,					\
            .bAlternateSetting = 0,					\
            .bNumEndpoints = 1,					\
            .bInterfaceClass = USB_BCC_HID,				\
            .bInterfaceSubClass = 1,				\
            .bInterfaceProtocol = 0,				\
            .iInterface = 0,					\
         }
      #else
      #define INITIALIZER_IF							\
      {								\
         .bLength = sizeof(struct usb_if_descriptor),		\
         .bDescriptorType = USB_DESC_INTERFACE,			\
         .bInterfaceNumber = 0,					\
         .bAlternateSetting = 0,					\
         .bNumEndpoints = 1,					\
         .bInterfaceClass = USB_BCC_HID,				\
         .bInterfaceSubClass = 0,				\
         .bInterfaceProtocol = 0,				\
         .iInterface = 0,					\
      }
      #endif

   接口描述符可以通过CONFIG_USB_HID_BOOT_PROTOCOL配置，此外bInterfaceProtocol可以通过usb_hid_set_proto_code这个api进行配置。

* USB端点描述符如下：

   .. code-block:: c

      #define INITIALIZER_IF_EP(addr, attr, mps)				\
      {								\
         .bLength = sizeof(struct usb_ep_descriptor),		\
         .bDescriptorType = USB_DESC_ENDPOINT,			\
         .bEndpointAddress = addr,				\
         .bmAttributes = attr,					\
         .wMaxPacketSize = sys_cpu_to_le16(mps),			\
         .bInterval = CONFIG_USB_HID_POLL_INTERVAL_MS,		\
      }

端点描述符中的bInterval，wMaxPacketSize，CONFIG_ENABLE_HID_INT_OUT_EP，CONFIG_USB_HID_POLL_INTERVAL_MS，CONFIG_HID_INTERRUPT_EP_MPS可配置。

USB Start/Stop
~~~~~~~~~~~~~~~~~

如果处于USB模式，上电时在 :file:`mode_monitor.c` 中开启software timer检查USB插入的电平抖动情况，检测成功执行 :func:`zmk_usb_init`。

   .. code-block:: c

      int zmk_usb_init(void) {
         int usb_enable_ret;
         
         usb_enable_ret = usb_enable(usb_status_cb);
         if (usb_enable_ret != 0) {
            LOG_ERR("Unable to enable USB ,err = %d", usb_enable_ret);
            app_mode.is_in_usb_mode = false;
            return -EINVAL;
         }
         return 0;
      }

当上电初始化完成后，程序运行过程中，可以通过 :func:`zmk_usb_deinit` 完全关闭USB模块。

   .. code-block:: c

      int zmk_usb_deinit(void) {
         int usb_disable_ret;
         usb_disable_ret = usb_disable();

         if (usb_disable_ret != 0) {
            LOG_ERR("Unable to disable USB");
            return -EINVAL;
         }
         return 0;
      }

USB HID Class
~~~~~~~~~~~~~~~~

HID描述符和报告描述符
^^^^^^^^^^^^^^^^^^^^^^^

* ­HID描述符如下。

   .. code-block:: c

      #define INITIALIZER_IF_HID						\
      {								\
         .bLength = sizeof(struct usb_hid_descriptor),		\
         .bDescriptorType = USB_DESC_HID,			\
         .bcdHID = sys_cpu_to_le16(USB_HID_VERSION),		\
         .bCountryCode = 0,					\
         .bNumDescriptors = 1,					\
         .subdesc[0] = {						\
            .bDescriptorType = USB_DESC_HID_REPORT,	\
            .wDescriptorLength = 0,				\
         },							\
      }

* 报告描述符可参考 :file:`hid.h` 里的 :mod:`zmk_hid_report_desc[]` 进行配置。

Interrupt Report
^^^^^^^^^^^^^^^^^^^

以keyboard interface为例进行说明。

通过 :func:`zmk_usb_hid_send_keyboard_report` 进行Interrupt report。:func:`app_usb_send_keyboard_data` 调用了 :func:`zmk_usb_hid_send_report` ，:func:`zmk_usb_hid_send_report` 调用了 :func:`hid_int_ep_write`。

.. code-block:: c

   /**
    * @brief  Write to USB HID interrupt endpoint buffer
    * @param  
    * dev – Pointer to USB HID device
    * data – Pointer to data buffer
    * data_len – Length of data to copy
    * bytes_ret – Bytes written to the EP buffer.
    * @return 0 on success, negative errno code on fail.
    */
   int hid_int_ep_write(const struct device *dev, const uint8_t *data, uint32_t data_len,
            uint32_t *bytes_ret)
   {
      const struct usb_cfg_data *cfg = dev->config;
      struct hid_device_info *hid_dev_data = dev->data;

      if (hid_dev_data->configured && !hid_dev_data->suspended) {
         return usb_write(cfg->endpoint[HID_INT_IN_EP_IDX].ep_addr, data,
            data_len, bytes_ret);
      } else {
         LOG_WRN("Device is not configured");
         return -EAGAIN;
      }
   }

USB HID CLASS REQUEST
^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   /** USB HID Class GetReport bRequest value */
   #define USB_HID_GET_REPORT		0x01
   /** USB HID Class GetIdle bRequest value */
   #define USB_HID_GET_IDLE		0x02
   /** USB HID Class GetProtocol bRequest value */
   #define USB_HID_GET_PROTOCOL	0x03
   /** USB HID Class SetReport bRequest value */
   #define USB_HID_SET_REPORT		0x09
   /** USB HID Class SetIdle bRequest value */
   #define USB_HID_SET_IDLE		0x0A
   /** USB HID Class SetProtocol bRequest value */
   #define USB_HID_SET_PROTOCOL	0x0B

以上的request对应的handlers在，:file:`zephyr/subsys/usb/device/class/hid/core.c` 中都有相应的实现。

Set/Get Report
^^^^^^^^^^^^^^^^^

app 中如果需要添加callback, 以get/set report为例：

.. code-block:: c

   static const struct hid_ops ops = {
    .get_report = get_report_cb,
    .set_report = set_report_cb,
};

在 :func:`zmk_usb_hid_init` 初始化函数中，调用 :func:`usb_hid_register_device` 注册set/get report的回调函数：

.. code-block:: c

   static int zmk_usb_hid_init(void) {
      hid_dev = device_get_binding("HID_0");
      if (hid_dev == NULL) {
         LOG_ERR("Unable to locate HID device");
         return -EINVAL;
      }

      usb_hid_register_device(hid_dev, zmk_hid_report_desc, sizeof(zmk_hid_report_desc), &ops);

      usb_hid_init(hid_dev);

      return 0;
   }

:func:`get_report_cb` 具体定义如下。需要给数据指针和数据长度指针进行赋值。返回值为0时表示success。

.. code-block:: c

   static int get_report_cb(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                           uint8_t **data) {

      /*
      * 7.2.1 of the HID v1.11 spec is unclear about handling requests for reports that do not exist
      * For requested reports that aren't input reports, return -ENOTSUP like the Zephyr subsys does
      */
      if ((setup->wValue & HID_GET_REPORT_TYPE_MASK) != HID_REPORT_TYPE_INPUT) {
         LOG_ERR("Unsupported report type %d requested", (setup->wValue & HID_GET_REPORT_TYPE_MASK)
                                                               << 8);
         return -ENOTSUP;
      }

      switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
      case ZMK_HID_REPORT_ID_KEYBOARD: {
         *data = get_keyboard_report(len);
         break;
      }
      case ZMK_HID_REPORT_ID_CONSUMER: {
         struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();
         *data = (uint8_t *)report;
         *len = sizeof(*report);
         break;
      }
      default:
         LOG_ERR("Invalid report ID %d requested", setup->wValue & HID_GET_REPORT_ID_MASK);
         return -EINVAL;
      }

      return 0;
   }

:func:`set_report_cb` 具体定义如下。返回值为0时表示success。

.. code-block:: c

   static int set_report_cb(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                           uint8_t **data) {
      if ((setup->wValue & HID_GET_REPORT_TYPE_MASK) != HID_REPORT_TYPE_OUTPUT) {
         LOG_ERR("Unsupported report type %d requested",
                  (setup->wValue & HID_GET_REPORT_TYPE_MASK) >> 8);
         return -ENOTSUP;
      }

      switch (setup->wValue & HID_GET_REPORT_ID_MASK) {
   #if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
      case ZMK_HID_REPORT_ID_LEDS:
         if (*len != sizeof(struct zmk_hid_led_report)) {
               LOG_ERR("LED set report is malformed: length=%d", *len);
               return -EINVAL;
         } else {
               struct zmk_hid_led_report *report = (struct zmk_hid_led_report *)*data;
               struct zmk_endpoint_instance endpoint = {
                  .transport = ZMK_TRANSPORT_USB,
               };
               zmk_hid_indicators_process_report(&report->body, endpoint);
         }
         break;
   #endif // IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
      default:
         LOG_ERR("Invalid report ID %d requested", setup->wValue & HID_GET_REPORT_ID_MASK);
         return -EINVAL;
      }

      return 0;
   }

设备枚举
~~~~~~~~~~

USB设备进行枚举时，会进行 set/get descriptor 和 set/get config。当进行了set config后，USB状态会进行切换，在 :func:`usb_status_cb` 获知状态切换到了USB_CONFIGURED，认为USB设备枚举完成。USB模式下，状态切到USB_DC_CONFIGURED后，键盘数据才允许正常发送。

.. code-block:: c

   void usb_status_cb(enum usb_dc_status_code status, const uint8_t *params) {
    // Start-of-frame events are too frequent and noisy to notify, and they're
    // not used within ZMK
    LOG_DBG("usb status cb: usb status is %d", status);
    if (status == USB_DC_SOF) {
        return;
    }

   #if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
      if (status == USB_DC_RESET) {
         zmk_usb_hid_set_protocol(HID_PROTOCOL_REPORT);
      }
   #endif
      usb_status = status;
      if (status == USB_DC_CONFIGURED) {
         app_global_data.is_usb_enumeration_success = true;
      }
      k_work_submit(&usb_status_notifier_work);
   };

休眠和唤醒
~~~~~~~~~~~~~

USB suspend时候状态会切到USB_DC_SUSPEND，此时允许进入DLPS。

USB进入suspend以后，有两种方式唤醒：USB host wakeup和键盘主动wakeup。使用键盘尝试发送USB数据时，如果当前处于suspend状态，会主动进行wakeup。以键盘数据发送为例：

.. code-block:: c

   static int zmk_usb_hid_send_report(const uint8_t *report, size_t len) {
      switch (zmk_usb_get_status()) {
      case USB_DC_SUSPEND:
         return usb_wakeup_request();
      case USB_DC_ERROR:
      case USB_DC_RESET:
      case USB_DC_DISCONNECTED:
      case USB_DC_UNKNOWN:
         return -ENODEV;
      default:
         k_sem_take(&hid_sem, K_MSEC(30));
         int err = hid_int_ep_write(hid_dev, report, len, NULL);

         if (err) {
               k_sem_give(&hid_sem);
         }
         return err;
      }
   }

当USB从suspend状态被唤醒后，会重新进行枚举，进入USB_SUSPENDED状态，并发送一笔数据以触发对端设备亮屏。

按照USB spec规定，需要USB host使能键盘remote wake up功能后，键盘才有主动唤醒的能力。当将宏 **CONFIG_USB_DEVICE_REMOTE_WAKEUP** 置1后，键盘使能remote wake up功能。

按键
-------

可以通过GPIO或硬件Keyscan两种方案实现按键功能。两种方案在功能和性能上，主要差别如下：

   - 按键按下时，GPIO方案通过触发GPIO中断来检测按键按下和释放，按键按下时也可以进入DLPS；Keyscan方案的功耗会稍微高一些：Keyscan Autoscan方案在按键按下后需要持续扫描，无法进入DLPS（进入DLPS后Keyscan硬件模块会掉电，无法进行扫描）； Keyscan manualscan由于是通过timer来定时扫描，如果timer定时周期大于20ms, 期间允许进入dlps。
   - 在IO管脚数量有限时，Keyscan可以提供更多的输入，实现更多的按键。

GPIO按键
~~~~~~~~~~~

GPIO方案需要将 :file:`zmk/app/boards/shields/xxx.overlay` 中定义 kscan0 节点的 :mod:`compatible = "zmk,kscan-gpio-matrix"`。在 :file:`kscan_gpio_matrix.c` 中，会遍历设备树中是否有zmk_kscan_gpio_matrix的描述，如有，就进行gpio_kscan相关的初始化。

.. code-block:: c

      kscan0: kscan0 {
         compatible = "zmk,kscan-gpio-matrix";
         debounce-press-ms = <10>;
         debounce-release-ms = <10>;
         debounce-scan-period-ms = <10>;
         wakeup-source;
         diode-direction = "col2row";

         col-gpios
             = < &gpioa   4    GPIO_ACTIVE_HIGH>,
               < &gpiob   8    GPIO_ACTIVE_HIGH>,
               < &gpiob   7    GPIO_ACTIVE_HIGH>;

         row-gpios
             = < &gpioa    0    GPIO_ACTIVE_HIGH>,
               < &gpioa    15   GPIO_ACTIVE_HIGH>,
               < &gpioa    13   GPIO_ACTIVE_HIGH>;
      };
   };

.. figure:: ../figures/GPIO_key_detection_and_processing_flow.*
   :align: center
   :scale: 80%
   :name: GPIO按键检测和处理流程

   GPIO按键检测和处理流程

初始化
^^^^^^^^^

kscan默认行作为输出，列作为输入，如需调换可通过 :mod:`diode-direction` 调整。

.. code-block:: c

   static int kscan_matrix_init(const struct device *dev) {
      struct kscan_matrix_data *data = dev->data;

      data->dev = dev;

      // Sort inputs by port so we can read each port just once per scan.
      kscan_gpio_list_sort_by_port(&data->inputs);

      kscan_matrix_init_inputs(dev);
      kscan_matrix_init_outputs(dev);
      kscan_matrix_set_all_outputs(dev, 0);

      k_work_init_delayable(&data->work, kscan_matrix_work_handler);

      return 0;
   }

按键去抖
^^^^^^^^^^

按键防抖可分为硬件防抖和软件防抖，其中硬件防抖可通过在设备树中添加 :mod:`RTL87X2G_GPIO_INPUT_DEBOUNCE_MS` 的描述；软件防抖则是通过 :mod:`debounce-press-ms`， :mod:`debounce-release-ms`定义。

在初始化中配置GPIO按键debounce实现去抖，但并不是每个管脚都有单独的GPIO debounce，而是一组几个管脚共用一个GPIO debounce。每组管脚只有同时有一个使能debounce，因此需要在硬件设计阶段，合理规划管脚，避免同一组两个管脚都需要GPIO debounce的情况。GPIO debounce分组详情参考HDK中的文档《RTL87x2G_IOPin_Information.xlsx》。

按键状态识别和处理
^^^^^^^^^^^^^^^^^^^^

初始化完成后，当有按键按下后，会触发GPIO中断，中断处理函数如下：

.. code-block:: c

   static void kscan_matrix_irq_callback_handler(const struct device *port, struct gpio_callback *cb,
                                                const gpio_port_pins_t pin) {
      struct kscan_matrix_irq_callback *irq_data =
         CONTAINER_OF(cb, struct kscan_matrix_irq_callback, callback);
      struct kscan_matrix_data *data = irq_data->dev->data;

      // Disable our interrupts temporarily to avoid re-entry while we scan.
      kscan_matrix_interrupt_disable(data->dev);

      data->scan_time = k_uptime_get();

      k_work_reschedule(&data->work, K_NO_WAIT);
   }

通过work queue调度，执行对应的 :mod:`work queue handler`， 随后执行 :mod:`kscan_matrix_read` 开始扫描，扫描到的按键经过设置的debounce后，会通过callback通知app, 进行下一步的按键处理。

如果有按键一直按下，会调用 :mod:`kscan_matrix_read_continue` 持续扫描；当所有按键都松开时，调用 :mod:`kscan_matrix_read_end` 停止扫描。

.. code-block:: c

   static void kscan_matrix_work_handler(struct k_work *work) {
      struct k_work_delayable *dwork = k_work_delayable_from_work(work);
      struct kscan_matrix_data *data = CONTAINER_OF(dwork, struct kscan_matrix_data, work);
      kscan_matrix_read(data->dev);
   }

   static int kscan_matrix_read(const struct device *dev) {
      struct kscan_matrix_data *data = dev->data;
      const struct kscan_matrix_config *config = dev->config;
      LOG_DBG("");
      // Scan the matrix.
      for (int i = 0; i < config->outputs.len; i++) {
         const struct kscan_gpio *out_gpio = &config->outputs.gpios[i];

         int err = gpio_pin_set_dt(&out_gpio->spec, 1);
         if (err) {
               LOG_ERR("Failed to set output %i active: %i", out_gpio->index, err);
               return err;
         }

   #if CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS > 0
         k_busy_wait(CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS);
   #endif
         struct kscan_gpio_port_state state = {0};

         for (int j = 0; j < data->inputs.len; j++) {
               const struct kscan_gpio *in_gpio = &data->inputs.gpios[j];

               const int index = state_index_io(config, in_gpio->index, out_gpio->index);
               const int active = kscan_gpio_pin_get(in_gpio, &state);
               if (active < 0) {
                  LOG_ERR("Failed to read port %s: %i", in_gpio->spec.port->name, active);
                  return active;
               }

               zmk_debounce_update(&data->matrix_state[index], active, config->debounce_scan_period_ms,
                                 &config->debounce_config);
         }

         err = gpio_pin_set_dt(&out_gpio->spec, 0);
         if (err) {
               LOG_ERR("Failed to set output %i inactive: %i", out_gpio->index, err);
               return err;
         }

   #if CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS > 0
         k_busy_wait(CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS);
   #endif
      }

      // Process the new state.
      bool continue_scan = false;

      for (int r = 0; r < config->rows; r++) {
         for (int c = 0; c < config->cols; c++) {
               const int index = state_index_rc(config, r, c);
               struct zmk_debounce_state *state = &data->matrix_state[index];

               if (zmk_debounce_get_changed(state)) {
                  const bool pressed = zmk_debounce_is_pressed(state);

                  LOG_DBG("Sending event at %i,%i state %s", r, c, pressed ? "on" : "off");
                  data->callback(dev, r, c, pressed);
               }

               continue_scan = continue_scan || zmk_debounce_is_active(state);
         }
      }

      if (continue_scan) {
         // At least one key is pressed or the debouncer has not yet decided if
         // it is pressed. Poll quickly until everything is released.
         kscan_matrix_read_continue(dev);
      } else {
         // All keys are released. Return to normal.
         kscan_matrix_read_end(dev);
      }

      return 0;
   }

Keyscan按键
~~~~~~~~~~~~~~

Keyscan方案需要将 :file:`zmp/app/boards/shields/xxx.overlay` 中定义如下。Keyscan方案使用硬件Keyscan模块扫描得到按键的按下和释放状态。

.. code-block:: c
   &kscan {
      pinctrl-0 = <&kscan_default>;
      pinctrl-1 = <&kscan_sleep>;
      pinctrl-names = "default", "sleep";
      row-size = <8>;
      col-size = <18>;
      debounce-time-us = <10000>;
      scan-time-us = <10000>;
      release-time-us = <10000>;
      scan-debounce-cnt = <1>;
      status = "okay";
   };

管脚配置
^^^^^^^^^^^

Keyscan按键方案使用管脚在 :mod:`pinctrl` 中配置, 其中kscan_default是在cpu active下的配置，kscan_sleep是cpu sleep时的配置：

.. code-block:: c

   &pinctrl {
      adc_default: adc_default {
         group1 {
               psels = <RTL87X2G_PSEL(SW_MODE, P2_2, DIR_IN, DRV_LOW, PULL_NONE)>;
         };
      };
      kscan_default: kscan_default {
         group1 {
               psels =
               <RTL87X2G_PSEL(KEY_COL_0,  P0_4, DIR_OUT, DRV_LOW, PULL_NONE)>,
               <RTL87X2G_PSEL(KEY_COL_1,  P4_3, DIR_OUT, DRV_LOW, PULL_NONE)>,
               <RTL87X2G_PSEL(KEY_COL_2,  P4_2, DIR_OUT, DRV_LOW, PULL_NONE)>;
         };
         group2 {
               psels =
               <RTL87X2G_PSEL(KEY_ROW_0, P0_0, DIR_IN, DRV_LOW, PULL_UP)>,
               <RTL87X2G_PSEL(KEY_ROW_1, P1_7, DIR_IN, DRV_LOW, PULL_UP)>,
               <RTL87X2G_PSEL(KEY_ROW_2, P1_5, DIR_IN, DRV_LOW, PULL_UP)>;
               bias-pull-strong;
         };
         };
      kscan_sleep: kscan_sleep {
         group1 {
               psels =
               <RTL87X2G_PSEL(SW_MODE, P0_4, DIR_OUT, DRV_LOW, PULL_DOWN)>,
               <RTL87X2G_PSEL(SW_MODE, P4_3, DIR_OUT, DRV_LOW, PULL_DOWN)>,
               <RTL87X2G_PSEL(SW_MODE, P4_2, DIR_OUT, DRV_LOW, PULL_DOWN)>;
      
         };
         group2 {
               psels =
               <RTL87X2G_PSEL(SW_MODE, P0_0, DIR_IN, DRV_HIGH, PULL_UP)>,
               <RTL87X2G_PSEL(SW_MODE, P1_7, DIR_IN, DRV_HIGH, PULL_UP)>,
               <RTL87X2G_PSEL(SW_MODE, P1_5, DIR_IN, DRV_HIGH, PULL_UP)>;
               bias-pull-strong;
               wakeup-low;
         };
         };
   };

初始化
^^^^^^^^^

在 :file:`kscan_rtl87x2g.c` 中，对kscan的clk, pinctrl, 结构体参数以及中断进行初始化。

.. code-block:: c

   static int kscan_rtl87x2g_init(const struct device *dev)
   {
      LOG_DBG("dev %s init\n", dev->name);
      const struct kscan_rtl87x2g_config *config = dev->config;
      struct kscan_rtl87x2g_data *data = dev->data;
      KEYSCAN_TypeDef *keyscan = (KEYSCAN_TypeDef *)config->reg;
      int err;

      memset(data->key_map, 0, sizeof(data->key_map));
      memset(data->keys, 0, sizeof(data->keys));

      (void)clock_control_on(RTL87X2G_CLOCK_CONTROLLER,
                              (clock_control_subsys_t)&config->clkid);

      err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
      if (err < 0)
      {
         return err;
      }

      KEYSCAN_InitTypeDef kscan_init_struct;
      KeyScan_StructInit(&kscan_init_struct);

      kscan_init_struct.rowSize = config->row_size;
      kscan_init_struct.colSize = config->col_size;

      /* default scan clk is 2.5 MHz */
      kscan_init_struct.clockdiv = 1;

      /* default delay clk is 50 kHz */
      kscan_init_struct.delayclk = 49;

      kscan_init_struct.debouncecnt = (config->deb_us + 10) / 20;
      kscan_init_struct.scanInterval = (config->scan_us + 10) / 20;
      kscan_init_struct.releasecnt = (config->rel_us + 10) / 20;

      kscan_init_struct.debounceEn = kscan_init_struct.debouncecnt ? ENABLE : DISABLE;
      kscan_init_struct.scantimerEn = kscan_init_struct.scanInterval ? ENABLE : DISABLE;
      kscan_init_struct.detecttimerEn = kscan_init_struct.releasecnt ? ENABLE : DISABLE;

      kscan_init_struct.scanmode = KeyScan_Auto_Scan_Mode;
      kscan_init_struct.keylimit = 26;

      KeyScan_Init(keyscan, &kscan_init_struct);
      
      /* set pre guard time */
      KEYSCAN_CLK_DIV_TypeDef keyscan_0x00 = {.d32 = keyscan->KEYSCAN_CLK_DIV};
      keyscan_0x00.b.keyscan_gt_pre_sel = 6;
      keyscan->KEYSCAN_CLK_DIV = keyscan_0x00.d32;

      KeyScan_INTConfig(keyscan, KEYSCAN_INT_SCAN_END, ENABLE);
      KeyScan_INTConfig(keyscan, KEYSCAN_INT_ALL_RELEASE, ENABLE);

      KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_SCAN_END);
      KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_ALL_RELEASE);

      KeyScan_INTMask(keyscan, KEYSCAN_INT_SCAN_END, DISABLE);
      KeyScan_INTMask(keyscan, KEYSCAN_INT_ALL_RELEASE, DISABLE);
      KeyScan_Cmd(keyscan, ENABLE);
      config->irq_config_func();

   #ifdef CONFIG_PM_DEVICE
      kscan_register_dlps_cb();
   #endif
      return 0;
   }


   其中ScanMode默认是KeyScan_Auto_Scan_Mode，当有任意按键被按下时间超过 debouncecnt 后会触发自动扫描，两次扫描的间隔为ScanInterval。Keyscan使用了两种中断：KEYSCAN_INT_SCAN_END中断表示单次扫描结束，以获取自动扫描的结果；KEYSCAN_INT_ALL_RELEASE中断表示全部按键都被释放。

按键去抖机制
^^^^^^^^^^^^^^

Keyscan方案debounce分为三种场景：

   1. 第一个按键按下的debounce：使用的是Keyscan模块的硬件debounce，通过 :mod:`debounceEn` 和 :mod:`debouncecnt` 设置。
   2. 已有按键被按下后，后续按键按下和抬起的debounce：当连续n+1次自动扫描（对应n*Scan Interval时间）都扫描到按键状态改变后，才认为检测到新的按键状态，以此实现后续按键的按下和抬起的去抖。
   3. 全部按键都释放的debounce：通过配置参数 :mod:`releasecnt` 实现。

Keyscan的所有参数均可以通过设备树进行统一配置。

按键状态检查和处理
^^^^^^^^^^^^^^^^^^^^

Keyscan的检测流程分为 KEYSCAN_INT_SCAN_END 和 KEYSCAN_INT_ALL_RELEASE 中断相关处理，其中 KEYSCAN_INT_ALL_RELEASE 用于检测是否全部按键都被释放了，KEYSCAN_INT_SCAN_END用于检测按键的其他状态的改变。

基于Keyscan模块的硬件去抖功能，当首个按键按下超过debounce时间后才开始扫描，因此当第一次扫描结束触发了 KEYSCAN_INT_FLAG_SCAN_END 中断时，可将扫到的键值判定为有效按键；如果不是第一次扫描，则只有当按键状态已经维持了debounce时间后才认为按键状态改变。具体流程为：先判断此次扫描所得键值是否异常超出FIFO大小，超出则重新初始化等待下一次扫描；再判断当前键值是否与记录的上一次扫描值相同，只有当前后值不同并且all release flag为true时才会认为是首个按键，之后每次按键都会再连续扫描n次，每次的值相同之后才会认为是有效按键。

.. figure:: ../figures/The_processing_for_scan_end_interrupt.*
   :align: center
   :scale: 80%
   :name: Scan End中断相关处理

   Scan End中断相关处理

全部按键都被释放后会触发 KEYSCAN_INT_ALL_RELEASE 中断，将会进行按键释放处理并重新初始化keyscan模块。

.. figure:: ../figures/The_processing_for_all_release_interrupt.*
   :align: center
   :scale: 80%
   :name: All Release中断相关处理

   All Release中断相关处理

在被判断为有效按键之后，将通过 :func:`callback` 通知应用层进行处理。

电量检测和充电
----------------

USB插拔检测
~~~~~~~~~~~~~

USB_MODE_MONITOR管脚默认是低电平，当USB插入后USB_MODE_MONITOR管脚会被拉高。通过检测USB_MODE_MONITOR管脚的电平来判断USB当前是插入还是拔出状态。USB_MODE_MONITOR管脚配置为GPIO，使能GPIO中断，当USB_MODE_MONITOR管脚的GPIO中断触发后，会打开软件定时器，反复检测若干次电平，来实现去抖动，以防止误判USB插拔状态，具体处理可参考 :func:`usb_mode_monitor_debounce_timeout_cb`。

USB_MODE_MONITOR管脚GPIO中断处理如下：

.. code-block:: c

   static void zmk_mode_monitor_callback(const struct device *dev, struct gpio_callback *gpio_cb,
                                      uint32_t pins) {
      ...
      } else if ((!strcmp(dev->name, detect_usb.port->name)) && (pins >> detect_usb.pin == 1)) {
         gpio_pin_interrupt_configure_dt(&detect_usb, GPIO_INT_DISABLE);

         if (usb_mode_monitor_trigger_level == GPIO_PIN_LEVEL_HIGH) {
               is_usb_in_debonce_check = true;
               usb_in_debonce_timer_num = 0;
         } else {
               gpio_pin_interrupt_configure_dt(&detect_usb, GPIO_INT_DISABLE);
               is_usb_out_debonce_check = true;
               usb_out_debonce_timer_num = 0;
         }
         k_timer_start(&usb_mode_monitor_timer, K_MSEC(50), K_NO_WAIT);
      }
   }

电池电量获取
~~~~~~~~~~~~~~

电池电压会通过外部电路分压到0.9V以下，然后通过ADC进行采样。采样得到电压值后，通过换算得到电池电压和电量百分比。

.. figure:: ../figures/Battery_voltage_divider_and_ADC_sampling_circuit.*
   :align: center
   :scale: 60%
   :name: 电池电压分压和ADC采样电路

   电池电压分压和ADC采样电路

管脚配置
^^^^^^^^^^

ADC采样所使用的管脚在 :file:`rtk_keyboard.overlay` 中定义，ADC管脚只能使用 P2_0 ~ P2_7，对应 ADC0 ~ ADC7。

.. code-block:: c

   vbatt: vbatt {
      compatible = "zmk,battery-voltage-divider";
      io-channels = <&adc 2>;
      output-ohms = <100000>;
      full-ohms = <(100000 + 100000)>;
   };

ADC初始化
^^^^^^^^^^^

支持不同的channel独立配置 :mod:`bypass mode` 或 :mod:`divide mode`。目前ADC channel2配置为 :mod:`bypass mode` ， :mod:`is-bypass-mode` 设置为0x4, 默认使用one shot采样。

.. code-block:: c

   &adc {
      pinctrl-0 = <&adc_default>;
      pinctrl-names = "default";
      #address-cells = <1>;
      #size-cells = <0>;
      status = "okay";
      is-bypass-mode = <0x4>;
      channel@2 {
         reg = <2>;
         zephyr,gain = "ADC_GAIN_1";
         zephyr,reference = "ADC_REF_INTERNAL";
         zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
         zephyr,resolution = <12>;
      };
   };

电量获取和换算
^^^^^^^^^^^^^^^^

设置宏定义 **CONFIG_ZMK_BATTERY_REPORT_INTERVAL** 可以调整电量定时采样的频率，默认是60s采集一次，打开软件定时器 :mod:`battery_timer`，在timer的callback中定时进行ADC采样和换算获取电池电压，具体函数调用流程处理为 :func:`zmk_battery_timer`-> :func:`zmk_battery_work`-> :func:`zmk_battery_update` -> :func:`sensor_sample_fetch_chan` -> :func:`bvd_sample_fetch` -> :func:`lithium_ion_mv_to_pct_rtk`。

获得电池电压后，需要根据电池的放电和充电曲线，得到具体的电量百分比。

灯效
-------

LED可分为gpio leds和pwm leds两种。gpio leds可以通过PAD输出高低电平直接控制LED，一般用于实现颜色和闪烁效果简单的灯效；pwm leds可以通过PWM输出控制LED，一般用于实现颜色和闪烁效果复杂且精确的灯效。通过宏 **LED_NUM_MAX** 设置LED的总个数。

PAD输出控制
~~~~~~~~~~~~~

在设备树 :file:`rtl_keyboard.overlay` 中配置gpio leds对应的pin以及个数，配置如下：

.. code-block:: c

    gpio_leds: gpio_leds {
        compatible = "zmk,gpio-leds-all";
        pwr-gpios = <&gpiob 29 GPIO_ACTIVE_HIGH>;
        cap-gpios = <&gpioa 28 GPIO_ACTIVE_HIGH>;
        num-gpios = <&gpioa 16 GPIO_ACTIVE_HIGH>;
        ble-gpios = <&gpioa 27 GPIO_ACTIVE_HIGH>;
        ppt-gpios = <&gpioa 26 GPIO_ACTIVE_HIGH>;
        other-gpios = <&gpioa 23 GPIO_ACTIVE_HIGH>;
    };

PAD输出控制LED的原理如下：通过结构体 :mod:`T_GPIO_LEDS_GLOBAL_DATA` 记录每个LED的亮灭情况，以及需要闪烁的次数，通过 :func:`led_blink_start` 接口使能某个LED闪烁事件，并传入闪烁时间；通过 :func:`led_blink_exit` 接口停止某个LED闪烁事件。

结构体 :mod:`T_GPIO_LEDS_GLOBAL_DATA` 如下：

.. code-block:: c

   typedef struct gpio_leds_global_data {
      bool led_flag;
      struct gpio_dt_spec *led;
      uint8_t blink_cnt;

   } T_GPIO_LEDS_GLOBAL_DATA;

程序中默认的LED事件以及相关的结构体参数在数组 :mod:`led_event_arr` 中配置。

PWM输出控制
~~~~~~~~~~~~~

PWM输出是通过硬件定时器实现的，一路PWM就需要配置一个硬件定时器（包含普通的HW Timer和Enhance Timer），因此一个RGB灯需要3个硬件定时器。以一组RGB灯为例，LED的管脚和所使用的硬件定时器配置如下：

.. code-block:: c

   &timer2 {
      status = "okay";
      prescaler = <1>;
      pwm2: pwm2 {
         status = "okay";
         pinctrl-0 = <&pwm2_default>;
         pinctrl-names = "default";
      };
   };
   &timer3 {
      status = "okay";
      prescaler = <1>;
      pwm3: pwm3 {
         status = "okay";
         pinctrl-0 = <&pwm3_default>;
         pinctrl-names = "default";
      };
   };

   &timer4 {
      status = "okay";
      prescaler = <1>;
      pwm4: pwm4 {
         status = "okay";
         pinctrl-0 = <&pwm4_default>;
         pinctrl-names = "default";
      };
   };

   &pinctrl {
      pwm2_default: pwm2_default {
         group1 {
               psels = <RTL87X2G_PSEL(TIMER_PWM2, P2_4,DIR_OUT, DRV_LOW, PULL_DOWN)>;
         };
      };
      pwm3_default: pwm3_default {
         group1 {
               psels = <RTL87X2G_PSEL(TIMER_PWM3, P2_3,DIR_OUT, DRV_LOW, PULL_DOWN)>;
         };
      };
      pwm4_default: pwm4_default {
         group1 {
               psels = <RTL87X2G_PSEL(TIMER_PWM4, P9_3,DIR_OUT, DRV_LOW, PULL_DOWN)>;
         };
      };
   };

PWM输出控制LED的原理如下：通过 :file:`led_pwm.c` 中相关的api控制，目前支持的有 :mod:`on/off/blink/set_brightness`。

设备树描述如下：

.. code-block:: c

    battery_pwm_leds: pwmleds {
        compatible = "pwm-leds";
        pwm_led_0 {
            pwms = <&pwm2 0 10000 PWM_POLARITY_INVERTED>;
        };
        pwm_led_1 {
            pwms = <&pwm3 0 10000 PWM_POLARITY_INVERTED>;
        };
        pwm_led_2 {
            pwms = <&pwm4 0 10000 PWM_POLARITY_INVERTED>;
        };
    };

   static const struct led_driver_api led_pwm_api = {
      .on		= led_pwm_on,
      .off		= led_pwm_off,
      .blink		= led_pwm_blink,
      .set_brightness	= led_pwm_set_brightness,
   };

看门狗
---------

RTL87x2G平台有两种看门狗，可以分别在 CPU active 和 DLPS 状态下工作，在 :file:`rtk_keyboard.overlay` 中设置两种看门狗的状态和超时时间（默认设置的是5秒），喂狗时间是（initial-timeout-ms - 1）秒，超过时间没有喂狗会自动重启。打开看门狗后，会开一个软件定时器，每隔一段时间（小于超时时间）定时喂狗。

.. code-block:: c

   &corewdt {
      status = "disabled";
      initial-timeout-ms = <5000>;
   };

   &aonwdt {
      status = "okay";
      initial-timeout-ms = <5000>;
   };

可以调用接口 :func:`sys_reboot`，主动触发看门狗重启，可以记录重启的原因，在IC重启上电后可以获取重启原因。用户可以添加新的原因至 :file:`reset_reason.h` 的 :mod:`T_SW_RESET_REASON` 中。

.. code-block:: c

   /* Overrides the weak ARM implementation */
   void sys_arch_reboot(int type)
   {
      extern void WDG_SystemReset(int wdt_mode, int reset_reason);
      WDG_SystemReset(0, type);
   }

DFU
------

本文中DFU（Device Firmware Upgrade）是指设备的固件升级，基于USB来实现，目前做法是在flash map的appdata6区域中烧录一版支持dfu的app以及定制的system patch, 在上电时检测指定电平的状态，来切换到appdata6执行。

DFU的原理是，通过 MPPackTool 将想要升级的image进行打包，而后通过 CFUDownloadTool 将打包的image内容通过USB传输给键盘，键盘将需要升级的image存储到IC的OTA Temp区，当数据传输完成并校验通过，会自动重启将OTA Temp区所有image搬运到Flash的运行区域，以实现固件升级。

DFU支持一次打包并升级一个或多个image，可以打包并升级以下4个image：:file:`boot patch`、:file:`system patch`、:file:`stack patch`、:file:`app image`。但一次打包的image大小总和（不包括 :file:`boot patch`）不能超过OTA Temp区大小。

DFU数据传输完成后，需要把升级image从OTA Temp区搬运到Flash运行区，当该过程中断电重启后，会重新搬运，IC不会变砖。

DFU升级单独使用了一个USB interface，基于HID set/get report来实现升级过程的数据交互。

低功耗
---------

DLPS
~~~~~~~~

DLPS功能，是当CPU和外设不需要工作时，关闭相关模块电源，以达到省电的目的。在 :file:`rtk_keyboard.overlay` 中配置宏定义如下，打开DLPS功能。

.. code-block:: c

   CONFIG_PM=y
   CONFIG_PM_DEVICE=y
   CONFIG_PM_POLICY_CUSTOM=n # use zephyr pm flow, enable this config; use realtek pm flow, disable this config
   CONFIG_RUNTIME_NMI=y #customize NMI for power manager
   CONFIG_REALTEK_USING_SDK_LIB=y

APP可以通过 :func:`platform_pm_register_callback_func` 注册dlps相关的函数，用来管控进出dlps的时机，以及定义进出dlps时所需要做的事情：

.. code-block:: c

   #if CONFIG_PM
   #include "power_manager_unit_platform.h"
   enum PMCheckResult app_enter_dlps_check(void) {
      DBG_DIRECT("app check dlps flag %d", app_global_data.is_app_enabled_dlps);
      return app_global_data.is_app_enabled_dlps ? PM_CHECK_PASS : PM_CHECK_FAIL;
   }
   static void app_dlps_check_cb_register(void) {
      platform_pm_register_callback_func((void *)app_enter_dlps_check, PLATFORM_PM_CHECK);
   }
   #endif

Platform, Drivers都有适配zephyr os下的PM机制。PM的初始化流程请参考 :file:`power.c` 中的 :func:`rtl87x2g_power_init`。进出DLPS执行的函数分别对应 :func:`pm_suspend_devices_rtk` 和 :func:`pm_resume_devices_rtk`。

.. code-block:: c

   /* Initialize power system */
   static int rtl87x2g_power_init(void)
   {
      int ret = 0;

      bt_power_mode_set(BTPOWER_DEEP_SLEEP);
      power_mode_set(POWER_DLPS_MODE);
      z_arm_nmi_set_handler(NMI_Handler);
      k_work_queue_init(&rtk_pm_workq);
      k_work_queue_start(&rtk_pm_workq, rtk_pm_workq_stack_area,
                  K_THREAD_STACK_SIZEOF(rtk_pm_workq_stack_area),
                  RTK_PM_WORKQ_PRIORITY, NULL);
      k_work_init(&work_timeout_process, timeout_process_handler);
      k_work_init(&work_device_resume, device_resume_handler);
      platform_pm_register_callback_func_with_priority((void *)pm_suspend_devices_rtk,
         PLATFORM_PM_STORE, 1);
      /* do timeout function process and devices & nvic resume in
      * rtk_pm_workq thread via zephyr's workq mechanism.
      */
      platform_pm_register_callback_func_with_priority((void *)submit_items_to_rtk_pm_workq,
         PLATFORM_PM_RESTORE, 1);

      return ret;
   }

   static int pm_suspend_devices_rtk(void)
   {
      /*clear pad wakeup pending bit */
      Pad_ClearAllWakeupINT();
      CPU_DLPS_Enter();

      Pinmux_DLPSEnter(PINMUX, (void *)&Pinmux_StoreReg);

      const struct device *devs;
      size_t devc;

      devc = z_device_get_all_static(&devs);

      num_susp_rtk = 0;

      for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
         int ret;

         /* Ignore uninitialized devices, busy devices, wake up sources, and
         * devices with runtime PM enabled.
         */
         if (!device_is_ready(dev) || pm_device_is_busy(dev) ||
            pm_device_state_is_locked(dev) ||
            pm_device_wakeup_is_enabled(dev) ||
            pm_device_runtime_is_enabled(dev)) {
            continue;
         }

         ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
         /* ignore devices not supporting or already at the given state */
         if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
            continue;
         } else if (ret < 0) {
            LOG_ERR("Device %s did not enter %s state (%d)",
               dev->name,
               pm_device_state_str(PM_DEVICE_STATE_SUSPENDED),
               ret);
            return ret;
         }

         TYPE_SECTION_START(pm_device_slots)[num_susp_rtk] = dev;
         num_susp_rtk++;
      }

      return 0;
   }

   void pm_resume_devices_rtk(void)
   {
      Pinmux_DLPSExit(PINMUX, (void *)&Pinmux_StoreReg);
      for (int i = (num_susp_rtk - 1); i >= 0; i--) {
         pm_device_action_run(TYPE_SECTION_START(pm_device_slots)[i],
                  PM_DEVICE_ACTION_RESUME);
      }

      CPU_DLPS_Exit();

      num_susp_rtk = 0;
   }

对于driver来讲，每一个driver(gpio, kscan, adc等)都可以看作是一个pm device，zephyr通过PM device列表来统一管理，以kscan driver为例进行说明, 在进入DLPS时执行pm_action中的 **PM_DEVICE_ACTION_SUSPEND**，退出DLPS时执行pm_action中的 **PM_DEVICE_ACTION_RESUME**。

.. code-block:: c

   #ifdef CONFIG_PM_DEVICE
   static int kscan_rtl87x2g_pm_action(const struct device *dev,
                                       enum pm_device_action action)
   {
      const struct kscan_rtl87x2g_config *config = dev->config;
      struct kscan_rtl87x2g_data *data = dev->data;
      KEYSCAN_TypeDef *keyscan = (KEYSCAN_TypeDef *)config->reg;
      int err;
      extern void KEYSCAN_DLPSEnter(void *PeriReg, void *StoreBuf);
      extern void KEYSCAN_DLPSExit(void *PeriReg, void *StoreBuf);

      switch (action)
      {
      case PM_DEVICE_ACTION_SUSPEND:

         KEYSCAN_DLPSEnter(keyscan, &data->store_buf);

         /* Move pins to sleep state */
         err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
         if ((err < 0) && (err != -ENOENT))
         {
               return err;
         }
         break;
      case PM_DEVICE_ACTION_RESUME:
         /* Set pins to active state */
         err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
         if (err < 0)
         {
               return err;
         }

         KEYSCAN_DLPSExit(keyscan, &data->store_buf);

         /* check wakeup pin status */
         int ret;
         const struct pinctrl_state *state;
         ret = pinctrl_lookup_state(config->pcfg, PINCTRL_STATE_SLEEP, &state);
         if ((err < 0) && (err != -ENOENT))
         {
               /* no kscan wakeup pin is configured */
               return ret;
         }
         else
         {
               /* there are kscan wakeup pins configured, check if they wakeup the system */
               for (uint8_t i = 0U; i < state->pin_cnt; i++)
               {
                  if (state->pins[i].wakeup_low || state->pins[i].wakeup_high)
                  {
                     if (System_WakeUpInterruptValue(state->pins[i].pin) == SET)
                     {
                           System_WakeUpPinDisable(state->pins[i].pin);
                           Pad_ClearWakeupINTPendingBit(state->pins[i].pin);
                           kscan_pm_check_state = PM_CHECK_FAIL;
                     }
                  }
               }
         }
         break;
      default:
         return -ENOTSUP;
      }

      return 0;
   }

CPU WFI
~~~~~~~~~~

当keyboard工作在usb模式时，可以快速进入WFI来省电。正常情况，CPU进入WFI需要耗费一些检查的时间，为了降低功耗，可以在确定不能进入DLPS的场景下，调用接口 :func:`pm_no_check_status_before_enter_wfi`，使CPU能更快进入WFI，但调用该接口后，无法再进入DLPS；当需要进入DLPS时，需要调用 :func:`pm_check_status_before_enter_wfi_or_dlps`，而后可以正常进入DLPS和CPU WFI。

程序中默认在以下几种情况下调用 :func:`pm_no_check_status_before_enter_wfi`。

   - 键盘处于USB模式下，且USB不在suspend状态。

程序默认在以下几种情况下调用 :func:`pm_check_status_before_enter_wfi_or_dlps`。

   -  键盘所有按键都松开时。

Flash 存储
------

请参考：https://docs.zephyrproject.org/latest/services/settings/index.html，打开setting相关的宏

.. code-block:: c

   CONFIG_BT_SETTINGS=y
   CONFIG_BT_KEYS_OVERWRITE_OLDEST=y
   CONFIG_FLASH=y
   CONFIG_FLASH_PAGE_LAYOUT=y
   CONFIG_FLASH_MAP=y
   CONFIG_NVS=y
   CONFIG_SETTINGS=y

通过 :file:`settings_store.c` 中的api, 实现flash的存储功能，主要有以下几种：

   1. settings_subsys_init: 初始化
   2. settings_load_subtree: 从flash中加载有限的序列化项目集，并执行注册的handler
   3. settings_save_one: 向flash中写入单个序列化值

产测
-------

产测模式的进入退出方式
~~~~~~~~~~~~~~~~~~~~~~~

通过GPIO触发进入
^^^^^^^^^^^^^^^^^^

:file:`board.h` 中宏 **THE_WAY_TO_ENTER_MP_TEST_MODE** 配置为 **ENTER_MP_TEST_MODE_BY_GPIO_TRIGGER** 时，上电检查 mp_test_pin_1 和 mp_test_pin_2 两个管脚电平，电平满足条件就进入产测模式。在 :func:`mp_test_mode_check_and_enter` 函数中调整进入产测模式的两个电平状态。

.. code-block:: c

   mode_monitor: mode_monitor {
      ...
      mptest-gpios = <&gpioa 25 GPIO_ACTIVE_HIGH>,
                       <&gpioa 24 GPIO_ACTIVE_HIGH>;
    };

进入产测模式后，可以在指定的channel打single tone测试，具体的vendor command也可根据实际情况调整

.. code-block:: c

   typedef struct {
	uint8_t pkt_type;
	uint16_t opcode;
	uint8_t length;
	uint8_t moduleID;
	uint8_t subcmd;
	uint8_t start;
	uint8_t channle;
	uint8_t power_type;
	uint8_t tx_power;
} T_SINGLE_TONE_VEND_CMD_PARAMS;

通过USB CDC feature
^^^^^^^^^^^^^^^^^^^^^

当使用gpio触发当做RF相关测试时，无法动态调整vendor command。我们内部的rftest tool默认使用hci uart，然而keyboard对外仅有usb接口，为优化此种情况，可以使用usb cdc driver, 在做RF test时将usb虚拟成串口使用。

zephyr有自带的cdc driver，code在 :file:`subsys/usb/device/class/cdc_acm.c`, 此外以下宏需要打开：

**CONFIG_SERIAL**
**CONFIG_UART_INTERRUPT_DRIVEN**
**CONFIG_UART_LINE_CTRL**
**CONFIG_USB_CDC_FOR_RF_TEST**

App中在编译时如果 **CONFIG_USB_CDC_FOR_RF_TEST** 为1， 上电会作usb cdc相关的初始化，code在 :file:`usb_cdc.c` 中：

.. code-block:: c

   int usb_cdc_init(void) {
      const struct device *dev;
      uint32_t baudrate, dtr = 0U;
      int ret;

      dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
      if (!device_is_ready(dev)) {
         LOG_ERR("CDC ACM device not ready");
         return 0;
      }

      LOG_INF("usb cdc init");

      uart_irq_callback_set(dev, interrupt_handler);

      /* Enable rx interrupts */
      uart_irq_rx_enable(dev);

      vhci_init();
      return 0;
   }
usb cdc初始化完成后会出现一个新的com口，通过rftest tool open该com口，tool会下发一系列command, 读取IC的相关信息。这里将tool下发的指令在usb cdc中断中通过hci api下发给lowstack, lowstack 会在注册的callback中回复，回复的数据再通过usb cdc发送给tool, 形成闭环。

此外，usb cdc虚拟出的串口也可以用来做console, 输出log; 也可以结合上位机，制订一些新协议，来定制app的行为；并且虽然硬件上都是usb, 但对于os来讲，分别是usb device和uart device, 两者并不影响，可以共存。