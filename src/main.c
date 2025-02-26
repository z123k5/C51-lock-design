#include <absacc.h>     // 扩展接口库
#include <intrins.h>    // 移位函数库
#include <reg52.h>      // 基本89C52库

// 此处切换仿真器代码和开发板代码
#if 1
#define DISP_SEG P0 //扩展接口8255(U3)PA口地址,数码管字段输出口
#define DISP_BIT P2 //扩展接口8255(U3)PB口地址,数码管字位输出口
#define VKEY_RL P1  //扩展接口8255(U3)PC口地址,矩阵键盘行、列接口
#else
#define DISP_SEG XBYTE[0xfefc]   //扩展接口8255(U3)PA口地址,数码管字段输出口
#define DISP_BIT XBYTE[0xfefd]   //扩展接口8255(U3)PB口地址,数码管字位输出口
#define VKEY_RL XBYTE[0xfefe]    //扩展接口8255(U3)PC口地址,矩阵键盘行、列接口
#define U3CON XBYTE[0xfeff]      //扩展接口8255(U3)控制口地址
#endif
unsigned char gearTime = 0;         // 门锁时间（秒）
sbit gear = P3 ^ 0;
sbit beep = P3 ^ 2;
// sbit key6 = P3^2;

// 显示缓存
unsigned char dispBuf[10] = {0x10};
enum { MENU_TIME, MENU_DATE, MENU_PWD, MENU_PWD_CHANGE1, MENU_PWD_CHANGE2, MENU_PWD_CHANGE3, MENU_ZIJIAN };

// 字符字典
enum {
    DK_0,
    DK_1,
    DK_2,
    DK_3,
    DK_4,
    DK_5,
    DK_6,
    DK_7,
    DK_8,
    DK_9,
    DK_A,
    DK_B,
    DK_C,
    DK_D,
    DK_E,
    DK_F,
    DK_Space,
    DK_Del,
    DK_UdLine,
    DK_Dot,
    DK_D0,
    DK_D1,
    DK_D2,
    DK_D3,
    DK_D4,
    DK_D5,
    DK_D6,
    DK_D7,
    DK_D8,
    DK_D9,
    DK_DA,
    DK_DB,
    DK_DC,
    DK_DD,
    DK_DE,
    DK_DF,
    DK_NUL,
    DK_EOF,
    DK_P,
    DK_Equal,
    DK_Rectangle
};
// 字符字段
unsigned char code segtable[] = {
    0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, // 0~9字段码
    0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e, 0xff, 0xbf, 0xf7, 0x7f, // A~F,空格,-,_,.字段码
    0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x10, // 0.~9.字段码
    0x08, 0x03, 0x46, 0x21, 0x06, 0x0e,                         // A.~F.字段码
    0xcc, 0xee, 0x8c, 0xb7, 0xa3                                // 空码,结束码,P,=,正方形
};
unsigned char Scan_code[] = {0xee, 0xed, 0xeb, 0xe7, 0xde, 0xdd, 0xdb, 0xd7,
                             0xbe, 0xbd, 0xbb, 0xb7, 0x7e, 0x7d, 0x7b, 0x77}; //键盘扫描码
unsigned char scan_buf[6] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10}; //键盘滚动滤波缓冲单元

// 密码
unsigned char pwd[6] = {5, 3, 5, 8, 6, 6};  // 默认密码
unsigned char input[6], pwdBack[6];         // 输入密码、新密码
unsigned char inputLen = 0;                 // 输入密码的长度
unsigned char triedTimes = 0;               // 尝试连续输入密码的次数
unsigned char triedTimer = 0;               // 输入密码计数器
unsigned int temp;                          // 临时变量

// 菜单
unsigned char menu = MENU_TIME;     // 菜单状态变量
unsigned char zijianpos = 0;        // 自检显示位置变量

unsigned char settime = 0;          // 设置时间 ，0：不设置，1：设置时间，2：设置日期
unsigned char sgClick = 0;          // 是否单击
unsigned char hFast = 0;            // 时快调按下
unsigned long hhFastTime = 0;       // 时快调计数
unsigned int blinkTime = 0;         // 矩阵按键设置时间的光标闪烁计时器
unsigned char blinkIndex = 0;       // 矩阵按键设置时间的光标闪烁位置
unsigned char playType = 0;         // 播放类型：0不播放，1按键音，2失败，3成功
unsigned int playTimer = 0;         // 蜂鸣器响一声计时器
unsigned int playTime = 0;          // 播放次数
unsigned char playSound = 0;        // 播放开关

// 时间日期
unsigned int year = 2022;
unsigned char month = 1;
unsigned char day = 1;
unsigned char sec = 0;
unsigned char min = 0;
unsigned char hour = 0;
unsigned int tempA, tempB, tempC;   // 用来临时存储时间日期

// 延时函数，毫秒单位
void delay_ms(unsigned int j) {
    unsigned int k;
    for (; j > 0; j--)
        for (k = 0; k < 100; k++)
            ;
}

// 拆字，在每个菜单分别放入显示缓存
void display_buf() {
    int i;
    switch (menu) {
    case MENU_TIME:
        if(settime == 1) {
            // 时
            dispBuf[0] = tempA / 10;
            dispBuf[1] = tempA % 10;
            dispBuf[2] = DK_Del;
            // 分
            dispBuf[3] = tempB / 10;
            dispBuf[4] = tempB % 10;
            dispBuf[5] = DK_Del;
            // 秒
            dispBuf[6] = tempC / 10;
            dispBuf[7] = tempC % 10;
            dispBuf[8] = DK_EOF;
        } else {
            // 时
            dispBuf[0] = hour / 10;
            dispBuf[1] = hour % 10;
            dispBuf[2] = DK_Del;
            // 分
            dispBuf[3] = min / 10;
            dispBuf[4] = min % 10;
            dispBuf[5] = DK_Del;
            // 秒
            dispBuf[6] = sec / 10;
            dispBuf[7] = sec % 10;
            dispBuf[8] = DK_EOF;
        }
        break;
    case MENU_DATE:
        if(settime == 2) {
            // 年
            dispBuf[0] = tempA / 1000;
            dispBuf[1] = (tempA - dispBuf[0] * 1000) / 100;
            dispBuf[2] = (tempA - tempA / 100 * 100) / 10;
            dispBuf[3] = DK_D0 + tempA % 10;
            // 月
            dispBuf[4] = tempB / 10;
            dispBuf[5] = DK_D0 + tempB % 10;
            // 日
            dispBuf[6] = tempC / 10;
            dispBuf[7] = tempC % 10;
            dispBuf[8] = DK_EOF;
        } else {
            // 年
            dispBuf[0] = year / 1000;
            dispBuf[1] = (year - dispBuf[0] * 1000) / 100;
            dispBuf[2] = (year - year / 100 * 100) / 10;
            dispBuf[3] = DK_D0 + year % 10;
            // 月
            dispBuf[4] = month / 10;
            dispBuf[5] = DK_D0 + month % 10;
            // 日
            dispBuf[6] = day / 10;
            dispBuf[7] = day % 10;
            dispBuf[8] = DK_EOF;
        }
        break;

    case MENU_PWD: case MENU_PWD_CHANGE1: case MENU_PWD_CHANGE2: case MENU_PWD_CHANGE3:
        if(triedTimes >= 3) {
            dispBuf[0] = DK_F;
            dispBuf[1] = DK_P;
            dispBuf[2] = DK_DD;
            dispBuf[3] = DK_Equal;
            dispBuf[4] = triedTimer / 10;
            dispBuf[5] = triedTimer % 10;
            dispBuf[6] = DK_D5;
            dispBuf[7] = DK_Space;
        } else {
            if(menu == MENU_PWD || menu == MENU_PWD_CHANGE1) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_Equal;
            } else if(menu == MENU_PWD_CHANGE2) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_D1;
            } else if(menu == MENU_PWD_CHANGE3) {
                dispBuf[0] = DK_P;
                dispBuf[1] = DK_D2;
            }
            for (i = 2; i < 8; i++) {
                if (i - 2 >= inputLen)
                   break;
                if (menu == MENU_PWD_CHANGE2 || menu == MENU_PWD_CHANGE3)
                    dispBuf[i] = input[i - 2];
                else
                    dispBuf[i] = DK_Rectangle;
            }
            for (; i < 8; i++)
                dispBuf[i] = DK_UdLine;
            dispBuf[8] = DK_NUL;
        }
        break;
    case MENU_ZIJIAN:
        for (i=0; i<8; i++) {
            if (i == zijianpos) {
                dispBuf[i] = DK_Del;
            } else dispBuf[i] = DK_Space;
        }
        break;
    }
}

// 显示缓存到数码管
void display_swbuf() {
    unsigned int i, j;
#ifdef U3CON
    unsigned int disp_bit_con = 0x80; //字位控制码初值
#else
    unsigned int disp_bit_con = 0x01; //字位控制码初值
#endif
    DISP_SEG = 0xff;                  //关显示，使各位亮度一致、避免频闪

    for (i = 0, j = 0; i < 10 && dispBuf[i] != DK_EOF; i++) {
        // 闪烁位
        if (!(settime && blinkIndex == i && blinkTime < 900)) {
            if (dispBuf[i] == DK_NUL) // 空码忽略
                continue;
            DISP_BIT = disp_bit_con;         // 8255（U3）的B口输出字位控制码
            DISP_SEG = segtable[dispBuf[i]]; //显缓内容查字段码表得到字段码;
            delay_ms(1);                     //延时1ms
            DISP_SEG = 0xff;                 //关显示，使各位亮度一致、避免频闪
        }
#ifdef U3CON
        disp_bit_con >>= 1; //字位控制码左移一位
#else
        disp_bit_con <<= 1; //字位控制码左移一位
#endif
        j++;
    }
    for (; i < 10; i++) {
        DISP_BIT = disp_bit_con;       // 8255（U3）的B口输出字位控制码
        DISP_SEG = segtable[DK_Space]; //显缓内容查字段码表得到字段码;
        delay_ms(1);                   //延时1ms
        DISP_SEG = 0xff;               //关显示，使各位亮度一致、避免频闪
#ifdef U3CON
        disp_bit_con >>= 1;            //字位控制码左移一位
#else
        disp_bit_con <<= 1;            //字位控制码左移一位
#endif
    }
}

// 计时器初始化
void timer_init() {
    TMOD = 0x22; // T1 方式2、定时、GATE=0；T0 方式2、定时，GATE=0
    TH1 = 0x00;
    TL1 = 0x00; //置T1计数初值0,计数256，每次中断定时256x12/11.0592 us
    TH0 = 0x00;
    TL0 = 0x00; //置T1计数初值0,计数256，每次中断定时256x12/11.0592 us
    EA = 1;     //开中断总允许
    //ET1 = 1;    // T1中断允许
    //TR1 = 1;    //启动T1定时(时钟是否走时开关 1/0)！！！
    ET0 = 1;    // T1中断允许
    TR0 = 1;    //启动T1定时(时钟是否走时开关 1/0)！！！
}



// 判断是否是闰年
unsigned char isLeapYear(unsigned int y) {
    return y%400 == 0 || y % 4 == 0 && y %100 != 0;
}

// 获取某个月的天数
unsigned char getDayOfMonth(unsigned int y, unsigned char m) {
    switch(m) {
        case 1:case 3:case 5:case 7:case 8:case 10:case 12:
            return 31;
        case 2: return isLeapYear(y) ? 29:28;
        default:
            return 30;
    }
}

// 更新时间处理函数
void updateDateTime(unsigned char set, unsigned char index)
{
    switch(set) {
        case 0:
        if (sec >= 60) {
            sec = 0;
            min++;
            if (min >= 60) {
                min = 0;
                hour++;
                if (hour >= 24) {
                    hour = 0;
                    day++;
                    if(day >= getDayOfMonth(year, month) + 1) {
                        day = 1;
                        month++;
                        if(month >= 13) {
                            month = 1;
                            year++;
                            if(year >= 9999) {
                                year = 2000;
                            }
                        }
                    }
                }
            }
        }
        break;
        
        case 1:
            switch(index) {
                case 7: case 6:
                    if (tempC >= 60) {
                        tempC = 0;
                        tempB++;
                    }
                case 4:case 3:
                    if (tempB >= 60) {
                        tempB = 0;
                        tempA++;
                    }
                case 1:case 0:
                    if (tempA >= 24) {
                        tempA = 0;
                    }
            }
        break;
        
        case 2:
            switch(index) {
                case 7:case 6:case 5:case 4:
                    if (tempC >= getDayOfMonth(tempA, tempB) + 1) {
                        tempC = 1;
                        tempB++;
                    }
                case 3:case 2:
                    if (tempB >= 13) {
                        tempB = 1;
                        tempA++;
                    }
                case 1:case 0:
                    if (tempA >= 9999) {
                        tempA = 0;
                    }
            }
        break;
    }
}

// 定时器0中断处理函数
void timer1_isr() interrupt 1 {
    static unsigned int j = 0;

    // 时、分快调计数
    if (hhFastTime < 3600)
        hhFastTime++;

    blinkTime++;
    if (blinkTime >= 1800)
        blinkTime = 0;
    
    if(playType != 0) {
        playTimer++;
    }
    
    j++;
    if (menu == MENU_ZIJIAN && hhFastTime >= 900) {
        playType = 1;
        playTime = 100;
        playTimer = 0;
        zijianpos++;
        if(zijianpos >= 8) menu = MENU_TIME;
        hhFastTime = 0;
    }
    if (j >= 3600) {
        j = 0;
        
        if(gear) {
            if(gearTime <= 0) gear = 0;
            else gearTime--;
        }
        
        if(triedTimes >= 3) {
            if(triedTimer == 0) triedTimes = 0;
            else triedTimer--;
        }

        //秒、分、时修改*如按日期规则修改，则可显示日期*
        sec++;
        updateDateTime(0, 0);
    }
}

// 矩阵键盘扫描
void key_scan() {
    unsigned int i;
    unsigned char line; //行输入
    unsigned char row;  //列输入

    // 矩阵按键扫描
    // 滚动存储
    scan_buf[0] = scan_buf[1]; //新态、旧态滚动存储
    scan_buf[2] = scan_buf[3]; //滤波单元滚动存储
    scan_buf[3] = scan_buf[4];

#ifdef U3CON
    // 输入新键值
    //DISP_SEG = 0xff;          //关显示
    U3CON = 0x81;               //行（PC7-PC4）为输出，列(PC3-PC0)为输入
    VKEY_RL = 0x00;             //行输出0 
    row = VKEY_RL;

    //DISP_SEG = 0xff;          //关显示
    U3CON = 0x88;               //行（PC7-PC4）为输入，列(PC3-PC0)为输出
    VKEY_RL = 0x00;             //列输出0
    line = VKEY_RL;
#else
    DISP_SEG = 0xff; //关显示
    VKEY_RL = 0x0f;  //行输出0
    delay_ms(1);
    row = VKEY_RL & 0x0f;
    DISP_SEG = 0xff; //关显示
    VKEY_RL = 0xf0;  //列输出0
    delay_ms(1);
    line = VKEY_RL & 0xf0;
#endif
    row = row & 0x0f;         //屏蔽高4位
    line = line & 0xf0;       // 屏蔽低4位
    scan_buf[5] = line | row; //合并行列值

    // 键盘编码-->键值存入scan_buf[4]
    for (i = 0; i < 16; i++) {
        if (scan_buf[5] == Scan_code[i]) {
            scan_buf[4] = i; //查键盘扫描码，转换成键值0-15
            break;
        } else
            scan_buf[4] = 16; //其它情况为16
    }

    // 滤波
    if ((scan_buf[2] == scan_buf[3]) && (scan_buf[2] == scan_buf[4]))
        scan_buf[1] = scan_buf[4];
}

// 矩阵键盘四键加减功能，与自选实验冲突，只用其中一个
int key_pro_4() {
    int i;
    // 矩阵按键事件
    // 按下
    if ((scan_buf[0] == 0x10) && (scan_buf[1] != 0x10)) {
        //在密码菜单输入密码，修改密码
        if (menu == MENU_PWD || menu == MENU_PWD_CHANGE1 || menu == MENU_PWD_CHANGE2 || menu == MENU_PWD_CHANGE3) {
            if(triedTimes >= 3) {
                // 播放无法操作声音
                playTime = 700;
                playTimer = 0;
                playType = 3;
                return 1;
            }
            
            if (inputLen < 6) {
                if (scan_buf[1] >= 1 && scan_buf[1] <= 3)
                    input[inputLen++] = scan_buf[1];
                else if (scan_buf[1] >= 5 && scan_buf[1] <= 7)
                    input[inputLen++] = scan_buf[1] - 1;
                else if (scan_buf[1] >= 9 && scan_buf[1] <= 11)
                    input[inputLen++] = scan_buf[1] - 2;
                else if (scan_buf[1] == 14)
                    input[inputLen++] = 0;
            }
            
            if (inputLen == 6) {
                if(menu == MENU_PWD_CHANGE1) {
                    for(i = 0; i < 6; i++)
                        if(pwd[i] != input[i]) break;
                    if(i==6) {
                        menu = MENU_PWD_CHANGE2;
                        inputLen = 0;
                        // 播放输入成功声音
                        playTime = 300;
                        playTimer = 0;
                        playType = 4;
                    } else {
                        // 播放输入错误声音
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                        triedTimes++;
                        if(triedTimes >= 3) {
                            // 30秒钟后再输入
                            triedTimer = 30;
                        }
                    }
                } else if(menu == MENU_PWD_CHANGE2) {
                    for(i = 0; i < 6; i++)
                        pwdBack[i] = input[i];
                    menu = MENU_PWD_CHANGE3;
                    inputLen = 0;
                    
                    // 播放第一次输入成功声音
                    playTime = 300;
                    playTimer = 0;
                    playType = 4;
                    
                } else if(menu == MENU_PWD_CHANGE3) {
                    for(i = 0; i < 6; i++)
                        if(pwdBack[i] != input[i]) break;
                    if(i==6) {
                        for(i = 0; i < 6; i++)
                            pwd[i] = input[i];
                        menu = MENU_TIME;
                        inputLen = 0;
                        // 播放输入成功声音
                        playTime = 400;
                        playTimer = 0;
                        playType = 4;
                    } else {
                        inputLen = 0;
                        // 播放输入错误声音
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                    }
                } else {
                    for(i = 0; i < 6; i++)
                        if(pwd[i] != input[i]) break;
                    if(i == 6) {
                        gear = 1;
                        gearTime = 3;  // 开锁3秒
                        inputLen = 0;
                        
                        menu = MENU_TIME;
                        // 播放输入成功声音
                        playTime = 300;
                        playTimer = 0;
                        playType = 4;
                        triedTimes = 0;
                    } else {
                        // 播放输入错误声音
                        playTime = 700;
                        playTimer = 0;
                        playType = 3;
                        triedTimes++;
                        if(triedTimes >= 3) {
                            // 30秒钟后再输入
                            triedTimer = 30;
                        }
                    }
                }
                inputLen = 0;
            }
        }
    }
    return 0;
}

int key_pro_zixuan() {
    unsigned char key;
    
    // 按下
    if ((scan_buf[0] == 16) && (scan_buf[1] != 16)) {
        playType = 1;
        playTime = 200;
        playTimer = 0;
        
        switch (scan_buf[1]) {
        case 13:
            if(settime) settime = 0;
            else {
                menu++;
                if (menu > MENU_PWD)
                    menu = MENU_TIME;
                if (menu == MENU_PWD)
                    inputLen = 0;
            }
            return 1;
        case 15:
            if(settime == 1) {   // 更新时间
                hour = tempA;
                min = tempB;
                sec = tempC;
                settime = 0;
            } else if(settime == 2) {   // 更新日期
                year = tempA;
                month = tempB;
                day = tempC;
                settime = 0;
            } else if(hhFastTime < 1800) { // 0.2s
                // 双击事件
                menu = MENU_PWD_CHANGE1;
                inputLen = 0;
                return 1;
            } else {
                if(menu == MENU_TIME) {
                    settime = 1; // 设置时间
                    tempA = hour;
                    tempB = min;
                    tempC = sec;
                } else if(menu == MENU_DATE) {
                    settime = 2; // 设置日期
                    tempA = year;
                    tempB = month;
                    tempC = day;
                }
            }
        }
        if(settime) {
            if (1 <= scan_buf[1] && scan_buf[1] <= 3) {
                key = scan_buf[1];
            } else if (5 <= scan_buf[1] && scan_buf[1] <= 7) {
                key = scan_buf[1] - 1;
            } else if (9 <= scan_buf[1] && scan_buf[1] <= 11) {
                key = scan_buf[1] - 2;
            } else if (13 <= scan_buf[1] && scan_buf[1] <= 15) {
                key = scan_buf[1] - 3;
            }
            if (key < 10 || key == 11) {
                if (key == 11)
                    key = 0;
                switch (blinkIndex) {
                case 0:
                    if(settime == 1) {
                        // 设置时十位
                        tempA = key * 10 + tempA % 10;
                    } else {
                        // 设置年千位
                        tempA = tempA - (tempA / 1000 * 1000) + key * 1000;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 1:
                    if(settime == 1) {
                        // 设置时个位
                        tempA = (tempA / 10) * 10 + key;
                        updateDateTime(settime, blinkIndex);
                        blinkIndex++;
                    } else {
                        // 设置年百位
                        tempA = (tempA / 1000 * 10 + key) * 100 + (tempA - tempA / 100 * 100);
                        updateDateTime(settime, blinkIndex);
                    }
                    blinkIndex++;
                    break;
                case 2:
                    if(settime == 1) {
                        ;
                    } else {
                        // 设置年十位
                        tempA = (tempA / 100 * 10 + key) * 10 + tempA % 10;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 3:
                    if(settime == 1) {
                        // 设置分十位
                        tempB = key * 10 + tempB % 10;
                    } else {
                        // 设置年个位
                        tempA = tempA / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 4:
                    if(settime == 1) {
                        // 设置分个位
                        tempB = (tempB / 10) * 10 + key;
                        updateDateTime(settime, blinkIndex);
                        blinkIndex++;
                    } else {
                        // 设置月十位
                        tempB = key * 10 + tempB % 10;
                        updateDateTime(settime, blinkIndex);
                    }
                    blinkIndex++;
                    break;
                case 5:
                    if(settime == 1) {
                        ;
                    } else {
                        // 设置月个位
                        tempB = tempB / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 6:
                    if(settime == 1) {
                        // 设置秒十位
                        tempC = key * 10 + tempC % 10;
                    } else {
                        // 设置日十位
                        tempC = key * 10 + tempC % 10;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex++;
                    break;
                case 7:
                    if(settime == 1) {
                        // 设置秒个位
                        tempC = (tempC / 10) * 10 + key;
                    } else {
                        // 设置日个位
                        tempC = tempC / 10 * 10 + key;
                    }
                    updateDateTime(settime, blinkIndex);
                    blinkIndex = 0;
                    break;
                }
            }
            
            if (scan_buf[1] == 0) {
                // 向左移动，设置时、十位
                blinkIndex--;
                if (settime == 1) {
                    if (blinkIndex == 2 || blinkIndex == 5)
                        blinkIndex--;
                }
            }
            if (scan_buf[1] == 4) {
                // 向右移动，设置个位、分
                blinkIndex++;
                if (settime == 1) {
                    if (blinkIndex == 2 || blinkIndex == 5)
                        blinkIndex++;
                }
            }
            if (blinkIndex == 8)
                // 归位光标
                blinkIndex = 0;
            if (blinkIndex == -1)
                // 光标移至末尾
                blinkIndex = 7;
        }
    }
    
    // 松开
    if((scan_buf[0] != 0x10) && (scan_buf[1] == 0x10)) {
        switch(scan_buf[0]) {
            case 0:
                // 初始化时间日期
                hour = 23;
                min = 55;
                sec = 20;
                gear = 0;

                // 初始化密码
                pwd[0] = 5;
                pwd[1] = 3;
                pwd[2] = 5;
                pwd[3] = 8;
                pwd[4] = 6;
                pwd[5] = 6;

                // 初始化状态
                settime = 0;
                triedTimes = 0;               // 尝试连续输入密码的次数
                triedTimer = 0;               // 输入密码计数器
                zijianpos = 0;
                playSound = 0;

                // 进入自检菜单
                menu = MENU_ZIJIAN;
                break;
                
            case 15:
                // PB2 单双击LED松开
                if(!sgClick) {
                    sgClick = 1;
                    hhFastTime = 0; // 计时清零
                } else
                    sgClick = 0;
        }
    }
    return 0;
}

// 按键处理，分为两种
void key_pro() {
    // 设置时间日期
    if(key_pro_zixuan()) return;

    // 输入密码
    if(key_pro_4()) return;
}

// 蜂鸣器
void alarm() {
    // 蜂鸣器功能
    if(playType != 0) {
        if(playTime == 0) temp = 1;
        else temp = playTimer / playTime;
        if(temp % 2 == 0) playSound = 1;
        else playSound = 0;
        playTimer++;
        if(temp >= (playType << 1)) {
            playType = 0;
            playTimer = 0;
            playSound = 0;
        }
    }
    beep = !playSound;
}

void main() {
#ifdef U3CON
    U3CON = 0x89;   // 8255(U3)初始化，A、B口方式0输出，C口输入
#endif
    timer_init();
    delay_ms(60);

    // 初始化时间日期
    hour = 23;
    min = 55;
    sec = 20;
    gear = 0;
    
    // 进入自检菜单
    menu = MENU_ZIJIAN;

    // 主循环
    while (1) {
        // 刷新显示
        display_buf();
        display_swbuf();
        
        // 键盘检测
        key_scan();
        key_pro();

        // 蜂鸣器更新
        alarm();
    }
}